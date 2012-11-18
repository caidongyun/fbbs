#!/usr/bin/env perl

use strict;
use warnings;

use Digest::MD5 qw(md5);

use lib '.';
use Helper qw(get_options db_connect convert convert_time convert_file read_users $dir $dbh);

$| = 1;

get_options();
db_connect();

my $SCHEMA = 'posts';
my $post_digests = {};
my $boards = load_boards();
my $posts = read_posts($boards);

my ($alive_users, $past_users) = load_users();
insert_posts($posts, $alive_users, $past_users);

$dbh->disconnect;

sub load_boards
{
	my %boards;
	my $arr = $dbh->selectall_arrayref("SELECT id, name FROM boards");
	$boards{$_->[1]} = $_->[0] for (@$arr);
	\%boards;
}

sub load_users
{
	my $passwd_users = read_users();
	my %firstlogin = map { $_->[24] => $_->[19] } @$passwd_users;

	my $arr = $dbh->selectall_arrayref("SELECT id, name FROM alive_users");
	my %alive_users = map { $_->[1] => [$_->[0], $firstlogin{$_->[1]}] } @$arr;

	$arr = $dbh->selectall_arrayref("SELECT id, name FROM users WHERE NOT alive");
	my %past_users;
	for (@$arr) {
		if (not exists $past_users{$_->[1]} or $past_users{$_->[1]} < $_->[0]) {
			$past_users{$_->[1]} =  $_->[0];
		}
	}
	(\%alive_users, \%past_users);
}

sub read_posts
{
	my $boards = shift;
	my %posts;
	my $count = 0;

	while (my ($bname, $bid) = each %$boards) {
		for (qw/.NOTICE .DIGEST .DIR .TRASH .JUNK/) {
			$count += read_index($bname, $bid, $_, \%posts);
		}
		print "$count...";
	}
	\%posts;
}

sub read_index
{
	# 0 filename 1 id 2 gid 3 owner 4 title
	# 5 eraser 6 level 7 accessed 8 reid 9 timedeleted
	# 10 (date) 11 (board_id) 12 (type)
	my ($bname, $bid, $index, $posts, $inode) = @_;
	my ($fh, $buf);
	my $pcount = 0;
	open $fh, '<', "$dir/boards/$bname/$index" and do {
		while (1) {
			last if (read($fh, $buf, 256) != 256);
			my @t = unpack "Z72I2Z80Z67Z13i4", $buf;
			my $date;
			if ($t[0] =~ /^.\.(\d+)\./) {
				$date = $1;
				$t[0] = $bname . '/' . $t[0];
				push @t, $date, $bid, $index;

				if ($index eq '.NOTICE' or $index eq '.DIGEST') {
					my $content = convert_file("$dir/boards/$t[0]");
					my $md5 = md5($content);
					if (exists $post_digests->{$bid}{$md5}) {
						next;
					} else {
						$post_digests->{$bid}{$md5} = undef;
					}
				}

				my ($month, $year) = (localtime $date)[4, 5];
				my $archive = ($year + 1900) * 100 + $month + 1;
				push @{$posts->{$archive}}, \@t;
				++$pcount;
			}
		}
		close $fh;
	};
	$pcount;
}

sub insert_posts
{
	my ($posts, $alive_users, $past_users) = @_;

	my $pid_hash = {};
	my $base = 200_000_000;
	my $pid = 0;

	my @keys = sort keys %$posts;
	for (@keys) {
		my $array = delete $posts->{$_};
		print "sorting $_...";
		@$array = sort { $a->[10] <=> $b->[10] } @$array;
		print "finished\n";

		my $sth = $dbh->prepare(qq{
				INSERT INTO $SCHEMA.recent (id, reid, tid, owner, uname, stamp, board,
					sticky, digest, marked, locked, imported, water, title, content)
				VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
				}) or die $!;
		my $sth_deleted = $dbh->prepare(qq{
				INSERT INTO $SCHEMA.deleted (id, reid, tid, owner, uname, stamp, board,
					digest, marked, locked, imported, water, title, content,
					eraser, deleted, junk, bm_visible, ename)
				VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
				}) or die $!;

		for (@$array) {
			my ($file, $id, $gid, $owner, $title, $eraser,
					$level, $access, $reid, $deleted, $date, $bid, $type) = @$_;
			my $uid = get_uid($owner, $date, $alive_users, $past_users);

			$owner = convert($owner);
			$title = convert($title);
			my $content = convert_file("$dir/boards/$file");
			my $md5 = md5($content);
			if (exists $post_digests->{$bid}{$md5} and $type ne '.NOTICE'
					and $type ne '.DIGEST') {
				next;
			}

			if ($id != $gid) {
				if (not exists $pid_hash->{$bid}{$gid}) {
					$pid_hash->{$bid}{$gid} = ++$pid;
				}
				if (not exists $pid_hash->{$bid}{$reid}) {
					$pid_hash->{$bid}{$reid} = ++$pid;
				}
			}
			$pid_hash->{$bid}{$id} = ++$pid;
			$id = $pid + $base;
			$gid = $pid_hash->{$bid}{$gid} + $base;
			$reid = $pid_hash->{$bid}{$reid} + $base;

			my $digest = (($access & 0x10) or $type eq '.DIGEST') ? 1 : 0;
			my $marked = ($access & 0x8) ? 1 : 0;
			my $locked = ($access & 0x40) ? 1 : 0;
			my $imported = ($access & 0x0800) ? 1 : 0;
			my $water = ($access & 0x80) ? 1 : 0;
			my $stamp = convert_time($date);

			if ($type eq '.TRASH' or $type eq '.JUNK') {
				my $eid = get_uid($eraser, $deleted, $alive_users, $past_users);
				$deleted = convert_time($deleted);
				my $junk = ($access & 0x200) ? 1 : 0;
				my $bm_visible = ($type eq '.TRASH') ? 1 : 0;
				$sth_deleted->execute($id, $reid, $gid, $uid, $owner, $stamp, $bid,
						$digest, $marked, $locked, $imported, $water, $title, $content,
						$eid, $deleted, $junk, $bm_visible, $eraser);
			} else {
				my $sticky = ($type eq '.NOTICE') ? 1 : 0;
				$sth->execute($id, $reid, $gid, $uid, $owner, $stamp, $bid,
						$sticky, $digest, $marked, $locked, $imported, $water, $title,
						$content);
			}

			if ($pid % 1000 == 0) {
				print "$pid...";
				$dbh->commit;
			}
		}
	}
	$dbh->do("SELECT setval('posts.base_id_seq', ?)", undef, $pid + $base + 1);
	$dbh->commit;
}

sub get_uid
{
	my ($uname, $stamp, $alive_users, $past_users) = @_;
	if (exists $alive_users->{$uname} and $alive_users->{$uname}[1] <= $stamp) {
		return $alive_users->{$uname}[0];
	}
	$past_users->{$uname};
}
