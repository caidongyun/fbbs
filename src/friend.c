#include "bbs.h"
#include "fbbs/convert.h"
#include "fbbs/fbbs.h"
#include "fbbs/friend.h"
#include "fbbs/mail.h"
#include "fbbs/session.h"
#include "fbbs/string.h"
#include "fbbs/terminal.h"
#include "fbbs/tui_list.h"

static tui_list_loader_t following_list_loader(tui_list_t *p)
{
	if (p->data)
		following_list_free(p->data);
	p->data = following_list_load(session.uid);
	return (p->all = following_list_rows(p->data));
}

static tui_list_title_t following_list_title(tui_list_t *p)
{
	const char *middle = chkmail() ? "[�����ż�]" : BBSNAME;
	showtitle("[�༭��ע����]", middle);
	prints(" [\033[1;32m��\033[m,\033[1;32me\033[m] �뿪"
			" [\033[1;32mh\033[m] ����"
			" [\033[1;32m��\033[m,\033[1;32mRtn\033[m] ˵����"
			" [\033[1;32m��\033[m,\033[1;32m��\033[m] ѡ��"
			" [\033[1;32ma\033[m] �ӹ�ע"
			" [\033[1;32md\033[m] ȡ����ע\n"
			"\033[1;44m ���  ��ע����     �� ��ע\033[K\033[m\n");
}

static tui_list_display_t following_list_display(tui_list_t *p, int i)
{
	following_list_t *list = p->data;

	char gbk_note[FOLLOW_NOTE_CCHARS * 2 + 1];
	convert_u2g(following_list_get_notes(list, i), gbk_note);

	prints(" %4d  %-12s %s %s\n", i + 1, following_list_get_name(list, i),
			following_list_get_is_friend(list, i) ? "��" : "  ", gbk_note);
	return 0;
}

static int tui_follow(void)
{
	char buf[IDLEN + 1];
	getdata(t_lines - 1, 0, "������Ҫ��ע����: ", buf, IDLEN, DOECHO, YEA);
	if (!*buf)
		return 0;
	char note[FOLLOW_NOTE_CCHARS * 2 + 1], utf8_note[FOLLOW_NOTE_CCHARS * 4 + 1];
	getdata(t_lines - 1, 0, "�����뱸ע: ", note, sizeof(note), DOECHO, YEA);
	convert_g2u(note, utf8_note);
	return follow(session.uid, buf, utf8_note);
}

static int tui_unfollow(user_id_t uid)
{
	move(t_lines - 1, 0);
	return askyn("ȷ��ȡ����ע?", false, true) ? unfollow(session.uid, uid) : 0;
}

static int tui_edit_followed_note(user_id_t followed, const char *orig)
{
	char note[FOLLOW_NOTE_CCHARS * 2 + 1], utf8_note[FOLLOW_NOTE_CCHARS * 4 + 1];
	getdata(t_lines - 1, 0, "�����뱸ע: ", note, sizeof(note), DOECHO, YEA);
	convert_g2u(note, utf8_note);

	if (!*utf8_note || streq(orig, utf8_note))
		return DONOTHING;

	edit_followed_note(session.uid, followed, utf8_note);
	return FULLUPDATE;
}

