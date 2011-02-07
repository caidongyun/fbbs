#ifndef FB_TIME_H
#define FB_TIME_H

#include <time.h>

enum {
	DATE_ZH = 0,      ///< "2001年02月03日04:05:06 星期六"
	DATE_EN = 1,      ///< "02/03/01 04:05:06"
	DATE_SHORT = 2,   ///< "02.03 04:05"
	DATE_ENWEEK = 4,  ///< "02/03/01 04:05:06 Sat"
	DATE_XML = 8,     ///< "2001-02-03T04:05:06"
	DATE_RSS = 16,    ///< "Sat,03 Feb 2001 04:05:06 +0800"
};

extern const char *date2str(time_t time, int mode);

#endif // FB_TIME_H
