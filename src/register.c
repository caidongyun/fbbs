#include "bbs.h"
#include "fbbs/register.h"

enum {
	MAX_NEW_TRIES = 9,
	MAX_SET_PASSWD_TRIES = 7,
	MIN_PASSWD_LENGTH = 4
};

//modified by money 2002.11.15
extern char fromhost[60];
extern time_t login_start_time;

#ifdef ALLOWSWITCHCODE
extern int convcode;
#endif

/**
 *
 */
static void fill_new_userec(struct userec *user, const char *userid,
		const char *passwd, bool usegbk)
{
	memset(user, 0, sizeof(*user));
	strlcpy(user->userid, userid, sizeof(user->userid));
	strlcpy(user->passwd, genpasswd(passwd), ENCPASSLEN);

	user->gender = 'X';
#ifdef ALLOWGAME
	user->money = 1000;
#endif
	user->userdefine = ~0;
	if (!strcmp(userid, "guest")) {
		user->userlevel = 0;
		user->userdefine &= ~(DEF_FRIENDCALL | DEF_ALLMSG | DEF_FRIENDMSG);
	} else {
		user->userlevel = PERM_LOGIN;
		user->flags[0] = PAGER_FLAG;
	}
	user->userdefine &= ~(DEF_NOLOGINSEND);

	if (!usegbk)
		user->userdefine &= ~DEF_USEGB;

	user->flags[1] = 0;
	user->firstlogin = user->lastlogin = time(NULL);
}

/**
 *
 *
 */
static int gen_captcha_link(char *link, size_t size, int *n)
{
	char target[HOMELEN];
	int r = urandom_pos_int() % NUM_CAPTCHAS;
	snprintf(target, sizeof(target), CAPTCHA_DIR"/%d.gif", r + 1);
	while (1) {
		*n = urandom_pos_int();
		snprintf(link, size, CAPTCHA_OUT"/%d.gif", *n);
		if (symlink(target, link) == 0)
			return r;
		if (errno != EEXIST)
			return -1;
	}
}

/**
 *
 *
 */
static int get_captcha_answer(int pos, char *answer, size_t size)
{
	FILE *fp = fopen(CAPTCHA_INDEX, "r");
	if (!fp)
		return -1;
	fseek(fp, pos * (CAPTCHA_LEN + 1), SEEK_SET);
	fread(answer, size, 1, fp);
	strtok(answer, " ");
	fclose(fp);
	return 0;
}

/**
 * Telnet register interface.
 */
void new_register(void)
{
	char userid[IDLEN + 1], passwd[PASSLEN], passbuf[PASSLEN], log[STRLEN];
	const char *errmsg;

	if (is_no_register()) {
		ansimore("NOREGISTER", NA);
		pressreturn();
		return;
	}

	ansimore("etc/register", NA);
#ifndef FDQUAN
	if (!askyn("���Ƿ�ͬ�ⱾվAnnounce�澫����x-3Ŀ¼����վ��?", false, false))
		return;
#endif

	int tried = 0;
	prints("\n");
	while (1) {
		if (++tried >= MAX_NEW_TRIES) {
			outs("\n�ݰݣ���̫����  <Enter> ��...\n");
			refresh();
			return;
		}

		getdata(0, 0, "�������ʺ����� (Enter User ID, \"0\" to abort): ",
				userid, sizeof(userid), DOECHO, YEA);
		if (userid[0] == '0')
			return;
		errmsg = invalid_userid(userid);
		if (errmsg != NULL) {
			outs(errmsg);
			continue;
		}

		char path[HOMELEN];
		sethomepath(path, userid);
		if (dosearchuser(userid, &currentuser, &usernum) || dashd(path)) {
			outs("���ʺ��Ѿ�����ʹ��\n");
			continue;
		}
#ifndef REG_CAPTCHA
		break;
#else
		char link[STRLEN], attempt[CAPTCHA_LEN + 1], answer[CAPTCHA_LEN + 1];
		int lnum;
		int pos = gen_captcha_link(link, sizeof(link), &lnum);
		if (pos < 0)
			return;

		prints("http://"BBSHOST"/captcha/%d.gif\n", lnum);
		getdata(0, 0, "��������ͼ��������Ӣ����ĸ: ", attempt, sizeof(attempt),
				DOECHO, YEA);
		unlink(link);

		get_captcha_answer(pos, answer, sizeof(answer));
		if (strcasecmp(answer, attempt) != 0) {
			outs("��֤���������...\n");
			continue;
		} else {
			break;
		}
#endif // REG_CAPTCHA
	}

	for (tried = 0; tried <= MAX_SET_PASSWD_TRIES; ++tried) {
		passbuf[0] = '\0';
		getdata(0, 0, "���趨�������� (Setup Password): ", passbuf,
				sizeof(passbuf), NOECHO, YEA);
		errmsg = invalid_password(passbuf, userid);
		if (errmsg) {
			outs(errmsg);
			continue;
		}
		strlcpy(passwd, passbuf, PASSLEN);
		getdata(0, 0, "��������һ���������� (Confirm Password): ", passbuf,
				PASSLEN, NOECHO, YEA);
		if (strncmp(passbuf, passwd, PASSLEN) != 0) {
			prints("�����������, ��������������\n");
			continue;
		}
		passwd[8] = '\0';
		break;
	}
	if (tried > MAX_SET_PASSWD_TRIES)
		return;

	struct userec newuser;
#ifdef ALLOWSWITCHCODE
	fill_new_userec(&newuser, userid, passwd, !convcode);
#else
	fill_new_userec(&newuser, userid, passwd, true);
#endif

	if (create_user(&newuser) < 0) {
		outs("Failed to create user.\n");
		return;
	}

	snprintf(log, sizeof(log), "new account from %s", fromhost);
	report(log, currentuser.userid);

	prints("�����µ�¼ %s ����дע����Ϣ\n", newuser.userid);
	pressanykey();
	return;
}

