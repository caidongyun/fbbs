#include "fbbs/i18n.h"
#include "fbbs/screen.h"

/**
 * Prompt and wait user to press any key.
 * @param[in] msg The prompt message.
 * @param[in] x Line number.
 */
void presskeyfor(const char *msg, int line)
{
	move(line, 0);
	clrtoeol();

	prints("\033[1;33m%s\033[m", msg);
	getch();

	move(line, 0);
	clrtoeol();
}

/**
 * Prompt and wait user to press any key.
 */
void pressanykey(void)
{
	presskeyfor(_("Press any key to continue..."), get_screen_height() - 1);
}

#if 0
int pressreturn() {
	extern int showansi;
	char buf[3];
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(
			t_lines - 1,
			0,
			"                              [1;33m�밴 ��[5;36mEnter[m[1;33m�� ����\033[m",
			buf, 2, NOECHO, YEA);
	move(t_lines - 1, 0);
	clrtoeol();
	refresh();
	return 0;
}

/**
 * Ask for confirmation.
 * @param str The prompt string.
 * @param defa Default answer.
 * @param gobottom True if prompt at the bottom of the screen.
 * @return True if user answers "y", false if "n", default answer otherwise.
 */
bool askyn(const char *str, bool defa, bool gobottom)
{
	int x, y;
	char buf[100];
	char ans[3];
	snprintf(buf, sizeof(buf), "%s %s", str, (defa) ? "(YES/no)? [Y]"
			: "(yes/NO)? [N]");
	if (gobottom)
		move(t_lines - 1, 0);
	getyx(&x, &y);
	clrtoeol();
	getdata(x, y, buf, ans, 2, DOECHO, YEA);
	switch (ans[0]) {
		case 'Y':
		case 'y':
			return true;
		case 'N':
		case 'n':
			return false;
		default:
			return defa;
	}
}

int msgmorebar(char *filename) {
	extern int showansi;
	char title[256];
	int ch;
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();

	prints("[0m[1;44;32mѶϢ�����   ���� <[1;37mr[32m>    ��� <[1;37mc[1;32m>   �Ļ�����<[1;37mm[1;32m>                                [m");
	move(t_lines - 1, 0);

	ch = morekey();
	if (ch == 'C') {
		if (askyn("ȷ��Ҫ�����", NA, YEA) == YEA) {
			unlink(filename);
		}
		return ch;
	} else if (ch == 'M') {
		if (askyn("ȷ��Ҫ�Ļ���", NA, YEA) == YEA) {
			sprintf(title, "[%s] ����ѶϢ����", getdatestring(time(NULL), DATE_ZH));
			mail_file(filename, currentuser.userid, title);
			unlink(filename);
		}
		return ch;
	} else if (ch == 'H') {
		show_help("help/msghelp");
	}
	clrtoeol();
	refresh();
	return ch;
}

void printdash(const char *mesg)
{
	char buf[80], *ptr;
	int len;
	memset(buf, '=', sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	if (mesg != NULL) {
		len = strlen(mesg);
		if (len > sizeof(buf) - 4)
		len = sizeof(buf) - 4;
		ptr = buf + (sizeof(buf) - 1 - len) / 2 - 1;
		*ptr++ = ' ';
		memcpy(ptr, mesg, len);
		ptr[len] = ' ';
	}
	prints("%s\n", buf);
}

/* 990807.edwardc fix beep sound in bbsd .. */

void bell(void)
{
	ochar(Ctrl('G'));
}

#define LOOKFIRST  (0)
#define LOOKLAST   (1)
#define QUOTEMODE  (2)
#define MAXCOMSZ (1024)
#define MAXARGS (40)
#define MAXENVS (20)
#define BINDIR "/bin/"

//��dstΪĿ¼,�Ҳ���.,..,���һ���ַ���Ϊ/,
//			����ɾ��,�ɹ�����	1
//					 ���򷵻�	0
int deltree(char *dst) {
	if (strstr(dst, "//") || strstr(dst, "..") || strchr(dst, ' '))
		return 0; /* precaution */
	if (dst[strlen(dst) - 1] == '/')
		return 0;
	if (dashd(dst)) {
		sprintf(genbuf, "/bin/rm -rf %s", dst);
		system(genbuf);
		return 1;
	} else
		return 0;
}
#endif
