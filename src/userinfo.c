#include "bbs.h"
#include "fbbs/uinfo.h"

#ifndef DLM
#undef  ALLOWGAME
#endif

#ifdef FDQUAN
#define ALLOWGAME
#endif
//modified by money 2002.11.15

extern time_t login_start_time;
extern char fromhost[60];
extern char *cexpstr();

//��ptrָ����ַ������ַ�ֵΪ0xFF��ת���ɿո�
void filter_ff(char *ptr) {
	while (*ptr) {
		if (*(unsigned char *)ptr == 0xff)
			*ptr = ' ';
		ptr++;
	}
	return;
}

//	����	�趨��������  ѡ��ʱ��ʾ����Ϣ,����ʾ��������
void disply_userinfo(struct userec *u) {
	int num, exp;
	time_t now;

	move(2, 0);
	clrtobot();
	now = time(0);
	set_safe_record();
	prints("���Ĵ���     : %-14s", u->userid);
	prints("�ǳ� : %-20s", u->username);
	prints("     �Ա� : %s\n", (u->gender == 'M' ? "��" : "Ů"));
	prints("��������     : %d��%d��%d��", u->birthyear + 1900, u->birthmonth,
			u->birthday);
	prints(" (�ۼ��������� : %d)\n", days_elapsed(u->birthyear + 1900, 
			u->birthmonth, u->birthday, now));
	prints("�����ʼ����� : %s\n", u->email);
	prints("������ٻ��� : %-22s\n", u->lasthost);
	prints("�ʺŽ������� : %s[��� %d ��]\n",
			getdatestring(u->firstlogin, DATE_ZH),
			(now - (u->firstlogin)) / 86400);
	getdatestring(u->lastlogin, NA);
	prints("����������� : %s[��� %d ��]\n",
			getdatestring(u->lastlogin, DATE_ZH),
			(now-(u->lastlogin)) / 86400);
#ifdef ALLOWGAME
	prints("������Ŀ     : %-20d ������Ŀ : %d\n",u->numposts,u->nummedals);
	prints("˽������     : %d ��\n", u->nummails);
	prints("�������д�� : %dԪ  ���� : %dԪ (%s)\n",
			u->money,u->bet,cmoney(u->money-u->bet));
#else
	prints("������Ŀ     : %-20d \n", u->numposts);
	prints("˽������     : %d �� \n", u->nummails);
#endif
	prints("��վ����     : %d ��      ", u->numlogins);
	prints("��վ��ʱ��   : %d Сʱ %d ����\n", u->stay/3600, (u->stay/60)%60);
	exp = countexp(u);
	//modified by iamfat 2002.07.25
#ifdef SHOWEXP
	prints("����ֵ       : %d  (%-10s)    ", exp, cexpstr(exp));
#else
	prints("����ֵ       : [%-10s]     ", cexpstr(exp));
#endif
	exp = countperf(u);
#ifdef SHOWPERF
	prints("����ֵ : %d  (%s)\n", exp, cperf(exp));
#else
	prints("����ֵ  : [%s]\n", cperf(exp));
#endif
	strcpy(genbuf, "ltmprbBOCAMURS#@XLEast0123456789\0");
	for (num = 0; num < strlen(genbuf) ; num++)
		if (!(u->userlevel & (1 << num))) //��ӦȨ��Ϊ��,����'-'
			genbuf[num] = '-';
	prints("ʹ����Ȩ��   : %s\n", genbuf);
	prints("\n");
	if (u->userlevel & PERM_SYSOPS) {
		prints("  ���Ǳ�վ��վ��, ��л���������Ͷ�.\n");
	} else if (u->userlevel & PERM_BOARDS) {
		prints("  ���Ǳ�վ�İ���, ��л���ĸ���.\n");
	} else if (u->userlevel & PERM_REGISTER) {
		prints("  ����ע������Ѿ����, ��ӭ���뱾վ.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  ������·, ���Ķ� Announce ������.\n");
	} else {
		prints("  ע����δ�ɹ�, ��ο���վ��վ����˵��.\n");
	}
}

