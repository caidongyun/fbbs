#include "bbs.h"
#include "mmap.h"
#include "record.h"
#include "fbbs/board.h"
#include "fbbs/fileio.h"
#include "fbbs/helper.h"
#include "fbbs/mail.h"
#include "fbbs/session.h"
#include "fbbs/string.h"
#include "fbbs/post.h"
#include "fbbs/terminal.h"
USE_TRY;

extern char BoardName[];

#define M_MARK  0x01

struct textline {
	struct textline *prev;
	struct textline *next;
	int len;
	unsigned char attr;
	char data[WRAPMARGIN + 1];
};

static struct textline *firstline = NULL;
static struct textline *can_edit_begin = NULL;
static struct textline *can_edit_end = NULL;
static struct textline *top_of_win = NULL;
static struct textline *currline = NULL;
static struct textline *mark_begin, *mark_end;
static int currpnt = 0;
static char searchtext[80];
static int scrollen = 2;
static int moveln = 0;
static int shifttmp = 0;
static int ismsgline;
static int curr_window_line, currln;
static int redraw_everything;
static int insert_character = 1;
static int linelen = WRAPMARGIN;
static int mark_on;

int editansi = 0;
int enabledbchar = 1;

void vedit_key();
void process_MARK_action(int arg, char *msg);

/* for copy/paste */
#define CLEAR_MARK()  mark_on = 0; mark_begin = mark_end = NULL;
/* copy/paste */

static void strnput(const char *str)
{
	int count = 0;
	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			str++;
			continue;
		}
		outc(*str++);
	}
}

static void cstrnput(const char *str)
{
	int count = 0, tmp = 0;

	tmp = num_ans_chr(str);
	prints("%s", ANSI_REVERSE);
	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			tmp --;
			str++;
			continue;
		}
		outc(*str++);
	}
	while (++count < STRLEN+tmp)
		outc(' ');
	clrtoeol();
	prints("%s", ANSI_RESET);
}

static void msgline(void)
{
	char buf[256], buf2[STRLEN * 2];
	int tmpshow;

	if (ismsgline <= 0)
		return;
	tmpshow = showansi;
	showansi = 1;
	strcpy(buf, "[1;33;44m");
	if (chkmail())
		//% strcat(buf, "【[5;32m信[m[1;33;44m】");
		strcat(buf, "\xa1\xbe[5;32m\xd0\xc5[m[1;33;44m\xa1\xbf");
	else
		//% strcat(buf, "【  】");
		strcat(buf, "\xa1\xbe  \xa1\xbf");
	//% strcat(buf, "信箱 [[32m按[31mCtrl-Q[32m求救[33m] ");
	strcat(buf, "\xd0\xc5\xcf\xe4 [[32m\xb0\xb4[31mCtrl-Q[32m\xc7\xf3\xbe\xc8[33m] ");
	sprintf(
			buf2,
			"[[32m%s[33m][[32m%4.4d[33m,[32m%3.3d[33m][[32m%d[33m] [[32m%2s[33m]",
			//% insert_character ? "插入" : "改写", currln + 1, currpnt + 1,
			insert_character ? "\xb2\xe5\xc8\xeb" : "\xb8\xc4\xd0\xb4", currln + 1, currpnt + 1,
			//% linelen-1, enabledbchar ? "双" : "单");
			linelen-1, enabledbchar ? "\xcb\xab" : "\xb5\xa5");
	strcat(buf, buf2);
	//% sprintf(buf2, "\033[1;33m【\033[1;32m%.23s\033[33m】\033[m",
	sprintf(buf2, "\033[1;33m\xa1\xbe\033[1;32m%.23s\033[33m\xa1\xbf\033[m",
			format_time(fb_time(), TIME_FORMAT_ZH) + 6);
	strcat(buf, buf2);
	move(-1, 0);
	clrtoeol();
	prints("%s", buf);
	showansi = tmpshow;
}