int check_register_ok(void) {
	char fname[STRLEN];

	sethomefile(fname, currentuser.userid, "register");
	if (dashf(fname)) {
		move(21, 0);
		prints("������!! ����˳����ɱ�վ��ʹ����ע������,\n");
		prints("������������ӵ��һ��ʹ���ߵ�Ȩ��������...\n");
		pressanykey();
		return 1;
	}
	return 0;
}

void tui_check_reg_mail(void)
{
	char email[EMAIL_LEN] = "��������", file[HOMELEN], buf[RNDPASSLEN + 1];
	int i = 0;

	sethomefile(file, currentuser.userid, REG_CODE_FILE);
	if (!dashf(file)) {
		move(1, 0);
		outs("    ���������ĸ�������(username@fudan.edu.cn)\n");
		outs("    \033[1;32m��վ���ø����������֤����������֤�������ĸ�������\033[m");
		do {
			getdata(3, 0, "    E-Mail:> ", email, sizeof(email), DOECHO, YEA);
			if (!valid_addr(email) || !domain_allowed(email) ||
					is_banned_email(email)) {
				prints("    �Բ���, ��email��ַ��Ч, ���������� \n");
				continue;
			} else
				break;
		} while (1);

		send_regmail(&currentuser, email);
	}

	move(4, 0);
	clrtoeol();
	move(5, 0);
	prints(" \033[1;33m   ��֤���ѷ��͵� %s �������\033[m\n", email);

	move(7, 0);
	if (askyn("    ����������֤��ô��", true, false)) {
		move(9, 0);
		outs("������ע��ȷ������, \"��֤��\"����Ϊ���ȷ��\n");
		prints("һ���� %d ���ַ�, ��Сд���в���, ��ע��.\n", RNDPASSLEN);
		outs("��ע��, ����������һ����֤�������������������룡\n\n"
				"\033[1;31m��ʾ��ע������� 3�κ�ϵͳ��Ҫ����������󶨵����䡣\033[m\n");

		for (i = 0; i < 3; i++) {
			move(15, 0);
			prints("������ %d �λ���\n", 3 - i);
			getdata(16, 0, "��������֤��: ", buf, sizeof(buf), DOECHO, YEA);
			if (activate_email(currentuser.userid, buf))
				break;
		}
	}

	if (i == 3) {
		unlink(buf);
		prints("��֤ʧ��! ���������䡣\n");
		sethomefile(buf, currentuser.userid, ".regextra");
		if (dashf(buf))
			unlink(buf);
		pressanykey();
	} else {
		prints("��֤�ɹ�!\n");
		sethomefile(buf, currentuser.userid, ".regextra");
		if (dashf(buf)) {
			prints("���ǽ���ʱ������������ʹ��Ȩ��,����˶�������ĸ�����Ϣ����ֹͣ���ķ���Ȩ��,\n");
			prints("�����ȷ����������Ǹ�����ʵ��Ϣ.\n");
		}
		if (!HAS_PERM(PERM_REGISTER)) {
			prints("�������дע�ᵥ��\n");
		}
		pressanykey();
	}
}

