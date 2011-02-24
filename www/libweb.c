/*
 *  Fdubbs Project
 *  See COPYRIGHT for more details.
 */
#include <math.h>

#include "libweb.h"
#include "mmap.h"
#include "fbbs/string.h"
#include "fbbs/web.h"

char seccode[SECNUM][6]={
#ifdef FDQUAN
	"0ab","cd","ij","kl","mn","op","qr","st","uv"
#else
	"0oOhH", "1pP", "2qQ", "3rRhH", "4sS", "5tT", "6uU", "7vV", "8wW", "9xX", "ayY", "bzZ"
#endif
};

char secname[SECNUM][2][20] = {
#ifdef FDQUAN
	{"BBS ϵͳ", "[վ��]"},
	{"������ѧ", "[��У]"},
	{"�⻪��˾", "[����]"},	
	{"������֯", "[����]"},
	{"�����", "[��ʱ]"},
	{"̸���ĵ�", "[����]"},
	{"��Ϸ����", "[����]"},
	{"���˷��", "[����]"},
	{"������̳", "[����]"}
#else
	{"BBS ϵͳ", "[վ��]"},
	{"������ѧ", "[��У]"},
	{"Ժϵ���", "[У԰]"},
	{"���Լ���", "[����]"},
	{"��������", "[����]"},
	{"��ѧ����", "[����]"},
	{"��������", "[�˶�]"},
	{"���Կռ�", "[����]"},
	{"������Ϣ", "[����]"},
	{"ѧ��ѧ��", "[ѧ��]"},
	{"Ӱ������", "[Ӱ��]"},
	{"����ר��", "[����]"}
#endif
};

int loginok = 0;
struct userec currentuser;
struct user_info *u_info;
char fromhost[IP_LEN]; // IPv6 addresses can be represented in 39 chars.

/**
 * Get an environment variable.
 * The function searches environment list where FastCGI stores parameters
 * for a string that matches the string pointed by s. The strings are of 
 * the form name=value.
 * @param s the name
 * @return a pointer to the value in the environment, or empty string if
 *         there is no match
 */
const char *getsenv(const char *s)
{
	char *t = getenv(s);
	if (t!= NULL)
		return t;
	return "";
}

/**
 * Get referrer of the request.
 * @return the path related to the site root, an empty string on error.
 */
const char *get_referer(void)
{
	const char *r = getsenv("HTTP_REFERER");
	int i = 3;
	if (r != NULL) {
		// http://host/path... let's find the third slash
		while (i != 0 && *r != '\0') {
			if (*r == '/')
				i--;
			r++;
		}
	}
	if (i == 0)
		return --r;
	return "";
}

/**
 * Print XML escaped string.

 * @param s string to print
 * @param size maximum output bytes. If 0, print until NUL is encountered.
 * @param stream output stream
 * @note '<' => "&lt;" '&' => "&amp;" '>' => "&gt;" '\\033' => ">1b"
 *       '\\r' => ""
 *       ESC('\\033') is not allowed in XML 1.0. We have to use a workaround 
 *       for compatibility with ANSI escape sequences.
 */
void xml_fputs2(const char *s, size_t size, FILE *stream)
{
	char *c = (char *)s; // To fit FastCGI prototypes..
	char *last = c;
	char *end = c + size;
	char *subst;
	if (size == 0)
		end = NULL;
	while (c != end && *c != '\0') {
		switch (*c) {
			case '<':
				subst = "&lt;";
				break;
			case '>':
				subst = "&gt;";
				break;
			case '&':
				subst = "&amp;";
				break;
			case '\033':
				subst = ">1b";
				break;
			case '\r':
				subst = "";
				break;
			default:
				subst = NULL;
				break;
		}
		if (subst != NULL) {
			fwrite(last, sizeof(char), c - last, stream);
			while (*subst != '\0')
				fputc(*subst++, stream);
			last = ++c;
		} else {
			++c;
		}
	}
	fwrite(last, sizeof(char), c - last, stream);
}

/**
 * Print XML escaped string.
 * @param s string to print (should be NUL terminated)
 * @param stream output stream
 * @see xml_fputs2
 */
void xml_fputs(const char *s, FILE *stream)
{
	xml_fputs2(s, 0, stream);
}

