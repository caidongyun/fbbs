#include "site.h"
#include <stdbool.h>
#include "fbbs/session.h"

/**
 * Get raw user mode.
 * @return user mode without web mask.
 */
int get_raw_mode(int mode)
{
	return (mode & 0xffff);
}

/**
 * Get descriptions of user mode.
 * @param mode user mode.
 * @return a string describing the mode.
 */
const char *mode_type(int mode)
{
	switch (get_raw_mode(mode)) {
		case ST_IDLE:
			return "��������";
		case ST_NEW:
			return "��վ��ע��";
		case ST_LOGIN:
			return "���뱾վ";
		case ST_DIGEST:
			return "��ȡ����";
		case ST_MMENU:
			return "�δ��";
		case ST_ADMIN:
			return "��·����";
		case ST_SELECT:
			return "ѡ��������";
		case ST_READBRD:
			return "��������";
		case ST_READNEW:
			return "��������";
		case ST_READING:
			return "Ʒζ����";
		case ST_POSTING:
			return "�ĺ��ӱ�";
		case ST_MAIL:
			return "�����ż�";
		case ST_SMAIL:
			return "�����Ÿ�";
		case ST_RMAIL:
			return "�����ż�";
		case ST_TMENU:
			return "��ȵ��";
		case ST_LUSERS:
			return "�����ķ�";
		case ST_FRIEND:
			return "ҹ̽����";
		case ST_MONITOR:
			return "̽������";
		case ST_QUERY:
			return "��ѯ����";
		case ST_TALK:
			return "ȵ��ϸ��";
		case ST_PAGE:
			return "��ϿԳ��";
		case ST_CHAT2:
			return "��԰ҹ��";
		case ST_CHAT1:
			return "��԰ҹ��";
		case ST_LAUSERS:
			return "̽������";
		case ST_XMENU:
			return "ϵͳ��Ѷ";
		case ST_BBSNET:
#ifdef FDQUAN
			return "��Ȫ����";
#else
			return "������Ȫ";
#endif
		case ST_EDITUFILE:
			return "�༭���˵�";
		case ST_EDITSFILE:
			return "���ֶ���";
		case ST_SYSINFO:
			return "���ϵͳ";
		case ST_DICT:
			return "�����ֵ�";
		case ST_LOCKSCREEN:
			return "��Ļ����";
		case ST_NOTEPAD:
			return "���԰�";
		case ST_GMENU:
			return "������";
		case ST_MSG:
			return "��ѶϢ";
		case ST_USERDEF:
			return "�Զ�����";
		case ST_EDIT:
			return "�޸�����";
		case ST_OFFLINE:
			return "��ɱ��..";
		case ST_EDITANN:
			return "���޾���";
		case ST_LOOKMSGS:
			return "�鿴ѶϢ";
		case ST_WFRIEND:
			return "Ѱ������";
		case ST_WNOTEPAD:
			return "���߻���";
		case ST_BBSPAGER:
			return "��·����";
		case ST_M_BLACKJACK:
			return "��ڼ׿ˡ�";
		case ST_M_XAXB:
			return "������֡�";
		case ST_M_DICE:
			return "����������";
		case ST_M_GP:
			return "���˿����";
		case ST_M_NINE:
			return "��ؾž�";
		case ST_WINMINE:
			return "����ɨ��";
		case ST_M_BINGO:
			return "��������";
		case ST_FIVE:
			return "��ս������";
		case ST_MARKET:
			return "�����г�";
		case ST_PAGE_FIVE:
			return "��������";
		case ST_CHICK:
			return "����С��";
		case ST_MARY:
			return "��������";
		case ST_CHICKEN:
			return "�ǿ�ս����";
		case ST_GOODWISH:
			return "������ף��";
		case ST_GIVEUPBBS:
			return "������";
		case ST_UPLOAD:
			return "�ϴ��ļ�";
		case ST_PROP:
			return "�۱���";
		case ST_MY_PROP:
			return "�ؾ���";
		default:
			return "ȥ������!?";
	}
}

/**
 * Get whether user is web browsing.
 * @param mode user mode.
 * @return true if web browsing, false otherwise.
 */
bool is_web_user(int mode)
{
	return (mode & ST_WWW);
}

/**
 * Get web mode.
 * @param mode user mode.
 * @return correspoding web mode.
 */
int get_web_mode(int mode)
{
	return (mode | ST_WWW);
}