//	�ı��û���¼,uΪ��ǰ�ļ�¼,newinfoΪ�¼�¼,������������Ϊָ��
//		iΪ����ʾ����
void uinfo_change1(int i, struct userec *u, struct userec *newinfo) {
	char buf[STRLEN], genbuf[128];

	if (currentuser.userlevel & PERM_SYSOPS) {
		char temp[30];
		temp[0] = 0;
		FILE *fp;
		sethomefile(genbuf, u->userid, ".volunteer");
		if ((fp = fopen(genbuf, "r")) != NULL) {
			fgets(temp, 30, fp);
			fclose(fp);
			sprintf(genbuf, "�������(��ո�ȡ�����)��[%s]", temp);
		} else
			sprintf(genbuf, "������ݣ�");
		getdata(i++, 0, genbuf, buf, 30, DOECHO, YEA);
		if (buf[0]) {
			sethomefile(genbuf, u->userid, ".volunteer");
			if ((fp = fopen(genbuf, "w")) != NULL) {
				if (buf[0] != ' ') {
					fputs(buf, fp);
					fclose(fp);
				} else
					unlink(genbuf);
			}
		}
	}

	sprintf(genbuf, "�������� [%s]: ", u->email);
	getdata(i++, 0, genbuf, buf, STRLEN - 1, DOECHO, YEA);
	if (buf[0]) {
		strlcpy(newinfo->email, buf, STRLEN-12);
	}

	sprintf(genbuf, "���ߴ��� [%d]: ", u->numlogins);
	getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
	if (atoi(buf) > 0)
		newinfo->numlogins = atoi(buf);

	sprintf(genbuf, "���������� [%d]: ", u->numposts);
	getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
	if (atoi(buf) >0)
		newinfo->numposts = atoi(buf);

	sprintf(genbuf, "��½��ʱ�� [%d]: ", u->stay);
	getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
	if (atoi(buf) > 0)
		newinfo->stay = atoi(buf);
	sprintf(genbuf, "firstlogin [%d]: ", u->firstlogin);
	getdata(i++, 0, genbuf, buf, 15, DOECHO, YEA);
	if (atoi(buf) >0)
		newinfo->firstlogin = atoi(buf);
	//add end          				      	      	
#ifdef ALLOWGAME
	sprintf(genbuf, "���д�� [%d]: ", u->money);
	getdata(i++, 0, genbuf, buf, 8, DOECHO, YEA);
	if (atoi(buf)> 0)
	newinfo->money = atoi(buf);

	sprintf(genbuf, "���д��� [%d]: ", u->bet);
	getdata(i++, 0, genbuf, buf, 8, DOECHO, YEA);
	if (atoi(buf)> 0)
	newinfo->bet = atoi(buf);

	sprintf(genbuf, "������ [%d]: ", u->nummedals);
	getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
	if (atoi(buf)> 0)
	newinfo->nummedals = atoi(buf);
#endif
}

void tui_check_uinfo(struct userec *u)
{
	char ans[5];
	bool finish = false;

	while (!finish) {
		switch (check_user_profile(u)) {
			case UINFO_ENICK:
				getdata(2, 0, "�����������ǳ� (Enter nickname): ",
						u->username, NAMELEN, DOECHO, YEA);
				strlcpy(uinfo.username, u->username, sizeof(uinfo.username));
				printable_filter(uinfo.username);
				update_ulist(&uinfo, utmpent);
				break;
			case UINFO_EGENDER:
				getdata(3, 0, "�����������Ա�: M.�� F.Ů [M]: ",
						ans, 2, DOECHO, YEA);
				if (ans[0] != 'F' && ans[0] != 'f')
					u->gender = 'M';
				else
					u->gender = 'F';
				break;
			case UINFO_EBIRTH:
				getdata(4, 0, "�����������������(��λ��): ",
						ans, 5, DOECHO, YEA);
				u->birthyear = strtol(ans, NULL, 10);
				getdata(5, 0, "���������������·�: ", ans, 3, DOECHO, YEA);
				u->birthmonth = strtol(ans, NULL, 10);
				getdata(6, 0, "���������ĳ�����: ", ans, 3, DOECHO, YEA);
				u->birthday = strtol(ans, NULL, 10);
				break;
			default:
				finish = true;
				break;
		}
	}
}

