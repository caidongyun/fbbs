#include "bbs.h"
#include "list.h"

#define BBS_PAGESIZE (t_lines - 4)


typedef struct {
	comparator_t cmp;         ///< Sorting method.
	struct user_info **users; ///< Array of online users.
	int *ovrs;                ///< Array of online overriding users.
	int num;                  ///< Number of online users.
	int onum;                 ///< Number of online overriding users.
	bool ovr_only;            ///< True if show overriding users only.
	bool board;               ///< True if show users in current board only.
	bool show_note;           ///< True if show note to overriding users.
} online_users_t;

static int online_users_sort_userid(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return strcasecmp((*u1)->userid, (*u2)->userid);
}

static int online_users_sort_nick(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return strcasecmp((*u1)->username, (*u2)->username);
}

static int online_users_sort_from(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	// TODO: from[22] is used..
	return strncmp((*u1)->from, (*u2)->from, 20);
}

static int online_users_sort_status(const void *usr1, const void *usr2)
{
	struct user_info * const *u1 = usr1;
	struct user_info * const *u2 = usr2;
	return (*u1)->mode - (*u2)->mode;
}

static int online_users_init(online_users_t *up)
{
	up->users = malloc(sizeof(*up->users) * USHM_SIZE);
	if (!up->users)
		return -1;
	up->ovrs = malloc(sizeof(*up->ovrs) * USHM_SIZE);
	if (!up->ovrs) {
		free(up->users);
		return -1;
	}

	up->cmp = online_users_sort_userid;
	return 0;
}

static void online_users_swap(online_users_t *up, int a, int b)
{
	struct user_info *tmp = up->users[a];
	up->users[a] = up->users[b];
	up->users[b] = tmp;
}

static int online_users_load(choose_t *cp)
{
	online_users_t *up = cp->data;

	resolve_utmp();

	up->num = 0;
	up->onum = 0;

	struct user_info *uin;
	int i;
	for (i = 0; i < USHM_SIZE; i++) {
		uin = utmpshm->uinfo + i;
		if (!uin->active || !uin->pid)
			continue;
		if (up->board && uin->currbrdnum != uinfo.currbrdnum)
			continue;
		if (uin->invisible && uin->uid != usernum && (!HAS_PERM(PERM_SEECLOAK)))
			continue;
		if (myfriend(uin->uid))
			up->ovrs[up->onum++] = up->num;
		up->users[up->num++] = uin;
	}

	// Extract overriding users
	int n = 0;
	for (i = 0; i < up->num && n < up->onum; ++i) {
		if (up->ovrs[n] == i) {
			if (i != n)
				online_users_swap(up, n, i);
			n++;
		}
	}

	qsort(up->users, up->onum, sizeof(*up->users), up->cmp);
	qsort(up->users + up->onum, up->num - up->onum, sizeof(*up->users), up->cmp);

	cp->all = up->num;
	return cp->all;
}

static void online_users_title(choose_t *cp)
{
	online_users_t *up = cp->data;
	docmdtitle(up->ovr_only ? "[�������б�]" : "[ʹ�����б�]",
			" ����[\033[1;32mt\033[m] ����[\033[1;32mm\033[m] ��ѶϢ["
			"\033[1;32ms\033[m] ��,������[\033[1;32mo\033[m,\033[1;32md\033[m]"
			" ��˵����[\033[1;32m��\033[m,\033[1;32mRtn\033[m] �л�ģʽ "
			"[\033[1;32mf\033[m] ���[\033[1;32mh\033[m]");

}

static void get_override_note(const char *userid, char *buf, size_t size)
{
	struct override tmp;
	memset(&tmp, 0, sizeof(tmp));

	buf[0] = '\0';
	char file[HOMELEN];
	sethomefile(file, currentuser.userid, "friends");
	if (search_record(buf, &tmp, sizeof(tmp), cmpfnames, userid))
		strlcpy(buf, tmp.exp, size);
}

static int online_users_display(choose_t *cp)
{
	online_users_t *up = cp->data;

	struct user_info *uin;
	int i;
	bool is_ovr;
	char buf[STRLEN], line[256];
	int limit = up->ovr_only ? up->onum : up->num;
	if (limit > cp->start + BBS_PAGESIZE)
		limit = cp->start + BBS_PAGESIZE;
	for (i = cp->start; i < limit; ++i) {
		is_ovr = up->ovr_only || i < up->onum;
		uin = up->users + i;
		if (!uin || !uin->active || !uin->pid || *uin->userid == '\0') {
			outs("\033[1;44m��..�Ҹ���\033[m\n");
			continue;
		}

		if (is_ovr && up->show_note)
			get_override_note(uin->userid, buf, sizeof(buf));
		else
			strlcpy(buf, uin->username, sizeof(buf));
		ellipsis(buf, 20);

		const char *host;
		if (HAS_PERM2(PERM_OCHAT, &currentuser)) {
			host = uin->from;
		} else {
			if (uin->from[22] == 'H')
				host = "......";
			else
				host = mask_host(uin->from);
		}

		char pager;
		if (uin->mode == FIVE || uin->mode == BBSNET || uin->mode == LOCKSCREEN)
			pager = '@';
		else
			pager = pagerchar(hisfriend(uin), uin->pager);

		const char *color;
		if (uin->invisible)
			color = "\033[1;30m";
		else if (is_web_user(uin->mode))
			color = "\033[36m";
		else if (uin->mode == POSTING || uin->mode == MARKET)
			color = "\033[32m";
		else if (uin->mode == FIVE || uin->mode == BBSNET)
			color = "\033[33m";
		else
			color = "";

		snprintf(line, sizeof(line), " \033[m%4d%s%-12.12s\033[37m %-20.20s"
				"\033[m %-15.15s %c %c %c %s%-10.10s\033[37m %5.5s\033[m\n",
				cp->start + i + 1, is_ovr ? "\033[32m��" : "  ", uin->userid,
				buf, host, pager, msgchar(uin), uin->invisible ? '@' : ' ',
				color, mode_type(uin->mode), idle_str(uin));
		prints("%s", line);
	}
	return 0;
}

static int online_users_handler(choose_t *cp, int ch)
{
	return 0;
}

int online_users(online_users_t *op)
{
	choose_t cs;
	cs.data = op;
	cs.loader = online_users_load;
	cs.title = online_users_title;
	cs.display = online_users_display;
	cs.handler = online_users_handler;
	
	choose2(&cs);

	free(op->users);
	free(op->ovrs);
	return 0;
}

int online_users_show(void)
{
	online_users_t ou;
	if (online_users_init(&ou) < 0)
		return -1;
	ou.ovr_only = false;
	modify_user_mode(LUSERS);
	return online_users(&ou);
}