static void display_buffer(void)
{
	register struct textline *p;
	register int i;
	int shift;
	int temp_showansi;
	temp_showansi = showansi;

	for (p = top_of_win, i = 0; i < t_lines - 1; i++) {
		move(i, 0);
		if (p != can_edit_end) {
			shift = (currpnt + 2 > STRLEN) ? (currpnt
					/ (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
			if (editansi) {
				showansi = 1;
				prints("%s", p->data);
			} else if ((p->attr & M_MARK)) {
				showansi = 1;
				clear_whole_line(i);
				cstrnput(p->data + shift);
			} else {
				if (p->len >= shift) {
					showansi = 0;
					strnput(p->data + shift);
				} else
					clrtoeol();
			}
			p = p->next;
		} else
			prints("%s~", editansi ? ANSI_RESET : "");
		clrtoeol();
	}
	showansi = temp_showansi;
	msgline();
	return;
}

void msg(int unused)
{
	int x, y;
	int tmpansi;

	tmpansi = showansi;
	showansi = 1;
	getyx(&x, &y);
	msgline();
	signal(SIGALRM, msg);
	move(x, y);
	refresh();
	alarm(60);
	showansi = tmpansi;
}

void indigestion(int i) {
	char badmsg[STRLEN];
	sprintf(badmsg, "SERIOUS INTERNAL INDIGESTION CLASS %d\n", i);
	report(badmsg, currentuser.userid);
}

struct textline *back_line(struct textline *pos, int num) {
	moveln = 0;
	while (num-- > 0)
		if (pos && pos != can_edit_begin) {
			pos = pos->prev;
			moveln++;
		} else
			break;
	return pos;
}

struct textline * forward_line(struct textline *pos, int num) {
	moveln = 0;
	while (num-- > 0)
		if (pos->next && pos->next != can_edit_end) {
			pos = pos->next;
			moveln++;
		} else
			break;
	return pos;
}

void countline() {
	struct textline *pos;

	pos = can_edit_begin;
	moveln = 0;
	while (pos != can_edit_end)
		if (pos->next) {
			pos = pos->next;
			moveln++;
		} else
			break;
}

int getlineno() {
	int cnt = 0;
	struct textline *p = currline;
	while (p != top_of_win) {
		if (p == NULL)
			break;
		cnt++;
		p = p->prev;
	}
	return cnt;
}
char * killsp(char *s) {
	while (*s == ' ')
		s++;
	return s;
}

struct textline * alloc_line() {
	register struct textline *p;
	p = (struct textline *) malloc(sizeof(*p));
	if (p == NULL) {
		indigestion(13);
		abort_bbs(0);
	}
	p->next = NULL;
	p->prev = NULL;
	p->data[0] = '\0';
	p->len = 0;
	p->attr = 0; /* for copy/paste */
	return p;
}
/*
 Appends p after line in list.  keeps up with last line as well.
 */

void goline(int n) {
	register struct textline *p = can_edit_begin;
	int count;
	if (n < 0)
		n = 1;
	if (n == 0)
		return;
	for (count = 1; count < n; count++) {
		if (p && p->next != can_edit_end) {
			p = p->next;
			continue;
		} else
			break;
	}
	if (count == n)
		count --;
	currln = count - 1;
	curr_window_line = 0;
	top_of_win = p;
	currline = p;
}

#ifdef ALLOWAUTOWRAP
void set()
{
	char tmp[8],theinfo[STRLEN];
	int templinelen;

	signal(SIGALRM,SIG_IGN);
	//% sprintf(theinfo,"自动换行: 每行最多字符数(10 -- %d)[%d]: ",
	sprintf(theinfo,"\xd7\xd4\xb6\xaf\xbb\xbb\xd0\xd0: \xc3\xbf\xd0\xd0\xd7\xee\xb6\xe0\xd7\xd6\xb7\xfb\xca\xfd(10 -- %d)[%d]: ",
			WRAPMARGIN-1,linelen-1);
	getdata(23,0,theinfo,tmp,7,DOECHO,YEA);
	msg(0);
	if( tmp[0] == '\0')return;
	templinelen = atoi(tmp) + 1;
	if ( templinelen < 11 ) templinelen = 11;
	else if( templinelen> WRAPMARGIN) templinelen = WRAPMARGIN;
	linelen = templinelen;
	return;
}
#endif

void go() {
	char tmp[8];
	int line;

	signal(SIGALRM, SIG_IGN);
	//% getdata(23, 0, "请问要跳到第几行: ", tmp, 7, DOECHO, YEA);
	getdata(23, 0, "\xc7\xeb\xce\xca\xd2\xaa\xcc\xf8\xb5\xbd\xb5\xda\xbc\xb8\xd0\xd0: ", tmp, 7, DOECHO, YEA);
	msg(0);
	if (tmp[0] == '\0')
		return;
	line = atoi(tmp);
	goline(line);
	return;
}

void searchline(text)
char text[STRLEN];
{
	int tmpline, addr, count = 0, tt;
	register struct textline *p = currline;

	tmpline = currln;
	for (;p && p != can_edit_end; p = p->next) {
		count++;
		if (count == 1) tt = currpnt;
		else tt = 0;
		if (strstr(p->data + tt, text)) {
			addr = (int) (strstr(p->data + tt, text) - p->data) + strlen(text);
			currpnt = addr;
			break;
		}
	}
	if (p && p!= can_edit_end) {
		currln = currln + count - 1;
		curr_window_line = 0;
		top_of_win = p;
		currline = p;
	} else {
		goline(currln + 1);
	}
}

void search() {
	char tmp[STRLEN];

	signal(SIGALRM, SIG_IGN);
	//% "搜寻字串: 
	getdata(-1, 0, "\xcb\xd1\xd1\xb0\xd7\xd6\xb4\xae: ", tmp, 65, DOECHO, YEA);
	msg(0);
	if (tmp[0] == '\0')
		return;
	else
		strcpy(searchtext, tmp);
	searchline(searchtext);
	return;
}

void append(p, line)
register struct textline *p, *line;
{
	if( p == NULL ) return;
	p->next = line->next;
	if (line == can_edit_end )
	can_edit_end = p;
	if (line->next)
	line->next->prev = p;
	line->next = p;
	p->prev = line;
}

/*
 delete_line deletes 'line' from the list and maintains the can_edit_end, and
 firstline pointers.
 */
void delete_line(line)
register struct textline *line;
{
	if ( line == NULL ) return;
	if (line == can_edit_begin && line->next == can_edit_end) {
		line->data[0] = '\0';
		line->len = 0;
		line->attr &= ~(M_MARK);
		mark_begin = mark_end = NULL;
		currpnt = 0;
		return;
	}

	if( mark_begin == line ) {
		if( line->next == can_edit_end) mark_begin = NULL;
		else mark_begin = line->next;
	}
	if( mark_end == line ) {
		if( line == can_edit_begin ) mark_end = NULL;
		else mark_end = line->prev;
	}

	if( line == can_edit_begin ) can_edit_begin = line->next;
	if( line == firstline ) {
		line->next->prev = NULL;
		firstline = line->next;
	} else {
		line->next->prev = line->prev;
		line->prev->next = line->next;
	}

	if (line) free(line);
}
/*
 split splits 'line' right before the character pos
 */
void split(register struct textline *line, register int pos) {
	register struct textline *p;
	register int i, patch;

	if (pos > line->len)
		return;
	if (line->data[pos-1] & 0x80) {
		for (patch = 0, i = pos - 1; i >= 0 && line->data[i] & 0x80; i--)
			patch ++;
		if (patch%2)
			pos --;
	}
#ifdef ALLOWAUTOWRAP
	if(pos && strchr("[0123456789;",line->data[pos-1])) {
		for(i=pos-1; i>0&&line->data[i]!='';i--)
		if(!strchr("[0123456789;",line->data[i])) break;
		if(line->data[i]=='') {
			if(pos - i < 20 ) pos = i;
			else pos --;
		}
	}
#endif
	p = alloc_line();
	p->len = line->len - pos;
	line->len = pos;
	strcpy(p->data, (line->data + pos));
	p->attr = line->attr; /* for copy/paste */
	*(line->data + pos) = '\0';
	append(p, line);
	if ((mark_end == NULL && line == mark_begin) || line == mark_end)
		mark_end = p;
	if (line == currline && pos <= currpnt) {
		currline = p;
		currpnt -= pos;
		curr_window_line++;
		currln++;
	}
	redraw_everything = YEA;
}
/*
 join connects 'line' and the next line.  It returns true if:

 1) lines were joined and one was deleted
 2) lines could not be joined
 3) next line is empty

 returns false if:

 1) Some of the joined line wrapped
 */

int join(register struct textline *line) {
	register int ovfl;

	if (!line->next || line->next == can_edit_end)
		return YEA;
	if ( (line->next->data[0]=='-')&&(line->next->data[1]='-')
			&&(line->next->len==2))
		return YEA; //added by Tim,ignore sinature
#ifdef ALLOWAUTOWRAP
	ovfl = line->len - num_ans_chr(line->data)
	+ line->next->len - num_ans_chr(line->next->data) - linelen;
#else
	ovfl = line->len + line->next->len - linelen;
#endif
	if (ovfl < 0) {
		strcat(line->data, line->next->data);
		line->len += line->next->len;
		if (line->next->attr & M_MARK)
			line->attr = line->next->attr;
		if (mark_begin == line->next)
			mark_begin = line;
		if (mark_end == line->next)
			mark_end = line;
		delete_line(line->next);
		return YEA;
	} else {
		register char *s;
		register struct textline *p = line->next;
#ifdef ALLOWAUTOWRAP
		s = p->data + seekthestr(p->data,p->len - ovfl - 1);
#else
		s = p->data + p->len - ovfl - 1;
#endif
		while (s != p->data && *s == ' ')
			s--;
		while (s != p->data && *s != ' ')
			s--;
#ifdef ALLOWAUTOWRAP
		if (s == p->data || linelen - (s-p->data)> 8)
#else
		if (s == p->data)
#endif
			return YEA;
		split(p, (s - p->data) + 1);
#ifdef ALLOWAUTOWRAP
		if( line->len - num_ans_chr(line->data)
				+ p->len - num_ans_chr(p->data) >= linelen) {
#else
		if (line->len + p->len >= linelen) {
#endif
			indigestion(0);
			return YEA;
		}
		join(line);
		p = line->next;
#ifdef ALLOWAUTOWRAP
		if( p->len >= 1 && p->len+1-num_ans_chr(p->data) < linelen ) {
#else
		if (p->len >= 1 && p->len+1 < linelen) {
#endif
			if (p->data[p->len - 1] != ' ') {
				strcat(p->data, " ");
				p->len++;
			}
		}
		return NA;
	}
}

void insert_char(register int ch) {
	register char *s;
	register int i;
#ifdef	ALLOWAUTOWRAP
	register int ansinum;
	register struct textline *p = currline, *savep;
#else
	register struct textline *p = currline;
#endif
	int wordwrap = YEA;
	if (currpnt > p->len) {
		indigestion(1);
		return;
	}
#ifdef	ALLOWAUTOWRAP
	savep = p;
	i = currln;
	for(ansinum = num_ans_chr( p->data );p->len - ansinum> linelen;) {
		ansinum = seekthestr(p->data, linelen);
		split(p, ansinum);
		p = p-> next;
		ansinum = num_ans_chr( p->data );
	}
	p = savep;
	currln = i;
#endif
	if (currpnt < p->len && !insert_character) {
		p->data[currpnt++] = ch;
	} else if (p->len < WRAPMARGIN) {
		for (i = p->len; i >= currpnt; i--)
			p->data[i + 1] = p->data[i];
		p->data[currpnt] = ch;
		p->len++;
		currpnt++;
	}
#ifdef	ALLOWAUTOWRAP
	ansinum = num_ans_chr(p->data);
	if( p->len-ansinum < linelen-1 && p->len < WRAPMARGIN)
#else
	if (p->len < linelen-1 && p->len < WRAPMARGIN)
#endif
		return;
#ifdef	ALLOWAUTOWRAP
	ansinum = seekthestr(p->data,p->len-1);
	s = p->data + ansinum;
#else
	s = p->data + p->len - 1;
#endif
	while (s != p->data && *s == ' ')
		s--;
	if (!(*s&0x80)) {
		while (s != p->data && *s != ' '&& !(*s&0x80))
			s--;
		if (*s!=' ')
			wordwrap = NA;
	} else
		wordwrap = NA;
	if (!wordwrap) {
#ifdef	ALLOWAUTOWRAP
		ansinum = seekthestr(p->data, p->len -2);
		s = p->data + ansinum;
#else
		s = p->data + p->len - 2;
#endif
	}
	split(p, (s - p->data) + 1);
	p = p->next;
	if (wordwrap && p->len >= 1) {//如果是词，且其后没有空格，则自动补上空格
		i = p->len;
		if (p->data[i - 1] != ' ') {
			p->data[i] = ' ';
			p->data[i + 1] = '\0';
			p->len++;
		}
	}
	while (!join(p)) {
		p = p->next;
		if (p == NULL) {
			indigestion(2);
			break;
		}
	}
}

void ve_insert_str(char *str) {
	while (*str)
		insert_char(*(str++));
}

void delete_char() {
	register int i;
	register int patch=0, currDEC=0;

	if (currline->len == 0)
		return;
	if (currpnt >= currline->len) {
		indigestion(1);
		return;
	}
	if (enabledbchar&&(currline->data[currpnt]&0x80)) {
		for (i=currpnt-1; i>=0&&currline->data[i]&0x80; i--)
			patch ++;
		if (patch%2==0&&currpnt<currline->len-1&&currline->data[currpnt+1]
				&0x80)
			patch = 1;
		else if (patch%2) {
			patch = 1;
			currDEC = 1;
		} else
			patch = 0;
	}
	if (currDEC)
		currpnt --;
	for (i = currpnt; i != currline->len; i++)
		currline->data[i] = currline->data[i + 1 + patch];
	currline->len--;
	if (patch)
		currline->len--;
}

static void insert_to_fp(FILE *fp)
{
	int ansi = 0;
	struct textline *p;

	for (p = can_edit_begin; p != can_edit_end; p = p->next)
		if (p->data[0]) {
			fprintf(fp, "%s\n", p->data);
			if (!ansi)
				if (strchr(p->data, '\033'))
					ansi++;
		}
	if (ansi)
		fprintf(fp, "%s\n", ANSI_RESET);
}

void insertch_from_fp(int ch)
{
	int backup_linelen = linelen;
	linelen = WRAPMARGIN;
	if (isprint2(ch) || ch == 27) {
		if (currpnt < 254)
			insert_char(ch);
		else if (currpnt < 255)
			insert_char('.');
	} else if (ch == Ctrl('I')) {
		do {
			insert_char(' ');
		} while (currpnt&0x03);
	} else if (ch == '\n') {
		split(currline, currpnt);
	}

	linelen = backup_linelen;
}

void insert_from_fp(FILE *fp)
{
	// TODO: should change to mmap_open.
	mmap_t m = {fileno(fp), O_RDONLY, FILE_RDLCK, PROT_READ, MAP_SHARED};
	BBS_TRY {
		if (mmap_open_fd(&m) == 0) {
			char *data = m.ptr;
			long not;
			for (not = 0; not < m.size; not++, data++) {
				insertch_from_fp(*data);
			}
		} else {
			BBS_RETURN_VOID;
		}
	}
	BBS_CATCH {
	}
	BBS_END mmap_close(&m);
}

static int process_ESC_action(int action, int arg)
/* valid action are I/E/S/B/F/R/C/O/= */
/* valid arg are    '0' - '7' */
{
	int newch = 0;
	char msg[80], buf[80];
	char filename[80];
	FILE *fp;

	msg[0] = '\0';
	switch (action) {
		case 'L':
			if (ismsgline >= 1) {
				ismsgline = 0;
				move(-1, 0);
				clrtoeol();
				refresh();
			} else
				ismsgline = 1;
			break;
		case 'M':
			process_MARK_action(arg, msg);
			break;
		case 'I':
			sprintf(filename, "home/%c/%s/clip_%c",
					toupper(currentuser.userid[0]), currentuser.userid,
					arg);
			if ((fp = fopen(filename, "r")) != NULL) {
				insert_from_fp(fp);
				fclose(fp);
				//% sprintf(msg, "已取出剪贴簿第 %c 页", arg);
				sprintf(msg, "\xd2\xd1\xc8\xa1\xb3\xf6\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda %c \xd2\xb3", arg);
			} else
				//% sprintf(msg, "无法取出剪贴簿第 %c 页", arg);
				sprintf(msg, "\xce\xde\xb7\xa8\xc8\xa1\xb3\xf6\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda %c \xd2\xb3", arg);
			break;
#ifdef ALLOWAUTOWRAP
			case 'X':
			set();
			break;
#endif
		case 'G':
			go();
			redraw_everything = YEA;
			break;
		case 'E':
			sprintf(filename, "home/%c/%s/clip_%c",
					toupper(currentuser.userid[0]), currentuser.userid,
					arg);
			if ((fp = fopen(filename, "w")) != NULL) {
				if (mark_on) {
					struct textline *p;
					for (p = mark_begin; p != can_edit_end; p = p->next) {
						if (p->attr & M_MARK)
							fprintf(fp, "%s\n", p->data);
						else
							break;
					}
				} else
					insert_to_fp(fp);
				fclose(fp);
				//% sprintf(msg, "已贴至剪贴簿第 %c 页", arg);
				sprintf(msg, "\xd2\xd1\xcc\xf9\xd6\xc1\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda %c \xd2\xb3", arg);
			} else
				//% sprintf(msg, "无法贴至剪贴簿第 %c 页", arg);
				sprintf(msg, "\xce\xde\xb7\xa8\xcc\xf9\xd6\xc1\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda %c \xd2\xb3", arg);
			break;
		case 'N':
			searchline(searchtext);
			redraw_everything = YEA;
			break;
		case 'S':
			search();
			redraw_everything = YEA;
			break;
		case 'F':
			sprintf(buf, "%c[3%cm", 27, arg);
			ve_insert_str(buf);
			break;
		case 'B':
			sprintf(buf, "%c[4%cm", 27, arg);
			ve_insert_str(buf);
			break;
		case 'R':
			ve_insert_str(ANSI_RESET);
			break;
		case 'C':
			editansi = showansi = 1;
			redraw_everything = YEA;
			clear();
			display_buffer();
			redoscr();
			//% strcpy(msg, "已显示彩色编辑成果，即将切回单色模式");
			strcpy(msg, "\xd2\xd1\xcf\xd4\xca\xbe\xb2\xca\xc9\xab\xb1\xe0\xbc\xad\xb3\xc9\xb9\xfb\xa3\xac\xbc\xb4\xbd\xab\xc7\xd0\xbb\xd8\xb5\xa5\xc9\xab\xc4\xa3\xca\xbd");
			break;
	}
	if (strchr("FBRCM", action))
		redraw_everything = YEA;
	if (msg[0] != '\0') {
		if (action == 'C') { /* need redraw */
			move(-2, 0);
			clrtoeol();
			//% prints("[1m%s%s%s[m", msg, ", 请按任意键返回编辑画面...", ANSI_RESET);
			prints("[1m%s%s%s[m", msg, ", \xc7\xeb\xb0\xb4\xc8\xce\xd2\xe2\xbc\xfc\xb7\xb5\xbb\xd8\xb1\xe0\xbc\xad\xbb\xad\xc3\xe6...", ANSI_RESET);
			igetkey();
			newch = '\0';
			editansi = showansi = 0;
			clear();
			display_buffer();
		} else
			//% newch = ask(strcat(msg, "，请继续编辑。"));
			newch = ask(strcat(msg, "\xa3\xac\xc7\xeb\xbc\xcc\xd0\xf8\xb1\xe0\xbc\xad\xa1\xa3"));
	} else
		newch = '\0';
	return newch;
}

void vedit_init(void)
{
	register struct textline *p = alloc_line();
	firstline = p;
	can_edit_begin = p;
	currline = p;
	currpnt = 0;
	process_ESC_action('M', '0');
	top_of_win = p;
	p = alloc_line();
	can_edit_begin->next = p;
	p->prev = can_edit_begin;
	can_edit_end = p;
	curr_window_line = 0;
	currln = 0;
	redraw_everything = NA;
	CLEAR_MARK()
	;
}

int read_file(char *filename) {
	FILE *fp;
	struct textline *p;
	char tmp[STRLEN];

	if (currline == NULL)
		vedit_init();
	if ((fp = fopen(filename, "r+")) == NULL) {
		if ((fp = fopen(filename, "w+")) != NULL) {
			fclose(fp);
			return 0;
		}
		indigestion(4);
		abort_bbs(0);
	}
	insert_from_fp(fp);
	fclose(fp);
	//% sprintf(tmp, ":·%s %s·", BoardName, BBSHOST);
	sprintf(tmp, ":\xa1\xa4%s %s\xa1\xa4", BoardName, BBSHOST);
	for (p = can_edit_end; p != can_edit_begin; p = p->prev) {
		//  if(strstr(p->data, tmp)) {
		if (strstr(p->data, tmp) && p->data[0]!=':') { //modified by roly 02.04.26
			can_edit_end = p;
			break;
		}
	}
	if (p != can_edit_end)
		return 1;
	return 0;
}

int in_mail;

void write_header(FILE *fp, const struct postheader *header)
{
	int noname;
	extern char BoardName[];
	extern char fromhost[];
	char uid[20];
	char uname[NAMELEN];

	strlcpy(uid, currentuser.userid, 20);
	uid[19] = '\0';
	if (in_mail)
		strlcpy(uname, currentuser.username, NAMELEN);
	else
		strlcpy(uname, currentuser.username, NAMELEN);
	uname[NAMELEN-1] = '\0';
	
	board_t board;
	get_board(currboard, &board);
	noname = board.flag & BOARD_ANONY_FLAG;
	if (in_mail)
		//% fprintf(fp, "寄信人: %s (%s)\n", uid, uname);
		fprintf(fp, "\xbc\xc4\xd0\xc5\xc8\xcb: %s (%s)\n", uid, uname);
	else {
		//% fprintf(fp, "发信人: %s (%s), 信区: %s\n",
		fprintf(fp, "\xb7\xa2\xd0\xc5\xc8\xcb: %s (%s), \xd0\xc5\xc7\xf8: %s\n",
				(noname && header->anonymous) ? ANONYMOUS_ACCOUNT : uid,
				(noname && header->anonymous) ? ANONYMOUS_NICK : uname, currboard);
	}
	//% fprintf(fp, "标  题: %s\n", header->title);
	fprintf(fp, "\xb1\xea  \xcc\xe2: %s\n", header->title);
	//% 发信站
	fprintf(fp, "\xb7\xa2\xd0\xc5\xd5\xbe: %s (%s)", BoardName,
			format_time(fb_time(), TIME_FORMAT_ZH));
	if (in_mail)
		//% fprintf(fp, "\n来  源: %s\n", mask_host(fromhost));
		fprintf(fp, "\n\xc0\xb4  \xd4\xb4: %s\n", mask_host(fromhost));
	else
		//% fprintf(fp, ", 站内信件\n");
		fprintf(fp, ", \xd5\xbe\xc4\xda\xd0\xc5\xbc\xfe\n");
	fprintf(fp, "\n");
}

#define KEEP_EDITING -2

void valid_article(char *pmt, char *abort, int sure) {
	struct textline *p = can_edit_begin;
	char ch;
	int total, lines, len, y;
	int w;

	w = NA;
	if (session.status == ST_POSTING || session.status == ST_EDIT) {
		total = lines = len = 0;
		while (p && p != can_edit_end) {
			ch = p->data[0];
			if (ch != ':' && ch != '>' && ch != '=' && !strstr(p->data,
					//% "的大作中提到: 】")) {
					"\xb5\xc4\xb4\xf3\xd7\xf7\xd6\xd0\xcc\xe1\xb5\xbd: \xa1\xbf")) {
				lines++;
				len += strlen(p->data);
			}
			total++;
			p = p->next;
		}
		y = 2;
		if (lines < total * 0.35) {
			move(y, 0);
			//% prints("注意：本篇文章的引言过长, 建议您删掉一些不必要的引言.\n");
			prints("\xd7\xa2\xd2\xe2\xa3\xba\xb1\xbe\xc6\xaa\xce\xc4\xd5\xc2\xb5\xc4\xd2\xfd\xd1\xd4\xb9\xfd\xb3\xa4, \xbd\xa8\xd2\xe9\xc4\xfa\xc9\xbe\xb5\xf4\xd2\xbb\xd0\xa9\xb2\xbb\xb1\xd8\xd2\xaa\xb5\xc4\xd2\xfd\xd1\xd4.\n");
			y += 3;
		}
		if (w) {
			//% strcpy(pmt, "E.再编辑, S.转信, L.本站发表, A.取消 or T.更改标题?[L]: ");
			strcpy(pmt, "E.\xd4\xd9\xb1\xe0\xbc\xad, S.\xd7\xaa\xd0\xc5, L.\xb1\xbe\xd5\xbe\xb7\xa2\xb1\xed, A.\xc8\xa1\xcf\xfb or T.\xb8\xfc\xb8\xc4\xb1\xea\xcc\xe2?[L]: ");
		}
	}
	if (sure) {
		abort[0] = 'L';
		return;
	}
	getdata(0, 0, pmt, abort, 3, DOECHO, YEA);
	if (w && abort[0] == '\0')
		abort[0] = 'L';
#if 0   
	switch (abort[0]) {
		case 'A':
		case 'a': /* abort */
		case 'E':
		case 'e': /* keep editing */
		return;
	}
#endif   
}

int write_file(char *filename, int write_header_to_file, int addfrom,
		int sure, struct postheader *header) {
	struct textline *p;
	FILE *fp;
	char abort[6], p_buf[100];
	int aborted = 0;

	signal(SIGALRM, SIG_IGN);
	clear();
	if (session.status == ST_POSTING) {
		//% strcpy(p_buf, "L.本站发表, S.转信, A.取消, T.更改标题 or E.再编辑? [L]: ");
		strcpy(p_buf, "L.\xb1\xbe\xd5\xbe\xb7\xa2\xb1\xed, S.\xd7\xaa\xd0\xc5, A.\xc8\xa1\xcf\xfb, T.\xb8\xfc\xb8\xc4\xb1\xea\xcc\xe2 or E.\xd4\xd9\xb1\xe0\xbc\xad? [L]: ");
	} else if (session.status == ST_SMAIL)
		//% strcpy(p_buf, "(S)寄出, (A)取消, or (E)再编辑? [S]: ");
		strcpy(p_buf, "(S)\xbc\xc4\xb3\xf6, (A)\xc8\xa1\xcf\xfb, or (E)\xd4\xd9\xb1\xe0\xbc\xad? [S]: ");
	else
		//% strcpy(p_buf, "(S)储存档案, (A)放弃编辑, (E)继续编辑? [S]: ");
		strcpy(p_buf, "(S)\xb4\xa2\xb4\xe6\xb5\xb5\xb0\xb8, (A)\xb7\xc5\xc6\xfa\xb1\xe0\xbc\xad, (E)\xbc\xcc\xd0\xf8\xb1\xe0\xbc\xad? [S]: ");
	valid_article(p_buf, abort, sure);
	if (abort[0] == 'a' || abort[0] == 'A') {
		struct stat stbuf;
		clear();
		//% prints("取消...\n");
		prints("\xc8\xa1\xcf\xfb...\n");
		refresh();
		sleep(1);
		if (stat(filename, &stbuf) || stbuf.st_size == 0)
			unlink(filename);
		aborted = -1;
	} else if (abort[0] == 'e' || abort[0] == 'E') {
		msg(0);
		return KEEP_EDITING;
	} else if ((abort[0] == 't' || abort[0] == 'T')
			&& session.status == ST_POSTING) {
		char buf[STRLEN];
		move(1, 0);
		//% prints("旧标题: %s", header->title);
		prints("\xbe\xc9\xb1\xea\xcc\xe2: %s", header->title);
		ansi_filter(buf, header->title);
		//% getdata(2, 0, "新标题: ", buf, 50, DOECHO, NA);
		getdata(2, 0, "\xd0\xc2\xb1\xea\xcc\xe2: ", buf, 50, DOECHO, NA);
		if (strcmp(header->title, buf) && strlen(buf) != 0) {
			check_title(buf, sizeof(buf));
			strlcpy(header->title, buf, STRLEN);
		}
	}
	if (aborted != -1) {
		if ((fp = fopen(filename, "w")) == NULL) {
			indigestion(5);
			abort_bbs(0);
		}
		if (write_header_to_file)
			write_header(fp, header);
	}
	p = can_edit_end;
	if (p!=NULL)
		for (; p->next!=NULL; p=p->next)
			;
	while (1) {
		struct textline *v = p->prev;
		if (p!=can_edit_begin &&(p->data[0] == '\0'|| !strcmp(p->data,
				"\n"))) {
			free(p);
			v->next = NULL;
			p = v;
		} else {
			can_edit_end = p;
			p = firstline;
			break;
		}
	}
	firstline = NULL;
	//Modified by IAMFAT 2002.06.11 add *[m
	while (p != NULL) {
		struct textline *v = p->next;
		if (aborted != -1) {
			//% if (ADD_EDITMARK && strncmp(p->data, "[m[1;36m※ 修改:·", 17))
			if (ADD_EDITMARK && strncmp(p->data, "[m[1;36m\xa1\xf9 \xd0\xde\xb8\xc4:\xa1\xa4", 17))
				fprintf(fp, "%s\n", p->data);
		}
		free(p);
		p = v;
	}
	//added by iamfat 2002.08.17
	extern char fromhost[];
	if (aborted != -1 && session.status == ST_EDIT && ADD_EDITMARK) {
		//% fprintf(fp, "\033[m\033[1;36m※ 修改:·%s 于 %22.22s·[FROM: %s]"
		fprintf(fp, "\033[m\033[1;36m\xa1\xf9 \xd0\xde\xb8\xc4:\xa1\xa4%s \xd3\xda %22.22s\xa1\xa4[FROM: %s]"
				"\033[m\n", currentuser.userid,
				format_time(fb_time(), TIME_FORMAT_ZH), mask_host(fromhost));
	}
	//added end
	if ((session.status == ST_POSTING || session.status == ST_SMAIL
				|| session.status == ST_EDIT)
			&& addfrom != 0 && aborted != -1) {
		char fname[STRLEN];

		bool anony = (currbp->flag & BOARD_ANONY_FLAG) && (header->anonymous);
		int color = (currentuser.numlogins % 7) + 31;
		setuserfile(fname, "signatures");
		if (!dashf(fname) || currentuser.signature == 0 || anony)
			fputs("--\n", fp);
		//% fprintf(fp, "[m[1;%2dm※ 来源:·%s %s·[FROM: %s][m\n", color,
		fprintf(fp, "[m[1;%2dm\xa1\xf9 \xc0\xb4\xd4\xb4:\xa1\xa4%s %s\xa1\xa4[FROM: %s][m\n", color,
				BoardName, BBSHOST, (anony) ? ANONYMOUS_SOURCE : mask_host(fromhost));
	}
	if (aborted != -1)
		fclose(fp);
	currline = NULL;
	can_edit_end = NULL;
	firstline = NULL;
	can_edit_begin = NULL;
	can_edit_end = NULL;
	return aborted;
}

void keep_fail_post(void)
{
	char filename[STRLEN];
	struct textline *p = firstline;
	FILE *fp;

	sprintf(filename, "home/%c/%s/%s.deadve",
			toupper(currentuser.userid[0]), currentuser.userid,
			currentuser.userid);
	if ((fp = fopen(filename, "w")) == NULL) {
		indigestion(5);
		return;
	}
	while (p != NULL) {
		struct textline *v = p->next;
		if (p->next != NULL || p->data[0] != '\0')
			fprintf(fp, "%s\n", p->data);
		free(p);
		p = v;
	}
	return;
}

/* mark_block() 
 0	mark_begin == NULL; mark_end == NULL;
 1	mark_begin != NULL; mark_end == NULL;
 2	mark_begin != NULL; mark_end != NULL;
 */

int mark_block(void)
{
	struct textline *p;
	int pass_mark = 0;

	for (p = can_edit_begin; p && p != can_edit_end; p = p->next)
		p->attr &= ~(M_MARK);

	if (mark_begin == NULL && mark_end == NULL)
		return 0;
	if (mark_begin == mark_end) {
		mark_begin->attr |= M_MARK;
		return 2;
	}
	if (mark_begin == NULL) {
		mark_end->attr |= M_MARK;
		return 1;
	}
	if (mark_end == NULL) {
		mark_begin->attr |= M_MARK;
		return 1;
	}
	for (p = can_edit_begin; p != can_edit_end; p = p->next) {
		if (p == mark_begin || p == mark_end) {
			pass_mark++;
			p->attr |= M_MARK;
			continue;
		}
		if (pass_mark == 1)
			p->attr |= M_MARK;
		else
			p->attr &= ~(M_MARK);
	}
	return 2;
}

int vedit_process_ESC(int arg) /* ESC + x */
{
	int ch2, action;
//% "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用彩色 (F/B/R)前景/背景/还原色彩"
#define WHICH_ACTION_COLOR    \
"(M)\xc7\xf8\xbf\xe9\xb4\xa6\xc0\xed (I/E)\xb6\xc1\xc8\xa1/\xd0\xb4\xc8\xeb\xbc\xf4\xcc\xf9\xb2\xbe (C)\xca\xb9\xd3\xc3\xb2\xca\xc9\xab (F/B/R)\xc7\xb0\xbe\xb0/\xb1\xb3\xbe\xb0/\xbb\xb9\xd4\xad\xc9\xab\xb2\xca"
//% "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用单色 (F/B/R)前景/背景/还原色彩"
#define WHICH_ACTION_MONO    \
"(M)\xc7\xf8\xbf\xe9\xb4\xa6\xc0\xed (I/E)\xb6\xc1\xc8\xa1/\xd0\xb4\xc8\xeb\xbc\xf4\xcc\xf9\xb2\xbe (C)\xca\xb9\xd3\xc3\xb5\xa5\xc9\xab (F/B/R)\xc7\xb0\xbe\xb0/\xb1\xb3\xbe\xb0/\xbb\xb9\xd4\xad\xc9\xab\xb2\xca"

//% #define CHOOSE_MARK    "(0)取消标记 (1)设定区块标记 (2)复制标记内容 (3)删除区块"
#define CHOOSE_MARK    "(0)\xc8\xa1\xcf\xfb\xb1\xea\xbc\xc7 (1)\xc9\xe8\xb6\xa8\xc7\xf8\xbf\xe9\xb1\xea\xbc\xc7 (2)\xb8\xb4\xd6\xc6\xb1\xea\xbc\xc7\xc4\xda\xc8\xdd (3)\xc9\xbe\xb3\xfd\xc7\xf8\xbf\xe9"
//% #define FROM_WHICH_PAGE "读取剪贴簿第几页? (0-7) [预设为 0]"
#define FROM_WHICH_PAGE "\xb6\xc1\xc8\xa1\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda\xbc\xb8\xd2\xb3? (0-7) [\xd4\xa4\xc9\xe8\xce\xaa 0]"
//% #define SAVE_ALL_TO     "把整篇文章写入剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_ALL_TO     "\xb0\xd1\xd5\xfb\xc6\xaa\xce\xc4\xd5\xc2\xd0\xb4\xc8\xeb\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda\xbc\xb8\xd2\xb3? (0-7) [\xd4\xa4\xc9\xe8\xce\xaa 0]"
//% #define SAVE_PART_TO    "把区块内容写入剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_PART_TO    "\xb0\xd1\xc7\xf8\xbf\xe9\xc4\xda\xc8\xdd\xd0\xb4\xc8\xeb\xbc\xf4\xcc\xf9\xb2\xbe\xb5\xda\xbc\xb8\xd2\xb3? (0-7) [\xd4\xa4\xc9\xe8\xce\xaa 0]"
//% #define FROM_WHICH_SIG  "取出签名簿第几页? (0-7) [预设为 0]"
#define FROM_WHICH_SIG  "\xc8\xa1\xb3\xf6\xc7\xa9\xc3\xfb\xb2\xbe\xb5\xda\xbc\xb8\xd2\xb3? (0-7) [\xd4\xa4\xc9\xe8\xce\xaa 0]"
//% #define CHOOSE_FG     "前景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_FG     "\xc7\xb0\xbe\xb0\xd1\xd5\xc9\xab? 0)\xba\xda 1)\xba\xec 2)\xc2\xcc 3)\xbb\xc6 4)\xc9\xee\xc0\xb6 5)\xb7\xdb\xba\xec 6)\xc7\xb3\xc0\xb6 7)\xb0\xd7 "
//% #define CHOOSE_BG     "背景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_BG     "\xb1\xb3\xbe\xb0\xd1\xd5\xc9\xab? 0)\xba\xda 1)\xba\xec 2)\xc2\xcc 3)\xbb\xc6 4)\xc9\xee\xc0\xb6 5)\xb7\xdb\xba\xec 6)\xc7\xb3\xc0\xb6 7)\xb0\xd7 "
//% #define CHOOSE_ERROR    "选项错误"
#define CHOOSE_ERROR    "\xd1\xa1\xcf\xee\xb4\xed\xce\xf3"

	switch (arg) {
		case 'M':
		case 'm':
			ch2 = ask(CHOOSE_MARK);
			action = 'M';
			break;
		case 'I':
		case 'i': /* import */
			ch2 = ask(FROM_WHICH_PAGE);
			action = 'I';
			break;
		case 'E':
		case 'e': /* export */
			mark_on = mark_block();
			ch2 = ask(mark_on ? SAVE_PART_TO : SAVE_ALL_TO);
			action = 'E';
			break;
		case 'S':
		case 's': /* 搜索字符串 */
			ch2 = '0';
			action = 'S';
			break;
		case 'F':
		case 'f':
			ch2 = ask(CHOOSE_FG);
			action = 'F';
			break;
		case 'X':
		case 'x':
			ch2 = '0';
			action = 'X';
			break;
		case 'B':
		case 'b':
			ch2 = ask(CHOOSE_BG);
			action = 'B';
			break;
		case 'R':
		case 'r':
			ch2 = '0';
			action = 'R';
			break;
		case 'D':
		case 'd':
			ch2 = '3';
			action = 'M';
			break;
		case 'N':
		case 'n':
			ch2 = '0';
			action = 'N';
			break;
		case 'G':
		case 'g':
			ch2 = '1';
			action = 'G';
			break;
		case 'L':
		case 'l':
			ch2 = '0'; /* not used */
			action = 'L';
			break;
		case 'C':
		case 'c':
			ch2 = '0'; /* not used */
			action = 'C';
			break;
		case 'Q':
		case 'q':
			ch2 = '0'; /* not used */
			action = 'M';
			break;
		default:
			return '\0';
	}
	if (strchr("IES", action) && (ch2 == '\n' || ch2 == '\r'))
		ch2 = '0';
	if (ch2 >= '0' && ch2 <= '7')
		return process_ESC_action(action, ch2);
	else {
		return ask(CHOOSE_ERROR);
	}
}

void process_MARK_action(int arg, char *msg) {
	struct textline *p;

	switch (arg) {
		case '0': /* cancel */
			for (p = can_edit_begin; p && p != can_edit_end; p = p->next)
				p->attr &= ~(M_MARK);
			CLEAR_MARK()
			;
			break;
		case '1':
			if (mark_begin)
				mark_end = currline;
			else
				mark_begin = currline;
			mark_on = mark_block();
			//if( mark_on == 2) strcpy(msg, "区块标记已设定完成"); 
			//else strcpy(msg, "已设定开头标记, 尚无结尾标记");
			break;
		case '2': /* copy mark */
			mark_on = mark_block();
			if (mark_on && !(currline->attr & M_MARK)) {
				for (p = mark_begin; p != can_edit_end; p = p->next) {
					if (p->attr & M_MARK) {
						ve_insert_str(p->data);
						split(currline, currpnt);
					} else
						break;
				}
				//strcpy(msg, "区块标记已经粘贴到光标处");
				break;
			} else if (mark_on)
				//% strcpy(msg, "光标在区块外才可复制");
				strcpy(msg, "\xb9\xe2\xb1\xea\xd4\xda\xc7\xf8\xbf\xe9\xcd\xe2\xb2\xc5\xbf\xc9\xb8\xb4\xd6\xc6");
			else
				//% strcpy(msg, "您尚未用 Ctrl+U 设置区块");
				strcpy(msg, "\xc4\xfa\xc9\xd0\xce\xb4\xd3\xc3 Ctrl+U \xc9\xe8\xd6\xc3\xc7\xf8\xbf\xe9");
			bell();
			break;
		case '3': /* delete mark */
			mark_on = mark_block();
			if (mark_on == 0)
				break;
			for (p = can_edit_begin; p != can_edit_end;) {
				if (p->attr & M_MARK) {
					delete_line(p);
					p = can_edit_begin;
				} else
					p = p->next;
			}
			CLEAR_MARK()
			goline(1);
			break;
		default:
			strcpy(msg, CHOOSE_ERROR);
	}
}

void vedit_key(int ch) {
	int i, patch;
#define NO_ANSI_MODIFY  if(no_touch) { warn++; break; }

	static int lastindent = -1;
	int no_touch, warn, shift;

	if (ch == Ctrl('P')||ch == KEY_UP||ch == Ctrl('N')||ch == KEY_DOWN) {
		if (lastindent == -1)
			lastindent = currpnt;
	} else
		lastindent = -1;
	no_touch = (editansi && strchr(currline->data, '\033')) ? 1 : 0;
	warn = 0;
	if (ch < 0x100 && isprint2(ch)) {
		if (no_touch)
			warn++;
		else
			insert_char(ch);
	} else {
		switch (ch) {
			case Ctrl('I'): //tab键
				NO_ANSI_MODIFY
				;
				{
					register int count=4;
					while (count--) {
						insert_char(' ');
						if (!(currpnt&0x03))
							break;
					}
				}
				break;
			case '\r':
			case '\n':
				NO_ANSI_MODIFY
				;
				split(currline, currpnt);
				break;
			case Ctrl('R'):
				enabledbchar = !enabledbchar;
				break;
			case Ctrl('G'):/* redraw screen */
				clear();
				redraw_everything = YEA;
				break;
			case Ctrl('Q'):/* call help screen */
				show_help("help/edithelp");
				redraw_everything = YEA;
				break;
			case KEY_LEFT: /* backward character */
				if (currpnt > 0) {
					currpnt--;
				} else if (currline != can_edit_begin && currline->prev) {
					curr_window_line--;
					currln--;
					currline = currline->prev;
					currpnt = currline->len;
				}
				break;
			case Ctrl('C'):
				process_ESC_action('M', '2');
				break;
			case Ctrl('U'):
				process_ESC_action('M', '1');
				clear();
				break;
			case Ctrl('V'):
			case KEY_RIGHT:/* forward character */
				if (currline->len != currpnt) {
					currpnt++;
				} else if (currline->next && currline->next
						!= can_edit_end) {
					currpnt = 0;
					curr_window_line++;
					currln++;
					currline = currline->next;
				}
				break;
			case Ctrl('P'):
			case KEY_UP: /* Previous line */
				if (currline != can_edit_begin) {
					currln--;
					curr_window_line--;
					currline = currline->prev;
					currpnt=(currline->len>lastindent) ? lastindent
							: currline->len;
				}
				break;
			case Ctrl('N'):
			case KEY_DOWN: /* Next line */
				if (currline->next != can_edit_end) {
					currline = currline->next;
					curr_window_line++;
					currln++;
					currpnt=(currline->len>lastindent) ? lastindent
							: currline->len;
				}
				break;
			case Ctrl('B'):
			case KEY_PGUP: /* previous page */
				top_of_win = back_line(top_of_win, t_lines - 2);
				currline = back_line(currline, t_lines - 2);
				currln -= moveln;
				curr_window_line = getlineno();
				if (currpnt > currline->len)
					currpnt = currline->len;
				redraw_everything = YEA;
				break;
			case Ctrl('F'):
			case KEY_PGDN: /* next page */
				top_of_win = forward_line(top_of_win, 22);
				currline = forward_line(currline, 22);
				currln += moveln;
				curr_window_line = getlineno();
				if (currpnt > currline->len)
					currpnt = currline->len;
				redraw_everything = YEA;
				break;
			case Ctrl('A'):
			case KEY_HOME: /* begin of line */
				currpnt = 0;
				break;
			case Ctrl('E'):
			case KEY_END: /* end of line */
				currpnt = currline->len;
				break;
			case Ctrl('S'):/* start of file */
				top_of_win = can_edit_begin;
				currline = top_of_win;
				currpnt = 0;
				curr_window_line = 0;
				currln = 0;
				redraw_everything = YEA;
				break;
			case Ctrl('T'):/* tail of file */
				top_of_win = back_line(can_edit_end->prev, 22);
				countline();
				currln = moveln;
				currline = can_edit_end->prev;
				curr_window_line = getlineno();
				currpnt = 0;
				redraw_everything = YEA;
				break;
			case Ctrl('O'):
			case KEY_INS: /* Toggle insert/overwrite */
				insert_character = !insert_character;
				break;
			case Ctrl('H'):
			case KEY_BACKSPACE: /* backspace */
				NO_ANSI_MODIFY
				;
				if (currpnt == 0) {
					struct textline *p;
					if (currline == can_edit_begin)
						break;
					currln--;
					currline = currline->prev;
					currpnt = currline->len;
					curr_window_line--;
					if (curr_window_line < 0)
						top_of_win = top_of_win->next;
					if (*killsp(currline->next->data) == '\0') {
						delete_line(currline->next);
						redraw_everything = YEA;
						break;
					}
					p = currline;
					while (!join(p)) {
						p = p->next;
						if (p == NULL) {
							indigestion(2);
							abort_bbs(0);
						}
					}
					redraw_everything = YEA;
					break;
				}
				currpnt--;
				delete_char();
				break;
			case Ctrl('D'):
			case KEY_DEL: /* delete current character */
				NO_ANSI_MODIFY
				;
				if (currline->len == currpnt) {
					vedit_key(Ctrl('K'));
					break;
				}
				delete_char();
				break;
			case Ctrl('Y'):/* delete current line */
				no_touch = 0; /* ANSI_MODIFY hack */
				currpnt = 0;
				currline->len = 0;
				vedit_key(Ctrl('K'));
				break;
			case Ctrl('K'):/* delete to end of line */
				NO_ANSI_MODIFY
				;
				if (currline->next == can_edit_end) {
					if (currline->len > currpnt) {
						currline->len = currpnt;
					} else if (currpnt != 0)
						currline->len = currpnt = 0;;
					currline->data[currpnt] = '\0';
					break;
				}
				if (currline->len == 0) {
					struct textline *p = currline->next;
					if (currline == top_of_win)
						top_of_win = p;
					delete_line(currline);
					currline = p;
					redraw_everything = YEA;
					break;
				}
				if (currline->len == currpnt) {
					struct textline *p = currline;
					while (!join(p)) {
						p = p->next;
						if (p == NULL) {
							indigestion(2);
							abort_bbs(0);
						}
					}
					redraw_everything = YEA;
					break;
				}
				for (patch=0, i=currpnt-1; i>=0&&currline->data[i]&0x80; i--)
					patch ++;
				if (patch%2)
					currpnt --;
				currline->len = currpnt;
				currline->data[currpnt] = '\0';
				break;
			default:
				break;
		}
	}
	if (curr_window_line < 0) {
		curr_window_line = 0;
		if (top_of_win!=can_edit_begin) {
			top_of_win = top_of_win->prev;
			redraw_everything = YEA;
		}
	}
	if (curr_window_line >= t_lines - 1) {
		for (i = curr_window_line - t_lines + 1; i >= 0; i--) {
			curr_window_line--;
			if (top_of_win->next != can_edit_end) {
				top_of_win = top_of_win->next;
				//Modified by IAMFAT 2002-05-26
				redraw_everything = YEA;
				//scroll();
			}
		}
	}
	if (editansi)
		redraw_everything = YEA;
	shift = (currpnt + 2 > STRLEN) ? (currpnt / (STRLEN - scrollen))
			* (STRLEN - scrollen) : 0;
	msgline();
	if (shifttmp != shift || redraw_everything == YEA) {
		redraw_everything = YEA;
		shifttmp = shift;
	} else
		redraw_everything = NA;

	move(curr_window_line, 0);
	if (currline->attr & M_MARK) {
		showansi = 1;
		cstrnput(currline->data + shift);
		showansi = 0;
	} else {
		strnput(currline->data + shift);
		clrtoeol();
	}
}

int raw_vedit(char *filename, int write_header_to_file, int modifyheader,
		struct postheader *header)
{
	int newch, ch = 0, foo, shift, sure = 0;
	int addfrom = 0;
	addfrom = read_file(filename);
	if (!modifyheader) {
		top_of_win = firstline;
		//modified by iamfat 2002.08.18 不应该是跳过4行 应该是发现首个'\n'
		/*
		 while(top_of_win->next != can_edit_end && ch < 4 ){
		 top_of_win  = top_of_win->next;
		 ch ++;
		 }
		 ch = 0;
		 */
		while (top_of_win->next != can_edit_end && top_of_win->len>0) {
			top_of_win=top_of_win->next;
		}
		if (top_of_win->next!=can_edit_end)
			top_of_win = top_of_win->next;
		//modified end.
		can_edit_begin = top_of_win;
	}
	currline = top_of_win;
	curr_window_line = 0;
	currln = 0;
	currpnt = 0;
	clear();
	display_buffer();
	msgline();
	while (ch != EOF) {
		newch = '\0';
		switch (ch) {
			case Ctrl('X'):
				sure = 1;
			case Ctrl('W'):
				foo = write_file(filename, write_header_to_file, addfrom,
						sure, header);
				if (foo != KEEP_EDITING)
					return foo;
				redraw_everything = YEA;
				break;
			case KEY_ESC:
				if (KEY_ESC_arg == KEY_ESC)
					insert_char(KEY_ESC);
				else {
					newch = vedit_process_ESC(KEY_ESC_arg);
					clear();
				}
				redraw_everything = YEA;
				break;
			default:
				vedit_key(ch);
		}
		if (redraw_everything)
			display_buffer();
		redraw_everything = NA;
		shift = (currpnt + 2 > STRLEN) ? (currpnt / (STRLEN - scrollen))
				* (STRLEN - scrollen) : 0;
		move(curr_window_line, currpnt - shift);
		ch = (newch != '\0') ? newch : igetkey();
	}
	return 1;
}

int vedit(char *filename, int write_header_to_file, int modifyheader,
		struct postheader *header)
{
	int ans, t = showansi;
	showansi = 0;
#ifdef ALLOWAUTOWRAP
	if (DEFINE(DEF_AUTOWRAP)
			&& (session.status == ST_POSTING
				|| session.status == ST_SMAIL
				|| session.status == ST_EDIT)
			) {
		linelen = 79;
	} else
#endif
	{
		linelen = WRAPMARGIN;
	}
	ismsgline = (DEFINE(DEF_EDITMSG)) ? 1 : 0;
	msg(0);
	ans = raw_vedit(filename, write_header_to_file, modifyheader, header);
	showansi = t;
	signal(SIGALRM, SIG_IGN);
	return ans;
}