/**
 * Print a file with XML escaped.
 * @param file filename to print
 * @param stream output stream
 * @see xml_fputs2
 */
int xml_printfile(const char *file, FILE *stream)
{
	if (file == NULL || stream == NULL)
		return -1;
	mmap_t m;
	m.oflag = O_RDONLY;
	if (mmap_open(file, &m) < 0)
		return -1;
	if (m.size > 0)
		xml_fputs2((char *)m.ptr, m.size, stream);
	mmap_close(&m);
	return 0;
}

/**
 * Parse HTTP header and get client IP address.
 * @return 0
 */
static int http_init(void)
{
#ifdef SQUID
	char *from;
	from = strrchr(getsenv("HTTP_X_FORWARDED_FOR"), ',');
	if (from == NULL) {
		strlcpy(fromhost, getsenv("HTTP_X_FORWARDED_FOR"), sizeof(fromhost));
	} else {
		while ((*from < '0') && (*from != '\0'))
			from++;
		strlcpy(fromhost, from, sizeof(fromhost));
	}
#else
	strlcpy(fromhost, getsenv("REMOTE_ADDR"), sizeof(fromhost));
#endif
	return 0;
}

/**
 * Load user information from cookie.
 * If everything is OK, initializes @a x and @a y.
 * Otherwise, @a x is cleared and @a y is set to NULL.
 * @param[in,out] x pointer to user infomation struct.
 * @param[in,out] y pointer to pointer to user online cache.
 * @param[in] mode user mode.
 * @return 1 on valid user login, 0 on error.
 */
 // TODO: no lock?
static int user_init(web_ctx_t *ctx, struct userec *x, struct user_info **y, int mode)
{
	memset(x, 0, sizeof(*x));

	// Get information from cookie.
	const char *id = get_param(ctx->r, "utmpuserid");
	int i = strtol(get_param(ctx->r, "utmpnum"), NULL, 10);
	int key = strtol(get_param(ctx->r, "utmpkey"), NULL, 10);

	// Boundary check.
	if (i <= 0 || i > MAXACTIVE) {
		*y = NULL;
		return 0;
	}
	// Get user_info from utmp.
	(*y) = &(utmpshm->uinfo[i - 1]);

	// Verify cookie and user status.
	if (strcmp((*y)->from, fromhost)
			|| (*y)->utmpkey != key
			|| (*y)->active == 0
			|| (*y)->userid[0] == '\0'
			|| !is_web_user((*y)->mode)) {
		*y = NULL;
		return 0;
	}

	// If not normal user.
	if (!strcasecmp((*y)->userid, "new")
			|| !strcasecmp((*y)->userid, "guest")) {
		*y = NULL;
		return 0;
	}

	// Get userec from ucache.
	getuserbyuid(x, (*y)->uid);
	if (strcmp(x->userid, id)) {
		memset(x, 0, sizeof(*x));
		*y = NULL;
		return 0;
	}

	// Refresh idle time, set user mode.
	(*y)->idle_time = time(NULL);
	if (get_web_mode(IDLE) != mode)
		(*y)->mode = mode;

	return 1;
}

/**
 * Initialization inside a FastCGI loop.
 * @param mode user mode.
 * @return 0
 */
// TODO: return value?
int fcgi_init_loop(web_ctx_t *ctx, int mode)
{
	http_init();
	loginok = user_init(ctx, &currentuser, &u_info, mode);
	return 0;
}

/**
 * Print HTML response header.
 */
void http_header(void)
{
	printf("Content-type: text/html; charset=%s\n\n", CHARSET);
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
			"\"http://www.w3.org/TR/html4/strict.dtd\"><html><head>");
}

/**
 * HTML code for redirecting/refreshing.
 * @param second seconds before reloading
 * @param url URL to load
 */
void refreshto(int second, const char *url)
{
	printf("<meta http-equiv='Refresh' content='%d; url=%s' />\n", second, url);
}

// Convert exp to icons.
int iconexp(int exp, int *repeat)
{
	int i = 0, j;

	if (exp < 0)
		j = -1;
	else {
		exp = sqrt(exp / 5);
		i = exp / 10;
		i = i > 5 ? 5 : i;
		j = exp - i * 10;
		j = j > 9 ? 9 : j;
	}
	*repeat = ++j;
	return i;
}