/*add by Ashinmarch*/
int isNumStr(char *buf) {
	if (*buf =='\0'|| !(*buf))
		return 0;
	int i;
	for (i = 0; i < strlen(buf); i++) {
		if (!(buf[i]>='0' && buf[i]<='9'))
			return 0;
	}
	return 1;
}
int isNumStrPlusX(char *buf) {
	if (*buf =='\0'|| !(*buf))
		return 0;
	int i;
	for (i = 0; i < strlen(buf); i++) {
		if (!(buf[i]>='0' && buf[i]<='9') && !(buf[i] == 'X'))
			return 0;
	}
	return 1;
}
void check_reg_extra() {
	struct schoolmate_info schmate;
	struct userec *urec = &currentuser;
	char buf[192], bufe[192];
	sethomefile(buf, currentuser.userid, ".regextra");

	if (!dashf(buf)) {
		do {
			memset(&schmate, 0, sizeof(schmate));
			strcpy(schmate.userid, currentuser.userid);
			move(1, 0);
			prints("�����������Ϣ. ����������,������������.\n");
			/*default value is 0*/
			do {
				getdata(2, 0, "������ǰ��ѧ��: ", schmate.school_num,
						SCHOOLNUMLEN+1, DOECHO, YEA);
			} while (!isNumStr(schmate.school_num)); //��������������,��������!��ͬ
			do {
				getdata(4, 0, "��������(�ⲿ�������): ", schmate.email, STRLEN,
						DOECHO, YEA);
			} while (!valid_addr(schmate.email));
			do {
				getdata(6, 0, "�������֤����: ", schmate.identity_card_num,
						IDCARDLEN+1, DOECHO, YEA);
			} while (!isNumStrPlusX(schmate.identity_card_num)
					|| strlen(schmate.identity_card_num) !=IDCARDLEN);

			do {
				getdata(8, 0, "�����ҵ֤����: ", schmate.diploma_num,
						DIPLOMANUMLEN+1, DOECHO, YEA);
			} while (!isNumStr(schmate.diploma_num));

			do {
				getdata(10, 0, "�����ֻ���̶��绰����: ", schmate.mobile_num,
						MOBILENUMLEN+1, DOECHO, YEA);
			} while (!isNumStr(schmate.mobile_num));

			strcpy(buf, "");
			getdata(11, 0, "������Ϣ������ȷ�������������֤[Y/n]", buf, 2, DOECHO, YEA);
		} while (buf[0] =='n' || buf[0] == 'N');
		sprintf(buf, "%s, %s, %s, %s, %s\n", schmate.school_num,
				schmate.email, schmate.identity_card_num,
				schmate.diploma_num, schmate.mobile_num);
		sethomefile(bufe, currentuser.userid, ".regextra");
		file_append(bufe, buf);
		do_report(".SCHOOLMATE", buf);
		send_regmail(urec, schmate.email);
	}
	clear();
	tui_check_reg_mail();
}

/*add end*/