//	��ѯu��ָ����û���������Ϣ
int uinfo_query(struct userec *u, int real, int unum) {
	struct userec newinfo;
	char ans[3], buf[STRLEN], genbuf[128];
	char src[STRLEN], dst[STRLEN];
	int i, fail = 0;
	unsigned char *ptr; //add by money 2003.10.29 for filter '0xff' in nick
	int r = 0; //add by money 2003.10.14 for test ����
	time_t now;
	struct tm *tmnow;
	memcpy(&newinfo, u, sizeof(currentuser));
	getdata(t_lines - 1, 0, real ? "��ѡ�� (0)���� (1)�޸����� (2)�趨���� ==> [0]"
			: "��ѡ�� (0)���� (1)�޸����� (2)�趨���� (3) ѡǩ���� ==> [0]", ans, 2,
			DOECHO, YEA);
	clear();

	//added by roly 02.03.07
	if (real && !HAS_PERM(PERM_SPECIAL0))
		return;
	//add end

	refresh();
	now = time(0);
	tmnow = localtime(&now);

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("ʹ���ߴ���: %s\n", u->userid);
	switch (ans[0]) {
		case '1':
			move(1, 0);
			prints("�������޸�,ֱ�Ӱ� <ENTER> ����ʹ�� [] �ڵ����ϡ�\n");
			sprintf(genbuf, "�ǳ� [%s]: ", u->username);
			getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
			if (buf[0]) {
				strlcpy(newinfo.username, buf, NAMELEN);
				/* added by money 2003.10.29 for filter 0xff in nick */
				ptr = newinfo.username;
				filter_ff(ptr);
				/* added end */
			}
			sprintf(genbuf, "������ [%d]: ", u->birthyear + 1900);
			getdata(i++, 0, genbuf, buf, 5, DOECHO, YEA);
			if (buf[0] && atoi(buf) > 1920 && atoi(buf) < 1998)
				newinfo.birthyear = atoi(buf) - 1900;

			sprintf(genbuf, "������ [%d]: ", u->birthmonth);
			getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
			if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 12)
				newinfo.birthmonth = atoi(buf);

			sprintf(genbuf, "������ [%d]: ", u->birthday);
			getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
			if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 31)
				newinfo.birthday = atoi(buf);

			/* add by money 2003.10.24 for 2.28/29 test */
			if (newinfo.birthmonth == 2) {
				if (((newinfo.birthyear+1900) % 4) == 0) {
					if (((newinfo.birthyear+1900) % 100) != 0)
						r = 1;
					else if (((newinfo.birthyear+1900) % 400) == 0)
						r = 1;
				}
				if (r) {
					if (newinfo.birthday > 29)
						newinfo.birthday = 29;
				} else {
					if (newinfo.birthday > 28)
						newinfo.birthday = 28;
				}
			}

			if ((newinfo.birthmonth<7)&&(!(newinfo.birthmonth%2))
					&&(newinfo.birthday>30))
				newinfo.birthday = 30;
			if ((newinfo.birthmonth>8)&&(newinfo.birthmonth%2)
					&&(newinfo.birthday>30))
				newinfo.birthday = 30;
			/* add end */

			sprintf(genbuf, "�Ա�(M.��)(F.Ů) [%c]: ", u->gender);
			getdata(i++, 0, genbuf, buf, 2, DOECHO, YEA);
			if (buf[0]) {
				if (strchr("MmFf", buf[0]))
					newinfo.gender = toupper(buf[0]);
			}

			if (real)
				uinfo_change1(i, u, &newinfo);
			break;
		case '2':
			if (!real) {
				getdata(i++, 0, "������ԭ����: ", buf, PASSLEN, NOECHO, YEA);
				if (*buf == '\0' || !checkpasswd(u->passwd, buf)) {
					prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
					fail++;
					break;
				}
			}
			while (1) {
				getdata(i++, 0, "���趨������: ", buf, PASSLEN, NOECHO, YEA);
				if (buf[0] == '\0') {
					prints("\n\n�����趨ȡ��, ����ʹ�þ�����\n");
					fail++;
					break;
				}
				if (strlen(buf) < 4 || !strcmp(buf, u->userid)) {
					prints("\n\n����̫�̻���ʹ���ߴ�����ͬ, �����趨ȡ��, ����ʹ�þ�����\n");
					fail++;
					break;
				}
				strlcpy(genbuf, buf, PASSLEN);
				getdata(i++, 0, "����������������: ", buf, PASSLEN, NOECHO, YEA);
				if (strncmp(buf, genbuf, PASSLEN)) {
					prints("\n\n������ȷ��ʧ��, �޷��趨�����롣\n");
					fail++;
					break;
				}
				buf[8] = '\0';
				strlcpy(newinfo.passwd, genpasswd(buf), ENCPASSLEN);
				break;
			}
			break;
		case '3':
			if (!real) {
				sprintf(genbuf, "Ŀǰʹ��ǩ���� [%d]: ", u->signature);
				getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
				if (atoi(buf) >= 0)
					newinfo.signature = atoi(buf);
			}
			break;
		default:
			clear();
			return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("ȷ��Ҫ�ı���", NA, YEA) == YEA) {
		if (real) {
			char secu[STRLEN];
			sprintf(secu, "�޸� %s �Ļ������ϻ����롣", u->userid);
			securityreport(secu, 0, 0);
		}
		if (strcmp(u->userid, newinfo.userid)) {
			sprintf(src, "mail/%c/%s", toupper(u->userid[0]), u->userid);
			sprintf(dst, "mail/%c/%s", toupper(newinfo.userid[0]),
					newinfo.userid);
			rename(src, dst);
			sethomepath(src, u->userid);
			sethomepath(dst, newinfo.userid);
			rename(src, dst);
			sethomefile(src, u->userid, "register");
			unlink(src);
			sethomefile(src, u->userid, "register.old");
			unlink(src);
			setuserid(unum, newinfo.userid);
		}
		if (!strcmp(u->userid, currentuser.userid)) {
			extern int WishNum;
			strlcpy(uinfo.username, newinfo.username, NAMELEN);
			WishNum = 9999;
		}
		memcpy(u, &newinfo, (size_t)sizeof(currentuser));
		substitut_record(PASSFILE, &newinfo, sizeof(newinfo), unum);
	}
	clear();
	return 0;
}

