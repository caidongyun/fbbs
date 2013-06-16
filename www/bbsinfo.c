#include "libweb.h"
#include "record.h"
#include "fbbs/helper.h"
#include "fbbs/string.h"
#include "fbbs/web.h"

/**
 * Check user info validity.
 * @return empty string on success, error msg otherwise.
 */
static char *check_info(void)
{
	unsigned char *nick;
	nick = (unsigned char *)get_param("nick");
	unsigned char *t2 = nick;
	while (*t2 != '\0') {
		if (*t2 < 0x20 || *t2 == 0xFF)
			//% return "昵称太短或包含非法字符";
			return "\xea\xc7\xb3\xc6\xcc\xab\xb6\xcc\xbb\xf2\xb0\xfc\xba\xac\xb7\xc7\xb7\xa8\xd7\xd6\xb7\xfb";
		t2++;
	}
	strlcpy(currentuser.username, (char *)nick, sizeof(currentuser.username));

	// TODO: more accurate birthday check.
	const char *tmp = get_param("year");
	long num = strtol(tmp, NULL, 10);
	if (num < 1910 || num > 1998)
		//% return "错误的出生年份";
		return "\xb4\xed\xce\xf3\xb5\xc4\xb3\xf6\xc9\xfa\xc4\xea\xb7\xdd";
	else
		currentuser.birthyear = num - 1900;

	tmp = get_param("month");
	num = strtol(tmp, NULL, 10);
	if (num <= 0 || num > 12)
		//% return "错误的出生月份";
		return "\xb4\xed\xce\xf3\xb5\xc4\xb3\xf6\xc9\xfa\xd4\xc2\xb7\xdd";
	else
		currentuser.birthmonth = num;

	tmp = get_param("day");
	num = strtol(tmp, NULL, 10);
	if (num <= 0 || num > 31)
		//% return "错误的出生日期";
		return "\xb4\xed\xce\xf3\xb5\xc4\xb3\xf6\xc9\xfa\xc8\xd5\xc6\xda";
	else
		currentuser.birthday = num;

	tmp = get_param("gender");
	if (*tmp == 'M')
		currentuser.gender = 'M';
	else
		currentuser.gender = 'F';

	save_user_data(&currentuser);
	return "";
}

int bbsinfo_main(void)
{
	if (!session.id)
		return BBS_ELGNREQ;
	parse_post_data();
	const char *type = get_param("type");
	xml_header(NULL);
	if (*type != '\0') {
		printf("<bbsinfo>");
		print_session();
		printf("%s</bbsinfo>", check_info());
	} else {
		printf("<bbsinfo post='%d' login='%d' stay='%d' "
				"since='%s' host='%s' year='%d' month='%d' "
				"day='%d' gender='%c' ", currentuser.numposts,
				currentuser.numlogins, currentuser.stay / 60,
				format_time(currentuser.firstlogin, TIME_FORMAT_XML),
				currentuser.lasthost, currentuser.birthyear,
				currentuser.birthmonth, currentuser.birthday,
				currentuser.gender);
		printf(" last='%s'><nick>",
				format_time(currentuser.lastlogin, TIME_FORMAT_XML));
		xml_fputs(currentuser.username, stdout);
		printf("</nick>");
		print_session();
		printf("</bbsinfo>");
	}
	return 0;
}

static int set_password(const char *orig, const char *new1, const char *new2)
{
	if (!passwd_check(currentuser.userid, orig))
		return BBS_EWPSWD;

	if (!streq(new1, new2))
		return BBS_EINVAL;

	if (strlen(new1) < 2)
		return BBS_EINVAL;

	return (passwd_set(currentuser.userid, new1) == 0 ? 0 : BBS_EINTNL);
}

int bbspwd_main(void)
{
	if (!session.id)
		return BBS_ELGNREQ;
	parse_post_data();
	xml_header(NULL);
	printf("<bbspwd ");
	const char *pw1 = get_param("pw1");
	if (*pw1 == '\0') {
		printf(" i='i'>");
		print_session();
		printf("</bbspwd>");
		return 0;
	}
	printf(">");
	const char *pw2 = get_param("pw2");
	const char *pw3 = get_param("pw3");
	switch (set_password(pw1, pw2, pw3)) {
		case BBS_EWPSWD:
			//% printf("密码错误");
			printf("\xc3\xdc\xc2\xeb\xb4\xed\xce\xf3");
			break;
		case BBS_EINVAL:
			//% printf("新密码不匹配 或 新密码太短");
			printf("\xd0\xc2\xc3\xdc\xc2\xeb\xb2\xbb\xc6\xa5\xc5\xe4 \xbb\xf2 \xd0\xc2\xc3\xdc\xc2\xeb\xcc\xab\xb6\xcc");
			break;
		case BBS_EINTNL:
			//% printf("内部错误");
			printf("\xc4\xda\xb2\xbf\xb4\xed\xce\xf3");
		default:
			break;
	}
	print_session();
	printf("</bbspwd>");
	return 0;
}

typedef struct mail_count_t {
	time_t limit;
	int count;
} mail_count_t;

static int count_new_mail(void *buf, int count, void *args)
{
	struct fileheader *fp = buf;
	mail_count_t *cp = args;
	time_t date = getfiletime(fp);
	if (date < cp->limit)
		return QUIT;
	if (!(fp->accessed[0] & FILE_READ))
		cp->count++;
	return 0;
}

int bbsidle_main(void)
{
	if (!session.id)
		return BBS_ELGNREQ;
	printf("Content-type: text/xml; charset="CHARSET"\n\n"
			"<?xml version=\"1.0\" encoding=\""CHARSET"\"?>\n<bbsidle");
	char file[HOMELEN];
	setmdir(file, currentuser.userid);
	mail_count_t c;
	c.limit = time(NULL) - 24 * 60 * 60 * NEWMAIL_EXPIRE;
	c.count = 0;
	apply_record(file, count_new_mail, sizeof(struct fileheader), &c, false,
			true, true);
	printf(" mail='%d'></bbsidle>", c.count);
	return 0;
}