void check_register_info() {
	struct userec *urec = &currentuser;
	FILE *fout;
	char buf[192], buf2[STRLEN];
	if (!(urec->userlevel & PERM_LOGIN)) {
		urec->userlevel = 0;
		return;
	}
#ifdef NEWCOMERREPORT
	if (urec->numlogins == 1) {
		clear();
		sprintf(buf, "tmp/newcomer.%s", currentuser.userid);
		if ((fout = fopen(buf, "w")) != NULL) {
			fprintf(fout, "��Һ�,\n\n");
			fprintf(fout, "���� %s (%s), ���� %s\n",
					currentuser.userid, urec->username, fromhost);
			fprintf(fout, "����%s������վ����, ���Ҷ��ָ�̡�\n",
					(urec->gender == 'M') ? "С��" : "СŮ��");
			move(2, 0);
			prints("�ǳ���ӭ %s ���ٱ�վ��ϣ�������ڱ�վ�ҵ������Լ���һƬ��գ�\n\n", currentuser.userid);
			prints("����������̵ĸ��˼��, ��վ����ʹ���ߴ���к�\n");
			prints("(����������, д���ֱ�Ӱ� <Enter> ����)....");
			getdata(6, 0, ":", buf2, 75, DOECHO, YEA);
			if (buf2[0] != '\0') {
				fprintf(fout, "\n\n���ҽ���:\n\n");
				fprintf(fout, "%s\n", buf2);
				getdata(7, 0, ":", buf2, 75, DOECHO, YEA);
				if (buf2[0] != '\0') {
					fprintf(fout, "%s\n", buf2);
					getdata(8, 0, ":", buf2, 75, DOECHO, YEA);
					if (buf2[0] != '\0') {
						fprintf(fout, "%s\n", buf2);
					}
				}
			}
			fclose(fout);
			sprintf(buf2, "������·: %s", urec->username);
			Postfile(buf, "newcomers", buf2, 2);
			unlink(buf);
		}
		pressanykey();
	}
#endif
#ifndef FDQUAN
	//�������
	while (!HAS_PERM(PERM_BINDMAIL)) {
		clear();
		if (HAS_PERM(PERM_REGISTER)) {
			while (askyn("�Ƿ�󶨸�������", NA, NA)== NA)
			//add  by eefree.06.7.20
			{
				if (askyn("�Ƿ���дУ����Ϣ", NA, NA) == NA) {
					clear();
					continue;
				}
				check_reg_extra();
				return;
			}
			//add end.
		}
		tui_check_reg_mail();
	}

#endif

	clear();
	if (HAS_PERM(PERM_REGISTER))
		return;
#ifndef AUTOGETPERM

	if (check_register_ok()) {
#endif
		set_safe_record();
		urec->userlevel |= PERM_DEFAULT;
		substitut_record(PASSFILE, urec, sizeof(struct userec), usernum);
		return;
#ifndef AUTOGETPERM

	}
#endif

	if (!chkmail())
		fill_reg_form();
}

static void getfield(int line, char *info, char *desc, char *buf, int len)
{
	move(line, 0);
	prints("  ԭ���趨: %-40.40s \033[1;32m(%s)\033[m",
			(buf[0] == '\0') ? "(δ�趨)" : buf, info);
	char prompt[STRLEN];
	snprintf(prompt, sizeof(prompt), "  %s: ", desc);
	getdata(line + 1, 0, prompt, buf, len, DOECHO, YEA);
	printable_filter(buf);
	move(line, 0);
	clrtoeol();
	prints("  %s: %s\n", desc, buf);
	clrtoeol();
}

int fill_reg_form(void)
{
	reginfo_t reg;

	if (!strcmp("guest", currentuser.userid))
		return 0;

	modify_user_mode(NEW);

	clear();
	move(2, 0);
	clrtobot();
	if (currentuser.userlevel & PERM_REGISTER) {
		prints("���Ѿ���ɱ�վ��ʹ����ע������, ��ӭ���뱾վ������.");
		pressreturn();
		return 0;
	}

	if (is_reg_pending(currentuser.userid)) {
		prints("վ����δ��������ע�����뵥, ���ȵ���������.");
		pressreturn();
		return 0;
	}

	memset(&reg, 0, sizeof(reg));
	strlcpy(reg.userid, currentuser.userid, sizeof(reg.userid));
	strlcpy(reg.email, currentuser.email, sizeof(reg.email));
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s ����, ���ʵ��д���µ�����:\n", currentuser.userid);
		do {
			getfield(6, "��������", "��ʵ����",
					reg.realname, sizeof(reg.realname));
		} while (strlen(reg.realname) < 4);

		do {
			getfield(8, "ѧУϵ�������ڵ�λ", "ѧУϵ��",
					reg.dept, sizeof(reg.dept));
		} while (strlen(reg.dept) < 6);

		do {
			getfield(10, "�������һ����ƺ���", "Ŀǰסַ",
					reg.addr, sizeof(reg.addr));
		} while (strlen(reg.addr) < 10);

		do {
			getfield(12, "����������ʱ��", "����绰",
					reg.phone, sizeof(reg.phone));
		} while (strlen(reg.phone) < 8);

		getfield(14, "У�ѻ���ҵѧУ", "У �� ��",
				reg.assoc, sizeof(reg.assoc));

		char ans[3];
		getdata(t_lines - 1, 0,
				"���������Ƿ���ȷ, �� Q ����ע�� (Y/N/Quit)? [Y]: ",
				ans, sizeof(ans), DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return 0;
		if (ans[0] != 'N' && ans[0] != 'n')
			break;
	}

	reg.regdate = time(NULL);
	append_reg_list(&reg);
	return 0;
}