int save_user_data(struct userec *x) {
	int n;
	n = searchuser(x->userid) - 1;
	if(n < 0 || n >= MAXUSERS)
		return 0;
	memcpy( &(uidshm->passwd[n]), x, sizeof(struct userec) );
	return 1;
}

int user_perm(struct userec *x, int level) {
	return (level?x->userlevel & level:1);
}

// TODO: put into memory
int maxlen(const char *board)
{
	char path[HOMELEN];
	int	limit = UPLOAD_MAX;
	snprintf(path, sizeof(path), BBSHOME"/upload/%s/.maxlen", board);
	FILE *fp = fopen(path, "r");
	if (fp != NULL) {
		if (fscanf(FCGI_ToFILE(fp), "%d", &limit) <= 0)
			limit = UPLOAD_MAX;
		fclose(fp);
	}
	return limit;
}

// Get file time according to its name 's'.
time_t getfiletime(const struct fileheader *f)
{
	if (f->filename[0] == 's') // sharedmail..
		return strtol(f->filename + strlen(f->owner) + 20, NULL, 10);
	return strtol(f->filename + 2, NULL, 10);
}

struct fileheader *bbsmail_search(const void *ptr, size_t size, const char *file)
{
	if (ptr == NULL || file == NULL)
		return NULL;
	int total = size / sizeof(struct fileheader);
	if (total < 1)
		return NULL;
	// linear search from the end.
	// TODO: unique id should be added to speed up search.
	struct fileheader *begin = (struct fileheader *)ptr;
	struct fileheader *last = begin + total - 1;
	for (struct fileheader *fh = last; fh >= begin; --fh) {
		if (!strcmp(fh->filename, file))
			return fh;
	}
	return NULL;
}

bool valid_mailname(const char *file)
{
	if (!strncmp(file, "sharedmail/", 11)) {
		if (strstr(file + 11, "..") || strchr(file + 11, '/'))
			return false;
	} else {
		if (strncmp(file, "M.", 2) || strstr(file, "..") || strchr(file, '/'))
			return false;
	}
	return true;
}

static char *get_permission(void)
{
	static char c[5];
	c[0] = loginok ? 'l' : ' ';
	c[1] = HAS_PERM(PERM_TALK) ? 't' : ' ';
	c[2] = HAS_PERM(PERM_CLOAK) ? '#': ' ';
	c[3] = HAS_PERM(PERM_OBOARDS) && HAS_PERM(PERM_SPECIAL0) ? 'f' : ' ';
	c[4] = '\0';
	return c;
}

int get_doc_mode(void)
{
	return currentuser.flags[1];
}

void set_doc_mode(int mode)
{
	if (loginok)
		uidshm->passwd[u_info->uid - 1].flags[1] = mode;
}

const char *get_doc_mode_str(void)
{
	if (!loginok)
		return "";
	switch (uidshm->passwd[u_info->uid - 1].flags[1]) {
		case MODE_THREAD:
			return "t";
		default:
			return "";
	}
}

void print_session(web_ctx_t *ctx)
{
	if (ctx->r->flag & REQUEST_API)
		return;
	bool mobile = ctx->r->flag & REQUEST_MOBILE;

	printf("<session m='%s'><p>%s</p><u>%s</u><f>", get_doc_mode_str(),
			get_permission(), currentuser.userid);

	char file[HOMELEN];
	sethomefile(file, currentuser.userid, ".goodbrd");
	mmap_t m;
	m.oflag = O_RDONLY;
	if (mmap_open(file, &m) == 0) {
		struct goodbrdheader *iter, *end;
		int num = m.size / sizeof(struct goodbrdheader);
		if (num > GOOD_BRC_NUM)
			num = GOOD_BRC_NUM;
		end = (struct goodbrdheader *)m.ptr + num;
		for (iter = m.ptr; iter != end; ++iter) {
			if (!gbrd_is_custom_dir(iter)) {
				struct boardheader *bp = bcache + iter->pos;
				printf("<b");
				if (!isascii(bp->filename[0]))
					printf(" bid='%d'", iter->pos + 1);
				if (mobile && !brc_board_unread(currentuser.userid, bp))
					printf(" r='1'");
				printf(">%s</b>", bp->filename);
			}
		}
		mmap_close(&m);
	}

	printf("</f></session>");
}


