#include <stdio.h>
#include "bbs.h"
#include "record.h"
#include "fbbs/board.h"
#include "fbbs/convert.h"
#include "fbbs/fileio.h"
#include "fbbs/helper.h"
#include "fbbs/mail.h"
#include "fbbs/msg.h"
#include "fbbs/register.h"
#include "fbbs/string.h"
#include "fbbs/terminal.h"

extern void rebuild_brdshm();
int showperminfo(int, int);
char cexplain[STRLEN];
char buf2[STRLEN];
char lookgrp[30];
char bnames[3][STRLEN]; //����û����ΰ����İ���,���Ϊ��
FILE *cleanlog;

//��userid����Ŀ¼�� ��.bmfile�ļ�,���������������bname��Ƚ�
//              find��Ŵ�1��ʼ�������ΰ�������,Ϊ0��ʾû�ҵ�
//�����ķ���ֵΪuserid���ΰ����İ�����
static int getbnames(const char *userid, const char *bname, int *find)
{
	int oldbm = 0;
	FILE *bmfp;
	char bmfilename[STRLEN], tmp[20];
	*find = 0;
	sethomefile(bmfilename, userid, ".bmfile");
	bmfp = fopen(bmfilename, "r");
	if (!bmfp) {
		return 0;
	}
	for (oldbm = 0; oldbm < 3;) {
		fscanf(bmfp, "%s\n", tmp);
		if (!strcmp(bname, tmp)) {
			*find = oldbm + 1;
		}
		strcpy(bnames[oldbm++], tmp);
		if (feof(bmfp)) {
			break;
		}
	}
	fclose(bmfp);
	return oldbm;
}