//��Information�����.��comm_list.c��,������ʾ���趨��������
void x_info() {
	if (!strcmp("guest", currentuser.userid))
		return;
	modify_user_mode(GMENU);
	disply_userinfo(&currentuser);
	uinfo_query(&currentuser, 0, usernum);
}

//	�����û�������ĳ������Ӧ�趨
void getfield(int line, char *info, char *desc, char *buf, int len) {
	char prompt[STRLEN];
	sprintf(genbuf, "  ԭ���趨: %-40.40s [1;32m(%s)[m",
			(buf[0] == '\0') ? "(δ�趨)" : buf, info);
	move(line, 0);
	prints("%s", genbuf);
	sprintf(prompt, "  %s: ", desc);
	getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
	if (genbuf[0] != '\0')
		strlcpy(buf, genbuf, len);
	move(line, 0);
	clrtoeol();
	prints("  %s: %s\n", desc, buf);
	clrtoeol();
}

#include "fbbs/register.h"
//	��д�û�����
void x_fillform() {
	char ans[5], *mesg, *ptr;
	reginfo_t ri;
	FILE *fn;

	if (!strcmp("guest", currentuser.userid))
		return;
	modify_user_mode(NEW);
	clear();
	move(2, 0);
	clrtobot();
	if (currentuser.userlevel & PERM_REGISTER) {
		prints("���Ѿ���ɱ�վ��ʹ����ע������, ��ӭ���뱾վ������.");
		pressreturn();
		return;
	}
#ifdef PASSAFTERTHREEDAYS
	if (currentuser.lastlogin - currentuser.firstlogin < 3 * 86400) {
		prints("���״ε��뱾վδ������(72��Сʱ)...\n");
		prints("�����Ĵ���Ϥһ�£����������Ժ�����дע�ᵥ��");
		pressreturn();
		return;
	}
#endif      
	if ((fn = fopen("unregistered", "rb")) != NULL) {
		while (fread(&ri, sizeof(ri), 1, fn)) {
			if (!strcasecmp(ri.userid, currentuser.userid)) {
				fclose(fn);
				prints("վ����δ��������ע�����뵥, ���ȵ���������.");
				pressreturn();
				return;
			}
		}
		fclose(fn);
	}

	memset(&ri, 0, sizeof(ri));
	strlcpy(ri.userid, currentuser.userid, IDLEN+1);
	strlcpy(ri.email, currentuser.email, STRLEN-12);
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s ����, ���ʵ��д���µ�����:\n", currentuser.userid);
		do {
			getfield(6, "��������", "��ʵ����", ri.realname, NAMELEN);
		} while (strlen(ri.realname)<4);

		do {
			getfield(8, "ѧУϵ�������ڵ�λ", "ѧУϵ��", ri.dept, STRLEN);
		} while (strlen(ri.dept)< 6);

		do {
			getfield(10, "�������һ����ƺ���", "Ŀǰסַ", ri.addr, STRLEN);
		} while (strlen(ri.addr)<10);

		do {
			getfield(12, "����������ʱ��", "����绰", ri.phone, STRLEN);
		} while (strlen(ri.phone)<8);

		getfield(14, "У�ѻ���ҵѧУ", "У �� ��", ri.assoc, STRLEN);
		mesg = "���������Ƿ���ȷ, �� Q ����ע�� (Y/N/Quit)? [Y]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return;
		if (ans[0] != 'N' && ans[0] != 'n')
			break;
	}
	ptr = ri.realname;
	filter_ff(ptr);
	ptr = ri.addr;
	filter_ff(ptr);
	ptr = ri.dept;
	filter_ff(ptr);
#ifndef FDQUAN
	strlcpy(currentuser.email, ri.email, STRLEN-12);
#endif	
	if ((fn = fopen("unregistered", "ab")) != NULL) {
		ri.regdate= time(NULL);
		fwrite(&ri, sizeof(ri), 1, fn);
		fclose(fn);
	}
	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) == NULL) {
		fclose(fn);
		return;
	}
	fprintf(fn, "usernum: %d\n", usernum);
	fclose(fn);
}
// deardragon 2000.09.26 over