static tui_list_query_t following_list_query(tui_list_t *p)
{
	p->in_query = true;
	if (t_query(following_list_get_name(p->data, p->cur)) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints("\033[0;1;44;31m\033[33m ���� m �� ���� Q,�� ����һλ ����"
			"��һλ <Space>,��                            \033[m");
	refresh();
	return DONOTHING;
}

static tui_list_handler_t following_list_handler(tui_list_t *p, int key)
{
	switch (key) {
		case 'a':
			if (tui_follow() > 0)
				p->valid = false;
			else
				return MINIUPDATE;
			break;
		case 'd':
			if (tui_unfollow(following_list_get_id(p->data, p->cur)) > 0)
				p->valid = false;
			else
				return MINIUPDATE;
			break;
		case 'm':
			m_send(following_list_get_name(p->data, p->cur));
			break;
		case 'E':
			if (tui_edit_followed_note(following_list_get_id(p->data, p->cur),
						following_list_get_notes(p->data, p->cur)) == DONOTHING ) {
				return MINIUPDATE;
			} else {
				p->valid = false;
			}
			break;
		case KEY_RIGHT:
		case '\r':
		case '\n':
			following_list_query(p);
			return DONOTHING;
		case 'h':
			show_help("help/friendshelp");
			return FULLUPDATE;
			break;
		default:
			return DONOTHING;
	}
	return DONOTHING;
}

int tui_following_list(void)
{
	tui_list_t t = {
		.data = NULL,
		.loader = following_list_loader,
		.title = following_list_title,
		.display = following_list_display,
		.handler = following_list_handler,
		.query = following_list_query
	};

	set_user_status(ST_GMENU);
	tui_list(&t);

	following_list_free(t.data);
	return 0;
}

static tui_list_loader_t black_list_loader(tui_list_t *p)
{
	if (p->data)
		black_list_free(p->data);
	p->data = black_list_load(session.uid);
	return (p->all = black_list_rows(p->data));
}

static tui_list_title_t black_list_title(tui_list_t *p)
{
	const char *middle = chkmail() ? "[�����ż�]" : BBSNAME;
	showtitle("[�༭������]", middle);
	prints(" [\033[1;32m��\033[m,\033[1;32me\033[m] �뿪"
			" [\033[1;32mh\033[m] ����"
			" [\033[1;32m��\033[m,\033[1;32mRtn\033[m] ˵����"
			" [\033[1;32m��\033[m,\033[1;32m��\033[m] ѡ��"
			" [\033[1;32ma\033[m] ���"
			" [\033[1;32md\033[m] ���\n"
			"\033[1;44m ���  �û�     ��ע\033[K\033[m\n");
}

static tui_list_display_t black_list_display(tui_list_t *p, int i)
{
	black_list_t *l = p->data;
	
	GBK_BUFFER(note, BLACK_LIST_NOTE_CCHARS);
	*gbk_note = '\0';
	convert_u2g(black_list_get_notes(l, i), gbk_note);

	prints(" %4d  %-12s %s\n", i + 1, black_list_get_name(l, i), gbk_note);
	return 0;
}

static int tui_black_list_add(void)
{
	char buf[IDLEN + 1];
	getdata(t_lines - 1, 0, "������Ҫ���ڵ���: ", buf, IDLEN, DOECHO, YEA);
	if (!*buf)
		return 0;
	GBK_UTF8_BUFFER(note, BLACK_LIST_NOTE_CCHARS);
	getdata(t_lines - 1, 0, "�����뱸ע: ",
			gbk_note, sizeof(gbk_note), DOECHO, YEA);
	convert_g2u(gbk_note, utf8_note);
	return black_list_add(session.uid, buf, utf8_note);
}

static int tui_black_list_edit(user_id_t blocked, const char *orig)
{
	GBK_UTF8_BUFFER(note, BLACK_LIST_NOTE_CCHARS);
	getdata(t_lines - 1, 0, "�����뱸ע: ",
			gbk_note, sizeof(gbk_note), DOECHO, YEA);
	convert_g2u(gbk_note, utf8_note);

	if (*utf8_note && !streq(orig, utf8_note)
			&& black_list_edit(session.uid, blocked, utf8_note) > 0)
		return FULLUPDATE;

	return MINIUPDATE;
}

static tui_list_query_t black_list_query(tui_list_t *p)
{
	p->in_query = true;
	if (t_query(black_list_get_name(p->data, p->cur)) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints("\033[0;1;33;44m ���� Q,�� ����һλ ������һλ <Space>,�� "
			"                           \033[m");
	refresh();
	return DONOTHING;
}

static tui_list_handler_t black_list_handler(tui_list_t *p, int key)
{
	switch (key) {
		case 'a':
			if (tui_black_list_add() > 0)
				p->valid = false;
			else
				return MINIUPDATE;
		case 'h':
			show_help("help/rejectshelp");
			return FULLUPDATE;
	}

	if (p->cur >= p->all)
		return DONOTHING;

	switch (key) {
		case 'd':
			if (black_list_rm(session.uid,
						black_list_get_id(p->data, p->cur)) > 0)
				p->valid = false;
			break;
		case 'E':
			return tui_black_list_edit(black_list_get_id(p->data, p->cur),
					black_list_get_notes(p->data, p->cur));
			break;
		case KEY_RIGHT:
		case '\r':
		case '\n':
			black_list_query(p);
			return DONOTHING;
	}
	return DONOTHING;
}

int tui_black_list(void)
{
	tui_list_t t = {
		.data = NULL,
		.loader = black_list_loader,
		.title = black_list_title,
		.display = black_list_display,
		.handler = black_list_handler,
		.query = black_list_query,
	};

	set_user_status(ST_GMENU);
	tui_list(&t);

	black_list_free(t.data);
	return 0;
}