static int get_grp(char *seekstr)
{
	FILE   *fp;
	char    buf[STRLEN];
	char   *namep;
	if ((fp = fopen("0Announce/.Search", "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			strtok(NULL, "/");
			namep = strtok(NULL, "/");
			if (strlen(namep) < 30) {
				strcpy(lookgrp, namep);
				return 1;
			} else
				return 0;
		}
	}
	fclose(fp);
	return 0;
}

//      �޸�ʹ��������
int m_info() {
	struct userec user;
	char reportbuf[30];
	int id;

	if (!(HAS_PERM(PERM_USER)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return 0;
	}
	clear();
	stand_title("�޸�ʹ��������");
	if (!gettheuserid(1, "������ʹ���ߴ���: ", &id))
		return -1;
	memcpy(&user, &lookupuser, sizeof(user));
	sprintf(reportbuf, "check info: %s", user.userid);
	report(reportbuf, currentuser.userid);

	move(1, 0);
	clrtobot();
	disply_userinfo(&user);
	uinfo_query(&user, 1, id);
	return 0;
}

static const char *ordain_bm_check(const board_t *board, const char *uname)
{
	if (strneq(board->bms, "SYSOPs", 6))
		return "�������İ����� SYSOPs �㲻������������";
	if (strlen(uname) + strlen(board->bms) > BMNAMEMAXLEN)
		return "�����������б�̫��,�޷�����!";
	if (streq(uname, "guest"))
		return "�㲻������ guest ������";

	int find;
	int bms = getbnames(lookupuser.userid, board->name, &find);
	if (find || bms >= 3)
		return "�Ѿ��Ǹ�/������İ�����";

	bms = 1;
	for (const char *s = board->bms; *s; ++s) {
		if (*s == ' ')
			++bms;
	}
	if (bms >= BMMAXNUM)
		return "���������� 5 ������";

	return NULL;
}

static bool ordain_bm(int bid, const char *uname)
{
	user_id_t uid = get_user_id(uname);
	if (uid <= 0)
		return false;

	db_res_t *res = db_cmd("INSERT INTO bms (user_id, board_id, stamp) "
			"VALUES (%d, %d, current_timestamp) ", uid, bid);
	db_clear(res);
	return res;
}

int tui_ordain_bm(const char *cmd)
{
	if (!(HAS_PERM(PERM_USER)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd())
		return 0;

	clear();
	stand_title("��������\n");
	clrtoeol();

	int id;
	if (!gettheuserid(2, "������������ʹ�����ʺ�: ", &id))
		return 0;

	char bname[BOARD_NAME_LEN];
	board_t board;
	board_complete(3, "�����ʹ���߽����������������: ", bname, sizeof(bname),
			AC_LIST_BOARDS_ONLY);
	if (!*bname || !get_board(bname, &board))
		return -1;
	board_to_gbk(&board);

	move(4, 0);
	clrtobot();

	const char *error = ordain_bm_check(&board, lookupuser.userid);
	if (error) {
		move(5, 0);
		outs(error);
		pressanykey();
		clear();
		return -1;
	}

	bool bm1 = !board.bms[0];
	const char *bm_s = bm1 ? "��" : "��";
	prints("\n�㽫���� %s Ϊ %s ���%s.\n", lookupuser.userid, bname, bm_s);
	if (askyn("��ȷ��Ҫ������?", NA, NA) == NA) {
		prints("ȡ����������");
		pressanykey();
		clear();
		return -1;
	}

	if (!ordain_bm(board.id, lookupuser.userid)) {
		prints("Error");
		pressanykey();
		clear();
		return -1;
	}

	if (!HAS_PERM2(PERM_BOARDS, &lookupuser)) {
		lookupuser.userlevel |= PERM_BOARDS;
		substitut_record(PASSFILE, &lookupuser, sizeof(struct userec), id);

		char buf[STRLEN];
		snprintf(buf, sizeof(buf), "��������, ���� %s ����Ȩ��",
				lookupuser.userid);
		securityreport(buf, 0, 1);
		move(15, 0);
		outs(buf);
		pressanykey();
		clear();
	}

	char old_descr[STRLEN];
	snprintf(old_descr, sizeof(old_descr), "�� %s", board.descr);

	//sprintf(genbuf, "%-38.38s(BM: %s)", fh.title +8, fh.BM);
	//����������ʾ: ��̬����        ��ʾ10���ո� printf("%*c",10,' ');
	{
		int blanklen; //ǰ�����ռ��С
		static const char BLANK = ' ';
		blanklen = STRLEN - strlen(old_descr) - strlen(board.bms) - 7;
		blanklen /= 2;
		blanklen = (blanklen > 0) ? blanklen : 1;
		sprintf(genbuf, "%s%*c(BM: %s)",
				old_descr, blanklen, BLANK, board.bms);
	}

	get_grp(board.name);
	edit_grp(board.name, lookgrp, old_descr, genbuf);

	char file[HOMELEN];
	sethomefile(file, lookupuser.userid, ".bmfile");
	FILE *fp = fopen(file, "a");
	if (fp) {
		fprintf(fp, "%s\n", lookupuser.userid);
		fclose(fp);
	}

	/* Modified by Amigo 2002.07.01. Add reference to BM-Guide. */
	//sprintf (genbuf, "\n\t\t\t�� ͨ�� ��\n\n"
	//	   "\t���� %s Ϊ %s ��%s��\n"
	//	   "\t��ӭ %s ǰ�� BM_Home ��ͱ��� Zone �������ʺá�\n"
	//	   "\t��ʼ����ǰ������ͨ��BM_Home�澫�����İ���ָ��Ŀ¼��\n",
	//	   lookupuser.userid, bname, bm ? "����" : "�渱", lookupuser.userid);

	//the new version add by Danielfree 06.11.12
	sprintf(
			genbuf,
			"\n"
			" 		[1;31m   �X�T�[�X�T�[�X�T�[�X�T�[										 [m\n"
			" 	 [31m�賓��[1m�U[33m��[31m�U�U[33m��[31m�U�U[33m��[31m�U�U[33m��[31m�U[0;33m����[1;36m�����վ�澫����Ϥ����������[0;33m�����  [m\n"
			" 	 [31m��    [1m�^�T�a�^�T�a�^�T�a�^�T�a										  [m\n"
			" 	 [31m��																	  [m\n"
			" 		 [1;33m��	[37m����  %s  Ϊ  %s  ���%s��							   [m\n"
			" 		 [1;33mͨ																  [m\n"
			" 		[1m	��ӭ  %s  ǰ�� BM_Home ��ͱ��� Zone �������ʺá�			 [m\n"
			" 		 [1;33m��																  [m\n"
			" 		 [1;33m��	[37m��ʼ����ǰ������ͨ��BM_Home�澫�����İ���ָ��Ŀ¼��		   [m\n"
			" 																		 [33m��  [m\n"
			" 											 [1;33m�X�T�[�X�T�[�X�T�[�X�T�[   [0;33m ��  [m\n"
			" 	 [31m�����[1;35m��ά���������򡤽����г�⻪��[0;31m����[1;33m�U[31m��[33m�U�U[31m��[33m�U�U[31mί[33m�U�U[31m��[33m�U[0;33m������	[m\n"
			" 											 [1;33m�^�T�a�^�T�a�^�T�a�^�T�a		  [m\n"
			" 																			 [m\n", lookupuser.userid, bname,
			bm_s, lookupuser.userid);
	//add end

	char ps[5][STRLEN];
	move(8, 0);
	prints("��������������(������У��� Enter ����)");
	for (int i = 0; i < 5; i++) {
		getdata(i + 9, 0, ": ", ps[i], STRLEN - 5, DOECHO, YEA);
		if (ps[i][0] == '\0')
			break;
	}
	for (int i = 0; i < 5; i++) {
		if (ps[i][0] == '\0')
			break;
		if (i == 0)
			strcat(genbuf, "\n\n");
		strcat(genbuf, "\t");
		strcat(genbuf, ps[i]);
		strcat(genbuf, "\n");
	}

	char buf[STRLEN];
	snprintf(buf, sizeof(buf), "���� %s Ϊ %s ���%s", lookupuser.userid,
			board.name, bm_s);
	autoreport(board.name, buf, genbuf, lookupuser.userid, POST_FILE_BMS);
#ifdef ORDAINBM_POST_BOARDNAME
	autoreport(ORDAINBM_POST_BOARDNAME, buf, genbuf, lookupuser.userid,
			POST_FILE_BMS);
#endif
	securityreport(buf, 0, 1);
	move(16, 0);
	outs(buf);
	pressanykey();
	return 0;
}

static bool retire_bm(int bid, const char *uname)
{
	db_res_t *res = db_cmd("DELETE FROM bms b USING users u "
			"WHERE b.user_id = u.id AND b.board_id = %d AND u.name = %s",
			bid, uname);
	db_clear(res);
	return res;
}

int tui_retire_bm(const char *cmd)
{
	int id, right = 0, j = 0, bmnum;
	int find, bm = 1;
	FILE *bmfp;
	char bmfilename[STRLEN], usernames[BMMAXNUM][STRLEN];

	if (!(HAS_PERM(PERM_USER)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd())
		return 0;

	clear();
	stand_title("������ְ\n");
	clrtoeol();
	if (!gettheuserid(2, "��������ְ�İ����ʺ�: ", &id))
		return -1;

	char bname[BOARD_NAME_LEN];
	board_t board;
	board_complete(3, "������ð���Ҫ��ȥ�İ���: ", bname, sizeof(bname),
			AC_LIST_BOARDS_ONLY);
	if (!*bname || !get_board(bname, &board))
		return -1;
	board_to_gbk(&board);

	int oldbm = getbnames(lookupuser.userid, bname, &find);
	if (!oldbm || !find) {
		move(5, 0);
		prints(" %s %s����������д�����֪ͨ����վ����", lookupuser.userid,
				(oldbm) ? "���Ǹ�" : "û�е����κ�");
		pressanykey();
		clear();
		return -1;
	}
	for (int i = find - 1; i < oldbm; i++) {
		if (i != oldbm - 1)
			strcpy(bnames[i], bnames[i + 1]);
	}
	bmnum = 0;
	for (int i = 0; board.bms[i] != '\0'; i++) {
		if (board.bms[i] == ' ') {
			usernames[bmnum][j] = '\0';
			bmnum++;
			j = 0;
		} else {
			usernames[bmnum][j++] = board.bms[i];
		}
	}
	usernames[bmnum++][j] = '\0';
	for (int i = 0; i < bmnum; i++) {
		if (!strcmp(usernames[i], lookupuser.userid)) {
			right = 1;
			if (i)
				bm = 0;
		}
		if (right && i != bmnum - 1) //while(right&&i<bmnum)�ƺ�����һЩ;infotech
			strcpy(usernames[i], usernames[i + 1]);
	}
	if (!right) {
		move(5, 0);
		prints("�Բ��� %s �����������û�� %s �����д�����֪ͨ����վ����", bname,
				lookupuser.userid);
		pressanykey();
		clear();
		return -1;
	}
	prints("\n�㽫ȡ�� %s �� %s ���%sְ��.\n", lookupuser.userid, bname, bm ? "��"
			: "��");
	if (askyn("��ȷ��Ҫȡ�����ĸð����ְ����?", NA, NA) == NA) {
		prints("\n�Ǻǣ���ı������ˣ� %s �������� %s �����ְ��", lookupuser.userid, bname);
		pressanykey();
		clear();
		return -1;
	}

	retire_bm(board.id, lookupuser.userid);

	char old_descr[STRLEN];
	snprintf(old_descr, sizeof(old_descr), "�� %s", board.descr);

	if (!streq(board.bms, lookupuser.userid)) {
		//sprintf(genbuf, "%-38.38s(BM: %s)", fh.title +8, fh.BM);
		//����������ʾ: ��̬����        ��ʾ10���ո� printf("%*c",10,' ');
		{
			int blanklen; //ǰ�����ռ��С
			static const char BLANK = ' ';
			blanklen = STRLEN - strlen(old_descr) - strlen(board.bms) - 7;
			blanklen /= 2;
			blanklen = (blanklen > 0) ? blanklen : 1;
			sprintf(genbuf, "%s%*c(BM: %s)", old_descr, blanklen,
					BLANK, board.bms);
		}
	} else {
		sprintf(genbuf, "%-38.38s", old_descr);
	}
	get_grp(board.name);
	edit_grp(board.name, lookgrp, old_descr, genbuf);
	sprintf(genbuf, "ȡ�� %s �� %s �����ְ��", lookupuser.userid, board.name);
	securityreport(genbuf, 0, 1);
	move(8, 0);
	outs(genbuf);
	sethomefile(bmfilename, lookupuser.userid, ".bmfile");
	if (oldbm - 1) {
		bmfp = fopen(bmfilename, "w+");
		for (int i = 0; i < oldbm - 1; i++)
			fprintf(bmfp, "%s\n", bnames[i]);
		fclose(bmfp);
	} else {
		char secu[STRLEN];

		unlink(bmfilename);
		if (!(lookupuser.userlevel & PERM_OBOARDS) //û������������Ȩ��
				&& !(lookupuser.userlevel & PERM_SYSOPS) //û��վ��Ȩ��
		) {
			lookupuser.userlevel &= ~PERM_BOARDS;
			substitut_record(PASSFILE, &lookupuser, sizeof(struct userec),
					id);
			sprintf(secu, "����жְ, ȡ�� %s �İ���Ȩ��", lookupuser.userid);
			securityreport(secu, 0, 1);
			move(9, 0);
			outs(secu);
		}
	}
	prints("\n\n");
	if (askyn("��Ҫ����ذ��淢��ͨ����?", YEA, NA) == NA) {
		pressanykey();
		return 0;
	}
	prints("\n");
	if (askyn("���������밴 Enter ��ȷ�ϣ���ְ�ͷ��� N ��", YEA, NA) == YEA)
		right = 1;
	else
		right = 0;
	if (right)
		sprintf(bmfilename, "%s ��%s %s ����ͨ��", bname, bm ? "����" : "�渱",
				lookupuser.userid);
	else
		sprintf(bmfilename, "[ͨ��]���� %s ��%s %s", bname, bm ? "����" : "�渱",
				lookupuser.userid);
	if (right) {
		sprintf(genbuf, "\n\t\t\t�� ͨ�� ��\n\n"
			"\t��վ��ίԱ�����ۣ�\n"
			"\tͬ�� %s ��ȥ %s ���%sְ��\n"
			"\t�ڴˣ����������� %s �������������ʾ��л��\n\n"
			"\tϣ�����Ҳ��֧�ֱ���Ĺ���.\n", lookupuser.userid, bname, bm ? "����"
				: "�渱", bname);
	} else {
		sprintf(genbuf, "\n\t\t\t����ְͨ�桿\n\n"
			"\t��վ��ίԱ�����۾�����\n"
			"\t���� %s ��%s %s ��%sְ��\n", bname, bm ? "����" : "�渱",
				lookupuser.userid, bm ? "����" : "�渱");
	}

	char buf[5][STRLEN];
	for (int i = 0; i < 5; i++)
		buf[i][0] = '\0';
	move(14, 0);
	prints("������%s����(������У��� Enter ����)", right ? "��������" : "������ְ");
	for (int i = 0; i < 5; i++) {
		getdata(i + 15, 0, ": ", buf[i], STRLEN - 5, DOECHO, YEA);
		if (buf[i][0] == '\0')
			break;
		//      if(i == 0) strcat(genbuf,right?"\n\n���θ��ԣ�\n":"\n\n��ְ˵����\n");
		if (i == 0)
			strcat(genbuf, "\n\n");
		strcat(genbuf, "\t");
		strcat(genbuf, buf[i]);
		strcat(genbuf, "\n");
	}
	autoreport(board.name, bmfilename, genbuf, lookupuser.userid, POST_FILE_BMS);

	prints("\nִ����ϣ�");
	pressanykey();
	return 0;
}

static bool valid_board_name(const char *name)
{
	for (const char *s = name; *s; ++s) {
		char ch = *s;
		if (!isalnum(ch) && ch != '_' && ch != '.' && ch != '~')
			return false;
	}
	return true;
}

static int select_section(void)
{
	int id = 0;
	char buf[3];
	getdata(5, 0, "���������: ", buf, sizeof(buf), DOECHO, YEA);
	if (*buf) {
		db_res_t *res = db_query("SELECT id FROM board_sectors "
				"WHERE lower(name) = lower(%s)", buf);
		if (res && db_res_rows(res) == 1)
			id = db_get_integer(res, 0, 0);
		db_clear(res);
	}
	return id;
}

const char *chgrp(void)
{
	const char *explain[] = {
		"BBS ϵͳ", "������ѧ", "Ժϵ���", "���Լ���", "��������", "��ѧ����",
		"��������", "���Կռ�", "������Ϣ", "ѧ��ѧ��", "����Ӱ��", "����ר��",
		"���ط���", NULL
	};
	const char *groups[] = {
        "system.faq", "campus.faq", "ccu.faq", "comp.faq", "rec.faq",
		"literal.faq", "sport.faq", "talk.faq", "news.faq", "sci.faq",
		"other.faq", "business.faq", "hide.faq", NULL
	};

	clear();
	move(2, 0);
	prints("ѡ�񾫻�����Ŀ¼\n\n");

	int i, ch;
	for (i = 0; ; ++i) {
		if (!explain[i] || !groups[i])
			break;
		prints("\033[1;32m%2d\033[m. %-20s%-20s\n", i, explain[i], groups[i]);
	}

	char buf[STRLEN], ans[6];
	snprintf(buf, sizeof(buf), "����������ѡ��(0~%d): ", --i);
	while (1) {
		getdata(i + 6, 0, buf, ans, sizeof(ans), DOECHO, YEA);
		if (!isdigit(ans[0]))
			continue;
		ch = atoi(ans);
		if (ch < 0 || ch > i || ans[0] == '\r' || ans[0] == '\0')
			continue;
		else
			break;
	}
	snprintf(cexplain, sizeof(cexplain), "%s", explain[ch]);

	return groups[ch];
}

static int insert_categ(const char *categ)
{
	int id;
	db_res_t *res = db_query("SELECT id FROM board_categs "
			"WHERE name = %s", categ);
	if (res && db_res_rows(res) == 1) {
		id = db_get_integer(res, 0, 0);
		db_clear(res);
		return id;
	}

	res = db_query("INSERT INTO board_categs (name) "
			"VALUES (%s) RETURNING id", categ);
	if (res && db_res_rows(res) == 1) {
		id = db_get_integer(res, 0, 0);
		db_clear(res);
		return id;
	}
	return 0;
}

static int set_board_name(char *bname, size_t size)
{
	while (1) {
		getdata(2, 0, "����������:   ", bname, sizeof(bname), DOECHO, YEA);
		if (*bname) {
			board_t board;
			if (get_board(bname, &board)) {
				prints("\n����! ���������Ѿ�����!!");
				pressanykey();
				return -1;
			}
		} else {
			return -1;
		}

		if (valid_board_name(bname))
			break;
		prints("\n���Ϸ�����!!");
		return -1;
	}
	return 0;
}

int tui_new_board(const char *cmd)
{
	if (!(HAS_PERM(PERM_BLEVELS)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return 0;
	}

	clear();
	stand_title("������������");

	char bname[BOARD_NAME_LEN + 1];
	if (set_board_name(bname, sizeof(bname)) != 0)
		return -1;

	GBK_UTF8_BUFFER(descr, BOARD_DESCR_CCHARS);
	getdata(3, 0, "������˵��: ", gbk_descr, sizeof(gbk_descr), DOECHO, YEA);
	if (!*gbk_descr)
		return -1;
	convert_g2u(gbk_descr, utf8_descr);

	GBK_UTF8_BUFFER(categ, BOARD_CATEG_CCHARS);
	getdata(4, 0, "���������: ", gbk_categ, sizeof(gbk_categ), DOECHO, YEA);
	convert_g2u(gbk_categ, utf8_categ);
	int categ = insert_categ(utf8_categ);
	
	int sector = select_section();

	char pname[BOARD_NAME_LEN];
	board_complete(6, "��������Ŀ¼: ", pname, sizeof(pname),
			AC_LIST_DIR_ONLY);
	board_t parent;
	get_board(pname, &parent);

	int flag = 0, perm = 0;
	move(7, 0);
	clrtobot();
	if (askyn("������Ŀ¼��?", NA, NA)) {
		flag |= (BOARD_DIR_FLAG | BOARD_JUNK_FLAG
				| BOARD_NOREPLY_FLAG | BOARD_POST_FLAG);
		if (askyn("�Ƿ����ƴ�ȡȨ��?", NA, NA)) {
			char ans[2];
			getdata(8, 0, "���ƶ�? [R]: ", ans, sizeof(ans), DOECHO, YEA);
			move(1, 0);
			clrtobot();
			move(2, 0);
			prints("�趨 %s Ȩ��. ������: '%s'\n", "READ", bname);
			perm = setperms(perm, "Ȩ��", NUMPERMS, showperminfo);
			clear();
		}
	} else {
		if (askyn("�ð��ȫ�����¾������Իظ�", NA, NA))
			flag |= BOARD_NOREPLY_FLAG;
		if (askyn("�Ƿ��Ǿ��ֲ�����", NA, NA)) {
			flag |= BOARD_CLUB_FLAG;
			if (askyn("�Ƿ�����ƾ��ֲ�����", NA, NA))
				flag |= BOARD_READ_FLAG;
		}
		if (askyn("�Ƿ񲻼���������", NA, NA))
			flag |= BOARD_JUNK_FLAG;
		if (askyn("�Ƿ�Ϊ������", NA, NA))
			flag |= BOARD_ANONY_FLAG;
#ifdef ENABLE_PREFIX
		if (askyn ("�Ƿ�ǿ��ʹ��ǰ׺", NA, NA))
			flag |= BOARD_PREFIX_FLAG;
#endif
		if (askyn("�Ƿ����ƶ�д", NA, NA)) {
			char ans[2];
			getdata(15, 0, "���ƶ�(R)/д(P)? [R]: ", ans, sizeof(ans),
					DOECHO, YEA);
			if (*ans == 'P' || *ans == 'p')
				flag |= BOARD_POST_FLAG;
			move(1, 0);
			clrtobot();
			move(2, 0);
			prints("�趨 %s ����. ������: '%s'\n",
					(flag & BOARD_POST_FLAG ? "д" : "��"), bname);
			perm = setperms(perm, "Ȩ��", NUMPERMS, showperminfo);
			clear();
		}
	}

	db_res_t *res = db_query("INSERT INTO boards "
			"(name, descr, parent, flag, perm, categ, sector) "
			"VALUES (%s, %s, %d, %d, %d, %d, %d) RETURNING id",
			bname, utf8_descr, parent.id, flag, perm, categ, sector);
	if (!res) {
		prints("\n�����°����\n");
		pressanykey();
		clear();
		return -1;
	}
	int bid = db_get_integer(res, 0, 0);
	db_clear(res);

	char *bms = NULL;
	if (!(flag & BOARD_DIR_FLAG)
			&& !askyn("�������������(������SYSOPs����)?", YEA, NA)) {
		bms = "SYSOPs";
		ordain_bm(bid, bms);
	}

	char vdir[HOMELEN];
	snprintf(vdir, sizeof(vdir), "vote/%s", bname);
	char bdir[HOMELEN];
	snprintf(bdir, sizeof(bdir), "boards/%s", bname);
	if (mkdir(bdir, 0755) != 0 || mkdir(vdir, 0755) != 0) {
		prints("\n�½�Ŀ¼����!\n");
		pressreturn();
		clear();
		return -1;
	}

	if (!(flag & BOARD_DIR_FLAG)) {
		const char *group = chgrp();
		if (group) {
			char buf[STRLEN];
			if (bms) {
				snprintf(buf, sizeof(buf), "�� %-35.35s(BM: %s)",
						gbk_descr, bms);
			} else {
				snprintf(buf, sizeof(buf), "�� %-35.35s", gbk_descr);
			}
			if (add_grp(group, cexplain, bname, buf) == -1) {
				prints("\n����������ʧ��....\n");
			} else {
				prints("�Ѿ����뾫����...\n");
			}
		}
	}

	rebuild_brdshm(); //add by cometcaptor 2006-10-13
	prints("\n������������\n");

	char buf[STRLEN];
	snprintf(buf, sizeof(buf), "�����°棺%s", bname);
	securityreport(buf, 0, 1);

	clear();
	return 0;
}

static void show_edit_board_menu(board_t *bp, board_t *pp)
{
	prints("1)�޸�����:        %s\n", bp->name);
	prints("2)�޸�˵��:        %s\n", bp->descr);
	prints("4)�޸�����Ŀ¼:    %s(%d)\n", pp->name, pp->id);
	if (bp->flag & BOARD_DIR_FLAG) {
		prints("5)�޸Ķ�д����:    %s\n",
				(bp->perm == 0) ? "û������" : "r(�����Ķ�)");
	} else {
		prints("5)�޸Ķ�д����:    %s\n",
				(bp->flag & BOARD_POST_FLAG) ? "p(���Ʒ���)"
				: (bp->perm == 0) ? "û������" : "r(�����Ķ�)");
	}

	if (!(bp->flag & BOARD_DIR_FLAG)) {
		prints("8)��������:            %s\n",
				(bp->flag & BOARD_ANONY_FLAG) ? "��" : "��");
		prints("9)���Իظ�:            %s\n",
				(bp->flag & BOARD_NOREPLY_FLAG) ? "��" : "��");
		prints("A)�Ƿ����������:      %s\n",
				(bp->flag & BOARD_JUNK_FLAG) ? "��" : "��");
		prints("B)���ֲ�����:          %s\n",
				(bp->flag & BOARD_CLUB_FLAG) ?
				(bp->flag & BOARD_READ_FLAG) ?
				"\033[1;31mc\033[0m(������)"
				: "\033[1;33mc\033[0m(д����)"
				: "�Ǿ��ֲ�");
#ifdef ENABLE_PREFIX
		prints ("C)�Ƿ�ǿ��ʹ��ǰ׺:    %s\n",
				(bp->flag & BOARD_PREFIX_FLAG) ? "��" : "��");
#endif
	}
}

static bool alter_board_name(board_t *bp)
{
	char bname[BOARD_NAME_LEN + 1];
	getdata(t_lines - 2, 0, "������������: ", bname, sizeof(bname),
			DOECHO, YEA);
	if (!*bname || streq(bp->name, bname) || !valid_board_name(bname))
		return 0;

	if (!askyn("ȷ���޸İ���?", NA, YEA))
		return 0;

	db_res_t *res = db_cmd("UPDATE boards SET name = %s WHERE id = %d",
			bname, bp->id);
	db_clear(res);
	return res;
}

static bool alter_board_descr(board_t *bp)
{
	GBK_UTF8_BUFFER(descr, BOARD_DESCR_CCHARS);
	getdata(t_lines - 2, 0, "��������˵��: ", gbk_descr, sizeof(gbk_descr),
			DOECHO, YEA);
	if (!*gbk_descr)
		return 0;

	convert_g2u(gbk_descr, utf8_descr);
	db_res_t *res = db_cmd("UPDATE boards SET descr = %s WHERE id = %d",
			utf8_descr, bp->id);
	db_clear(res);
	return res;
}

static bool alter_board_parent(board_t *bp)
{
	GBK_UTF8_BUFFER(bname, BOARD_NAME_LEN / 2);
	board_complete(15, "����������������: ", gbk_bname, sizeof(gbk_bname),
			AC_LIST_DIR_ONLY);
	convert_g2u(gbk_bname, utf8_bname);

	board_t parent;
	get_board(utf8_bname, &parent);

	db_res_t *res = db_cmd("UPDATE boards SET parent = %d WHERE id = %d",
			parent.id, bp->id);
	db_clear(res);
	return res;
}

static bool alter_board_perm(board_t *bp)
{
	char buf[STRLEN], ans[2];
	int flag = bp->flag, perm = bp->perm;
	if (bp->flag & BOARD_DIR_FLAG) {
		snprintf(buf, sizeof(buf), "(N)������ (R)�����Ķ� [%c]: ",
				(bp->perm) ? 'R' : 'N');
		getdata(15, 0, buf, ans, sizeof(ans), DOECHO, YEA);
		if (ans[0] == 'N' || ans[0] == 'n') {
			flag &= ~BOARD_POST_FLAG;
			perm = 0;
		} else {
			if (ans[0] == 'R' || ans[0] == 'r')
				flag &= ~BOARD_POST_FLAG;
			clear();
			move(2, 0);
			prints("�趨 %s '%s' ��������Ȩ��\n", "�Ķ�", bp->name);
			perm = setperms(perm, "Ȩ��", NUMPERMS, showperminfo);
			clear();
		}
	} else {
		snprintf(buf, sizeof(buf), "(N)������ (R)�����Ķ� (P)�������� ���� [%c]: ",
				(flag & BOARD_POST_FLAG) ? 'P' : (perm) ? 'R' : 'N');
		getdata(15, 0, buf, ans, sizeof(ans), DOECHO, YEA);
		if (ans[0] == 'N' || ans[0] == 'n') {
			flag &= ~BOARD_POST_FLAG;
			perm = 0;
		} else {
			if (ans[0] == 'R' || ans[0] == 'r')
				flag &= ~BOARD_POST_FLAG;
			else if (ans[0] == 'P' || ans[0] == 'p')
				flag |= BOARD_POST_FLAG;
			clear();
			move(2, 0);
			prints("�趨 %s '%s' ��������Ȩ��\n",
					(flag & BOARD_POST_FLAG) ? "����" : "�Ķ�", bp->name);
			perm = setperms(perm, "Ȩ��", NUMPERMS, showperminfo);
			clear();
		}
	}

	db_res_t *res = db_cmd("UPDATE boards SET flag = %d, perm = %d "
			"WHERE id = %d", flag, perm, bp->id);
	db_clear(res);
	return res;
}

static bool alter_board_flag(board_t *bp, const char *prompt, int flag)
{
	int f = bp->flag;
	if (askyn(prompt, (bp->flag & flag) ? YEA : NA, YEA)) {
		f |= flag;
	} else {
		f &= ~flag;
	}

	if (flag == BOARD_CLUB_FLAG && (f & BOARD_CLUB_FLAG)) {
		if (askyn("�Ƿ�����ƾ��ֲ�?",
					(bp->flag & BOARD_READ_FLAG) ? YEA : NA, NA)) {
			f |= BOARD_READ_FLAG;
		} else {
			f &= ~BOARD_READ_FLAG;
		}
	}

	db_res_t *res = db_cmd("UPDATE boards SET flag = %d WHERE id = %d",
			f, bp->id);
	db_clear(res);
	return res;
}

int tui_edit_board(const char *cmd)
{
	if (!(HAS_PERM(PERM_BLEVELS)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd())
		return 0;

	clear();
	stand_title("�޸�����������");

	char bname[BOARD_NAME_LEN + 1];
	board_complete(2, "��������������: ", bname, sizeof(bname),
			AC_LIST_BOARDS_AND_DIR);
	board_t board;
	if (!*bname || !get_board(bname, &board))
		return -1;
	board_to_gbk(&board);

	board_t parent = { .id = 0, .name = { '\0' } };
	if (board.parent) {
		get_board_by_bid(board.parent, &parent);
		board_to_gbk(&parent);
	}

	clear();
	stand_title("�޸�����������");
	move(2, 0);

	show_edit_board_menu(&board, &parent);

	char ans[2];
	getdata(14, 0, "������������[0]", ans, sizeof(ans), DOECHO, YEA);
	if (!ans[0])
		return 0;

	int res = 0;
	move(15, 0);
	switch (ans[0]) {
		case '1':
			res = alter_board_name(&board);
			break;
		case '2':
			res = alter_board_descr(&board);
			break;
		case '4':
			res = alter_board_parent(&board);
			break;
		case '5':
			res = alter_board_perm(&board);
			break;
		default:
			break;
	}

	if (!(board.flag & BOARD_DIR_FLAG)) {
		switch (ans[0]) {
			case '7':
				res = askyn("�ƶ�������", NA, YEA);
				break;
			case '8':
				res = alter_board_flag(&board, "�Ƿ�����?", BOARD_ANONY_FLAG);
				break;
			case '9':
				res = alter_board_flag(&board, "��ֹ�ظ�?", BOARD_NOREPLY_FLAG);
				break;
			case 'a':
			case 'A':
				res = alter_board_flag(&board, "����������?", BOARD_JUNK_FLAG);
				break;
			case 'b':
			case 'B':
				res = alter_board_flag(&board, "�Ƿ���ֲ�?", BOARD_CLUB_FLAG);
				break;
#ifdef ENABLE_PREFIX
			case 'c':
			case 'C':
				res = alter_board_flag(&board, "ǿ��ǰ׺?", BOARD_PREFIX_FLAG);
				break;
#endif
		}
	}

	if (res) {
		board_t nb;
		get_board_by_bid(board.id, &nb);
		board_to_gbk(&board);

		if (ans[0] == '1') {
			char secu[STRLEN];
			sprintf(secu, "�޸���������%s(%s)", board.name, nb.name);
			securityreport(secu, 0, 1);

			char old[HOMELEN], tar[HOMELEN];
			setbpath(old, board.name);
			setbpath(tar, nb.name);
			rename(old, tar);
			sprintf(old, "vote/%s", board.name);
			sprintf(tar, "vote/%s", nb.name);
			rename(old, tar);
		}

		char vbuf[STRLEN];
		if (*nb.bms) {
			snprintf(vbuf, sizeof(vbuf), "�� %-35.35s(BM: %s)",
					nb.descr, nb.bms);
		} else {
			snprintf(vbuf, sizeof(vbuf), "�� %-35.35s", nb.descr);
		}

		char old_descr[STRLEN];
		snprintf(old_descr, sizeof(old_descr), "�� %s", board.descr);

		if (ans[1] == '2') {
			get_grp(board.name);
			edit_grp(board.name, lookgrp, old_descr, vbuf);
		}

		if (ans[1] == '1' || ans[1] == '7') {
			const char *group = chgrp();
			get_grp(board.name);
			char tmp_grp[STRLEN];
			strcpy(tmp_grp, lookgrp);
			if (strcmp(tmp_grp, group)) {
				char tmpbuf[160];
				sprintf(tmpbuf, "%s:", board.name);
				del_from_file("0Announce/.Search", tmpbuf);
				if (group != NULL) {
					if (add_grp(group, cexplain, nb.name, vbuf) == -1)
						prints("\n����������ʧ��....\n");
					else
						prints("�Ѿ����뾫����...\n");

					char newpath[HOMELEN], oldpath[HOMELEN];
					sprintf(newpath, "0Announce/groups/%s/%s",
							group, nb.name);
					sprintf(oldpath, "0Announce/groups/%s/%s",
							tmp_grp, board.name);
					if (strcmp(oldpath, newpath) != 0 && dashd(oldpath)) {
						deltree(newpath);
						rename(oldpath, newpath);
						del_grp(tmp_grp, board.name, old_descr);
					}
				}
			}
		}
		char buf[STRLEN];
		snprintf(buf, sizeof(buf), "���������� %s ������ --> %s", board.name, nb.name);
		report(buf, currentuser.userid);
	}

	pressanykey();
	clear();
	return 0;
}

// ��ע�ᵥʱ��ʾ�ı���
int regtitle(void)
{
	prints("\033[1;33;44m��ע�ᵥ NEW VERSION wahahaha              "
			"                                     \033[m\n"
			" �뿪[\033[1;32m��\033[m,\033[1;32me\033[m] "
			"ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] "
			"�Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] "
			"��׼[\033[1;32my\033[m] ɾ��[\033[1;32md\033[m]\n"
			"\033[1;37;44m  ��� �û�ID       ��  ��       ϵ��"
			"             סַ             ע��ʱ��     \033[m\n");
	return 0;
}

//      ����ע�ᵥʱ��ʾ��ע��ID�б�
char *regdoent(int num, reginfo_t* ent) {
	static char buf[128];
	char rname[17];
	char dept[17];
	char addr[17];
	//struct tm* tm;
	//tm=gmtime(&ent->regdate);
	strlcpy(rname, ent->realname, 12);
	strlcpy(dept, ent->dept, 16);
	strlcpy(addr, ent->addr, 16);
	ellipsis(rname, 12);
	ellipsis(dept, 16);
	ellipsis(addr, 16);
	sprintf(buf, "  %4d %-12s %-12s %-16s %-16s %s", num, ent->userid,
			rname, dept, addr, getdatestring(ent->regdate, DATE_SHORT));
	return buf;
}

//      ����userid ��ent->userid�Ƿ����
static int filecheck(void *ent, void *userid)
{
	return !strcmp(((reginfo_t *)ent)->userid, (char *)userid);
}

// ɾ��ע�ᵥ�ļ����һ����¼
int delete_register(int index, reginfo_t* ent, char *direct) {
	delete_record(direct, sizeof(reginfo_t), index, filecheck, ent->userid);
	return DIRCHANGED;
}

//      ͨ��ע�ᵥ
int pass_register(int index, reginfo_t* ent, char *direct) {
	int unum;
	struct userec user;
	char buf[80];
	FILE *fout;

	unum = getuser(ent->userid);
	if (!unum) {
		clear();
		prints("ϵͳ����! ���޴��˺�!\n"); //      �ڻص�����ĳЩ�����,�Ҳ�����ע�ᵥ�ļ�
		pressanykey(); // unregister�еĴ˼�¼,��ɾ��
		delete_record(direct, sizeof(reginfo_t), index, filecheck,
				ent->userid);
		return DIRCHANGED;
	}

	delete_record(direct, sizeof(reginfo_t), index, filecheck, ent->userid);

	memcpy(&user, &lookupuser, sizeof(user));
#ifdef ALLOWGAME
	user.money = 1000;
#endif
	substitut_record(PASSFILE, &user, sizeof (user), unum);
	sethomefile(buf, user.userid, "register");
	if ((fout = fopen(buf, "a")) != NULL) {
		fprintf(fout, "ע��ʱ��     : %s\n", getdatestring(ent->regdate, DATE_EN));
		fprintf(fout, "�����ʺ�     : %s\n", ent->userid);
		fprintf(fout, "��ʵ����     : %s\n", ent->realname);
		fprintf(fout, "ѧУϵ��     : %s\n", ent->dept);
		fprintf(fout, "Ŀǰסַ     : %s\n", ent->addr);
		fprintf(fout, "����绰     : %s\n", ent->phone);
#ifndef FDQUAN
		fprintf(fout, "�����ʼ�     : %s\n", ent->email);
#endif
		fprintf(fout, "У �� ��     : %s\n", ent->assoc);
		fprintf(fout, "�ɹ�����     : %s\n", getdatestring(time(NULL), DATE_EN));
		fprintf(fout, "��׼��       : %s\n", currentuser.userid);
		fclose(fout);
	}
	mail_file("etc/s_fill", user.userid, "�����������Ѿ����ע�ᡣ");
	sethomefile(buf, user.userid, "mailcheck");
	unlink(buf);
	sprintf(genbuf, "�� %s ͨ�����ȷ��.", user.userid);
	securityreport(genbuf, 0, 0);

	return DIRCHANGED;
}

//      ����ע�ᵥ
int do_register(int index, reginfo_t* ent, char *direct) {
	int unum;
	struct userec user;
	//char ps[80];
	register int ch;
	static char *reason[] = { "��ȷʵ��д��ʵ����.", "������ѧУ��ϵ���꼶.", "����д������סַ����.",
			"����������绰.", "��ȷʵ��дע�������.", "����������д���뵥.", "����" };
	unsigned char rejectindex = 4;

	if (!ent)
		return DONOTHING;

	unum = getuser(ent->userid);
	if (!unum) {
		prints("ϵͳ����! ���޴��˺�!\n"); //ɾ�������ڵļ�¼,����еĻ�
		delete_record(direct, sizeof(reginfo_t), index, filecheck,
				ent->userid);
		return DIRCHANGED;
	}

	memcpy(&user, &lookupuser, sizeof (user));
	clear();
	move(0, 0);
	prints("[1;33;44m ��ϸ����                                                                      [m\n");
	prints("[1;37;42m [.]���� [+]�ܾ� [d]ɾ�� [0-6]������ԭ��                                       [m");

	//strcpy(ps, "(��)");
	for (;;) {
		disply_userinfo(&user);
		move(14, 0);
		printdash(NULL);
		prints("   ע��ʱ��   : %s\n", getdatestring(ent->regdate, DATE_EN));
		prints("   �����ʺ�   : %s\n", ent->userid);
		prints("   ��ʵ����   : %s\n", ent->realname);
		prints("   ѧУϵ��   : %s\n", ent->dept);
		prints("   Ŀǰסַ   : %s\n", ent->addr);
		prints("   ����绰   : %s\n", ent->phone);
#ifndef FDQUAN
		prints("   �����ʼ�   : %s\n", ent->email);
#endif
		prints("   У �� ��   : %s\n", ent->assoc);
		ch = egetch();
		switch (ch) {
			case '.':
				pass_register(index, ent, direct);
				return READ_AGAIN;
			case '+':
				user.userlevel &= ~PERM_SPECIAL4;
				substitut_record(PASSFILE, &user, sizeof (user), unum);
				//mail_file("etc/f_fill", user.userid, "��������д����ע������");
				mail_file("etc/f_fill", user.userid, reason[rejectindex]);
			case 'd':
				user.userlevel &= ~PERM_SPECIAL4;
				substitut_record(PASSFILE, &user, sizeof (user), unum);
				delete_register(index, ent, direct);
				return READ_AGAIN;
			case KEY_DOWN:
			case '\r':
				return READ_NEXT;
			case KEY_LEFT:
				return DIRCHANGED;
			default:
				if (ch >= '0' && ch <= '6') {
					rejectindex = ch - '0';
				}
				break;
		}
	}
	return 0;
}

struct one_key reg_comms[] = {
		{'r', do_register},
		{'y', pass_register},
		{'d', delete_register},
		{'\0', NULL}
};

void show_register() {
	FILE *fn;
	int x; //, y, wid, len;
	char uident[STRLEN];
	if (!(HAS_PERM(PERM_USER)))
		return;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("��ѯʹ����ע������");
	move(1, 0);
	usercomplete("������Ҫ��ѯ�Ĵ���: ", uident);
	if (uident[0] != '\0') {
		if (!getuser(uident)) {
			move(2, 0);
			prints("�����ʹ���ߴ���...");
		} else {
			sprintf(genbuf, "home/%c/%s/register",
					toupper(lookupuser.userid[0]), lookupuser.userid);
			if ((fn = fopen(genbuf, "r")) != NULL) {
				prints("\nע����������:\n\n");
				for (x = 1; x <= 15; x++) {
					if (fgets(genbuf, STRLEN, fn))
						prints("%s", genbuf);
					else
						break;
				}
			} else {
				prints("\n\n�Ҳ�����/����ע������!!\n");
			}
		}
	}
	pressanykey();
}
//  ���� ע�ᵥ�쿴��,��ʹ���ߵ�ע�����ϻ��ע�ᵥ�������
int m_register() {
	FILE *fn;
	char ans[3]; //, *fname;
	int x; //, y, wid, len;
	char uident[STRLEN];

	if (!(HAS_PERM(PERM_USER)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return 0;
	}
	clear();

	stand_title("�趨ʹ����ע������");
	for (;;) {
		getdata(1, 0, "(0)�뿪  (1)�����ע�� (2)��ѯʹ����ע������ ? : ", ans, 2, DOECHO,
				YEA);
		if (ans[0] == '1' || ans[0] == '2') { // || ans[0]=='3') ����ֻ��0,1,2
			break;
		} else {
			return 0;
		}
	}
	switch (ans[0]) {
		case '2':
			move(1, 0);
			usercomplete("������Ҫ��ѯ�Ĵ���: ", uident);
			if (uident[0] != '\0') {
				if (!getuser(uident)) {
					move(2, 0);
					prints("�����ʹ���ߴ���...");
				} else {
					sprintf(genbuf, "home/%c/%s/register",
							toupper(lookupuser.userid[0]),
							lookupuser.userid);
					if ((fn = fopen(genbuf, "r")) != NULL) {
						prints("\nע����������:\n\n");
						for (x = 1; x <= 15; x++) {
							if (fgets(genbuf, STRLEN, fn))
								prints("%s", genbuf);
							else
								break;
						}
					} else {
						prints("\n\n�Ҳ�����/����ע������!!\n");
					}
				}
			}
			pressanykey();
			break;
		case '1':
			i_read(ST_ADMIN, "unregistered", regtitle, regdoent,
					&reg_comms[0], sizeof(reginfo_t));
			break;
	}
	clear();
	return 0;
}

//      ɾ��һ���ʺ�
int d_user(char *cid) {
	int id, num, i;
	char secu[STRLEN];
	char genbuf_rm[STRLEN]; //added by roly 02.03.24
	char passbuf[PASSLEN];

	if (!(HAS_PERM(PERM_USER)))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return 0;
	}
	clear();
	stand_title("ɾ��ʹ�����ʺ�");
	// Added by Ashinmarch in 2008.10.20 
	// �����˺�ʱ����������֤
	getdata(1, 0, "[1;37m����������: [m", passbuf, PASSLEN, NOECHO, YEA);
	passbuf[8] = '\0';
	if (!passwd_check(currentuser.userid, passbuf)) {
		prints("[1;31m�����������...[m\n");
		return 0;
	}
	// Add end.
	if (!gettheuserid(1, "��������ɾ����ʹ���ߴ���: ", &id))
		return 0;
	if (!strcmp(lookupuser.userid, "SYSOP")) {
		prints("\n�Բ����㲻����ɾ�� SYSOP �ʺ�!!\n");
		pressreturn();
		clear();
		return 0;
	}
	if (!strcmp(lookupuser.userid, currentuser.userid)) {
		prints("\n�Բ����㲻����ɾ���Լ�������ʺ�!!\n");
		pressreturn();
		clear();
		return 0;
	}
	prints("\n\n������ [%s] �Ĳ�������:\n", lookupuser.userid);
	prints("    User ID:  [%s]\n", lookupuser.userid);
	prints("    ��   ��:  [%s]\n", lookupuser.username);
	strcpy(secu, "ltmprbBOCAMURS#@XLEast0123456789\0");
	for (num = 0; num < strlen(secu) - 1; num++) {
		if (!(lookupuser.userlevel & (1 << num)))
			secu[num] = '-';
	}
	prints("    Ȩ   ��: %s\n\n", secu);

	num = getbnames(lookupuser.userid, secu, &num);
	if (num) {
		prints("[%s] Ŀǰ�е����� %d ����İ���: ", lookupuser.userid, num);
		for (i = 0; i < num; i++)
			prints("%s ", bnames[i]);
		prints("\n����ʹ�ð���жְ����ȡ�������ְ�������ò���.");
		pressanykey();
		clear();
		return 0;
	}

	sprintf(genbuf, "��ȷ��Ҫɾ�� [%s] ��� ID ��", lookupuser.userid);
	if (askyn(genbuf, NA, NA) == NA) {
		prints("\nȡ��ɾ��ʹ����...\n");
		pressreturn();
		clear();
		return 0;
	}
	sprintf(secu, "ɾ��ʹ���ߣ�%s", lookupuser.userid);
	securityreport(secu, 0, 0);
	sprintf(genbuf, "mail/%c/%s", toupper(lookupuser.userid[0]),
			lookupuser.userid);
	//f_rm(genbuf);
	/* added by roly 02.03.24 */
	sprintf(genbuf_rm, "/bin/rm -fr %s", genbuf); //added by roly 02.03.24
	system(genbuf_rm);
	/* add end */
	sprintf(genbuf, "home/%c/%s", toupper(lookupuser.userid[0]),
			lookupuser.userid);
	//f_rm(genbuf);
	/* added by roly 02.03.24 */
	sprintf(genbuf_rm, "/bin/rm -fr %s", genbuf); //added by roly 02.03.24
	system(genbuf_rm);
	/* add end */
	lookupuser.userlevel = 0;
#ifdef ALLOWGAME
	lookupuser.money = 0;
	lookupuser.nummedals = 0;
	lookupuser.bet = 0;
#endif
	strcpy(lookupuser.username, "");
	prints("\n%s �Ѿ��������...\n", lookupuser.userid);
	lookupuser.userid[0] = '\0';
	substitut_record(PASSFILE, &lookupuser, sizeof(lookupuser), id);
	setuserid(id, lookupuser.userid);
	pressreturn();
	clear();
	return 1;
}

//      ����ʹ���ߵ�Ȩ��
int x_level() {
	int id;
	char reportbuf[60];
	unsigned int newlevel;

	if (!HAS_PERM(PERM_SYSOPS))
		return 0;

	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return 0;
	}
	clear();
	move(0, 0);
	prints("����ʹ����Ȩ��\n");
	clrtoeol();
	move(1, 0);
	usercomplete("���������ĵ�ʹ�����ʺ�: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("�趨ʹ���� '%s' ��Ȩ�� \n", genbuf);
	newlevel
			= setperms(lookupuser.userlevel, "Ȩ��", NUMPERMS, showperminfo);
	move(2, 0);
	if (newlevel == lookupuser.userlevel)
		prints("ʹ���� '%s' Ȩ��û�б��\n", lookupuser.userid);
	else {
		sprintf(reportbuf, "change level: %s %.8x -> %.8x",
				lookupuser.userid, lookupuser.userlevel, newlevel);
		report(reportbuf, currentuser.userid);
		lookupuser.userlevel = newlevel;
		{
			char secu[STRLEN];
			sprintf(secu, "�޸� %s ��Ȩ��", lookupuser.userid);
			securityreport(secu, 0, 0);
		}

		substitut_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
		if (!(lookupuser.userlevel & PERM_REGISTER)) {
			char src[STRLEN], dst[STRLEN];
			sethomefile(dst, lookupuser.userid, "register.old");
			if (dashf(dst))
				unlink(dst);
			sethomefile(src, lookupuser.userid, "register");
			if (dashf(src))
				rename(src, dst);
		}
		prints("ʹ���� '%s' Ȩ���Ѿ��������.\n", lookupuser.userid);
	}
	pressreturn();
	clear();
	return 0;
}

void a_edits() {
	int aborted;
	char ans[7], buf[STRLEN], buf2[STRLEN];
	int ch, num, confirm;
	static char *e_file[] = { "../Welcome", "../Welcome2", "issue",
			"logout", "../vote/notes", "hotspot", "menu.ini",
			"../.badname", "../.bad_email", "../.bad_host", "autopost",
			"junkboards", "sysops", "whatdate", "../NOLOGIN",
			"../NOREGISTER", "special.ini", "hosts", "restrictip",
			"freeip", "s_fill", "f_fill", "register", "firstlogin",
			"chatstation", "notbackupboards", "bbsnet.ini", "bbsnetip",
			"bbsnet2.ini", "bbsnetip2", NULL };
	static char *explain_file[] = { "�����վ������", "��վ����", "��վ��ӭ��", "��վ����",
			"���ñ���¼", "ϵͳ�ȵ�", "menu.ini", "����ע��� ID", "����ȷ��֮E-Mail",
			"������վ֮λַ", "ÿ���Զ����ŵ�", "����POST���İ�", "����������", "�������嵥",
			"��ͣ��½(NOLOGIN)", "��ͣע��(NOREGISTER)", "����ip��Դ�趨��", "����ip��Դ�趨��",
			"ֻ�ܵ�½5id��ip�趨��", "����5 id���Ƶ�ip�趨��", "ע��ɹ��ż�", "ע��ʧ���ż�",
			"���û�ע�᷶��", "�û���һ�ε�½����", "���ʻ������嵥", "����ɾ�����豸��֮�嵥",
			"BBSNET תվ�嵥", "��������ip", "BBSNET2 תվ�嵥", "����2����IP", NULL };
	set_user_status(ST_ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	move(1, 0);
	prints("����ϵͳ����\n\n");
	for (num = 0; (HAS_PERM(PERM_ESYSFILE)) ? e_file[num] != NULL
			&& explain_file[num] != NULL : strcmp(explain_file[num], "menu.ini"); num++) {
		prints("[\033[1;32m%2d\033[m] %s", num + 1, explain_file[num]);
		if (num < 17)
			move(4 + num, 0);
		else
			move(num - 14, 50);
	}
	prints("[\033[1;32m%2d\033[m] �������\n", num + 1);

	getdata(23, 0, "��Ҫ������һ��ϵͳ����: ", ans, 3, DOECHO, YEA);
	ch = atoi(ans);
	if (!isdigit(ans[0]) || ch <= 0 || ch > num || ans[0] == '\n'
			|| ans[0] == '\0')
		return;
	ch -= 1;
	sprintf(buf2, "etc/%s", e_file[ch]);
	move(3, 0);
	clrtobot();
	sprintf(buf, "(E)�༭ (D)ɾ�� %s? [E]: ", explain_file[ch]);
	getdata(3, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		sprintf(buf, "��ȷ��Ҫɾ�� %s ���ϵͳ��", explain_file[ch]);
		confirm = askyn(buf, NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("ȡ��ɾ���ж�\n");
			pressreturn();
			clear();
			return;
		}
		{
			char secu[STRLEN];
			sprintf(secu, "ɾ��ϵͳ������%s", explain_file[ch]);
			securityreport(secu, 0, 0);
		}
		unlink(buf2);
		move(5, 0);
		prints("%s ��ɾ��\n", explain_file[ch]);
		pressreturn();
		clear();
		return;
	}
	set_user_status(ST_EDITSFILE);
	aborted = vedit(buf2, NA, YEA); /* ������ļ�ͷ, �����޸�ͷ����Ϣ */
	clear();
	if (aborted != -1) {
		prints("%s ���¹�", explain_file[ch]);
		{
			char secu[STRLEN];
			sprintf(secu, "�޸�ϵͳ������%s", explain_file[ch]);
			securityreport(secu, 0, 0);
		}

		if (!strcmp(e_file[ch], "../Welcome")) {
			unlink("Welcome.rec");
			prints("\nWelcome ��¼������");
		} else if (!strcmp(e_file[ch], "whatdate")) {
			brdshm->fresh_date = time(0);
			prints("\n�������嵥 ����");
		}
	}
	pressreturn();
}

// ȫվ�㲥...
int wall() {
	char passbuf[PASSLEN];

	if (!HAS_PERM(PERM_SYSOPS))
		return 0;
	// Added by Ashinmarch on 2008.10.20
	// ȫվ�㲥ǰ����������֤
	clear();
	stand_title("ȫվ�㲥!");
	getdata(1, 0, "[1;37m����������: [m", passbuf, PASSLEN, NOECHO, YEA);
	passbuf[8] = '\0';
	if (!passwd_check(currentuser.userid, passbuf)) {
		prints("[1;31m�����������...[m\n");
		return 0;
	}
	// Add end.

	set_user_status(ST_MSG);
	move(2, 0);
	clrtobot();

	char msg[MAX_MSG_SIZE + 2];
	if (!get_msg("����ʹ����", msg, 1)) {
		return 0;
	}

	if (!broadcast_msg(msg)) {
		move(2, 0);
		prints("���Ͽ���һ��\n");
		pressanykey();
	}
	prints("\n�Ѿ��㲥���...\n");
	pressanykey();
	return 1;
}

// �趨ϵͳ����
int setsystempasswd() {
	FILE *pass;
	char passbuf[20], prepass[20];
	set_user_status(ST_ADMIN);
	if (!check_systempasswd())
		return 0;
	if (strcmp(currentuser.userid, "SYSOP")) {
		clear();
		move(10, 20);
		prints("�Բ���ϵͳ����ֻ���� SYSOP �޸ģ�");
		pressanykey();
		return 0;
	}
	getdata(2, 0, "�������µ�ϵͳ����(ֱ�ӻس���ȡ��ϵͳ����): ", passbuf, 19, NOECHO, YEA);
	if (passbuf[0] == '\0') {
		if (askyn("��ȷ��Ҫȡ��ϵͳ������?", NA, NA) == YEA) {
			unlink("etc/.syspasswd");
			securityreport("ȡ��ϵͳ����", 0, 0);
		}
		return 0;
	}
	getdata(3, 0, "ȷ���µ�ϵͳ����: ", prepass, 19, NOECHO, YEA);
	if (strcmp(passbuf, prepass)) {
		move(4, 0);
		prints("�������벻��ͬ, ȡ���˴��趨.");
		pressanykey();
		return 0;
	}
	if ((pass = fopen("etc/.syspasswd", "w")) == NULL) {
		move(4, 0);
		prints("ϵͳ�����޷��趨....");
		pressanykey();
		return 0;
	}
	fprintf(pass, "%s\n", genpasswd(passbuf));
	fclose(pass);
	move(4, 0);
	prints("ϵͳ�����趨���....");
	pressanykey();
	return 0;
}

#define DENY_LEVEL_LIST ".DenyLevel"
extern int denylist_key_deal(const char *file, int ch, const char *line);

/**
 * ȫվ�����б����.
 */
static void denylist_title_show(void)
{
	move(0, 0);
	prints("\033[1;44;36m �������ڵ�ID�б�\033[K\033[m\n"
			" �뿪[\033[1;32m��\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] ���[\033[1;32ma\033[m]  �޸�[\033[1;32mc\033[m] �ָ�[\033[1;32md\033[m] ����[\033[1;32mx\033[m] ����[\033[1;32m/\033[m]\n"
			"\033[1;44m �û�����     ����˵��(A-Z;'.[])                 Ȩ�� ��������   վ��          \033[m\n");
}

/**
 * ȫվ�����б���ں���.
 * @return 0/1.
 */
int x_new_denylevel(void)
{
	if (!HAS_PERM(PERM_OBOARDS) && !HAS_PERM(PERM_SPECIAL0))
		return DONOTHING;
	set_user_status(ST_ADMIN);
	list_text(DENY_LEVEL_LIST, denylist_title_show, denylist_key_deal, NULL);
	return FULLUPDATE;
}

int kick_user(void)
{
	if (!(HAS_PERM(PERM_OBOARDS)))
		return -1;
	set_user_status(ST_ADMIN);

	stand_title("��ʹ������վ");
	move(1, 0);

	char uname[IDLEN + 1];
	usercomplete("����ʹ�����ʺ�: ", uname);
	if (*uname == '\0') {
		clear();
		return -1;
	}

	user_id_t uid = get_user_id(uname);
	if (!uid) {
		presskeyfor("�޴��û�..", 3);
		clear();
		return 0;
	}

	move(1, 0);
	clrtoeol();
	char buf[STRLEN];
	snprintf(buf, sizeof(buf), "�ߵ�ʹ���� : [%s].", uname);
	move(2, 0);
	if (!askyn(buf, NA, NA)) {
		presskeyfor("ȡ����ʹ����..", 2);
		clear();
		return 0;
	}

	basic_session_info_t *res = get_sessions(uid);
	if (res && basic_session_info_count(res) > 0) {
		for (int i = 0; i < basic_session_info_count(res); ++i) {
			bbs_kill(basic_session_info_sid(res, i),
					basic_session_info_pid(res, i), SIGHUP);
		}
		presskeyfor("���û��Ѿ�������վ", 4);
	} else {
		move(3, 0);
		presskeyfor("���û��������ϻ��޷��߳�վ��..", 3);
	}

	basic_session_info_clear(res);
	clear();
	return 0;
}
