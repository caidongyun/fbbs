#ifndef FB_POST_H
#define FB_POST_H

#include "fbbs/convert.h"

#define ANONYMOUS_ACCOUNT "Anonymous"
#define ANONYMOUS_NICK    "����������ʹ"
#define ANONYMOUS_SOURCE  "������ʹ�ļ�"

enum {
	QUOTE_NOTHING = 'N',
	QUOTE_AUTO = 'R',
	QUOTE_LONG = 'Y',
	QUOTE_SOURCE = 'S',
	QUOTE_ALL = 'A',
};

typedef struct {
	bool autopost;
	bool crosspost;
	const char *userid;
	const char *nick;
	const struct userec *user;
	const struct boardheader *bp;
	const char *title;
	const char *content;
	int sig;
	const char *ip;
	const struct fileheader *o_fp;
	bool noreply;
	bool mmark;
	bool anony;
	convert_t *cp;
} post_request_t;

extern unsigned int do_post_article(const post_request_t *pr);

extern void quote_string(const char *str, size_t size, const char *output,
		int mode, bool mail, size_t (*filter)(const char *, size_t, FILE *));
extern void quote_file_(const char *orig, const char *output, int mode,
		bool mail, size_t (*filter)(const char *, size_t, FILE *));

#endif // FB_POST_H

