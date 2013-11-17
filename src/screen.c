#include "bbs.h"
#include "fbbs/string.h"
#include "fbbs/terminal.h"

/** ANSI指令，清屏 */
#define ANSI_CMD_CL "\033[H\033[J"

/** ANSI指令，从光标处删除到行尾 */
#define ANSI_CMD_CE "\033[K"

/** ANSI指令，向上滚动一行 */
#define ANSI_CMD_SR "\033M"

/** ANSI指令，进入反色模式 */
#define ANSI_CMD_SO "\033[7m"

/** ANSI指令，退出反色模式 */
#define ANSI_CMD_SE "\033[m"

/** 输出一条ANSI指令 */
#define ansi_cmd(cmd)  \
	terminal_write_cached((unsigned char *) cmd, sizeof(cmd) - 1)

/* Maximum Screen width in chars */
#define LINELEN (1024)

/* Line buffer modes             */
#define MODIFIED (1)   /* if line has been modifed, output to screen   */
#define STANDOUT (2)   /* if this line has a standout region */

#define Min(a,b) ((a<b)?a:b)
#define Max(a,b) ((a>b)?a:b)

struct screenline {
	unsigned int oldlen; /* previous line length              */
	unsigned int len; /* current length of line            */
	unsigned char mode; /* status of line, as far as update  */
	unsigned char smod; /* start of modified data            */
	unsigned char emod; /* end of modified data              */
	unsigned char sso; /* start stand out */
	unsigned char eso; /* end stand out */
	unsigned char data[LINELEN];
};

extern int iscolor;

bool dumb_term = true;
bool automargins = true;
int t_lines = 24;          ///< Terminal height.
int t_columns = 255;       ///< Terminal width.

static int scr_lns;     ///< Lines of the screen.
unsigned int scr_cols;  ///< Columns of the screen.
static int cur_ln = 0;  ///< Current line.
static int cur_col = 0; ///< Current column.
static int roll; //roll 表示首行在big_picture的偏移量
//因为随着光标滚动,big_picture[0]可能不再保存第一行的数据
static int scrollcnt;
static int tc_col;      ///< Terminal's current column.
static int tc_line;     ///< Terminal's current line.
unsigned char docls;
unsigned char downfrom;
static bool standing = false;

struct screenline *big_picture = NULL;

//返回字符串中属于 ansi的个数?	对后一个continue不太理解 
int num_ans_chr(const char *str)
{
	int len, i, ansinum, ansi;

	ansinum=0;
	ansi=NA;
	len=strlen(str);
	for (i=0; i < len; i++) {
		if (str[i] == KEY_ESC) {
			ansi = YEA;
			ansinum++;
			continue;
		}
		if (ansi) {
			if (!strchr("[0123456789; ", str[i]))
				ansi = NA;
			ansinum++;
			continue;
		}
	}
	return ansinum;
}

/**
 * Initialize screen.
 * @param slns Lines of the screen, ::LINELEN max.
 * @param scols Columns of the screen.
 */
static void init_screen(int slns, int scols)
{
	struct screenline *slp;
	scr_lns = slns;
	scr_cols = Min(scols, LINELEN);
	big_picture = calloc(scr_lns, sizeof(*big_picture));
	for (slns = 0; slns < scr_lns; slns++) {
		slp = big_picture + slns;
		slp->mode = 0;
		slp->len = 0;
		slp->oldlen = 0;
	}
	docls = YEA;
	downfrom = 0;
	roll = 0;
}

//对于哑终端或是big_picture中尚无内存映射,将t_columns设置成WRAPMARGIN
//	调用init_screen初始化终端
void initscr(void)
{
	if (!dumb_term && !big_picture)
		t_columns = WRAPMARGIN;
	init_screen(t_lines, WRAPMARGIN);
}

/**
 * Generate and send terminal move cmd.
 * @param col Column to move to.
 * @param line Line to move to.
 */
static void do_move(int col, int line)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "\033[%d;%dH", line + 1, col + 1);
	char *p;
	for (p = buf; *p != '\0'; p++)
		terminal_putchar(*p);
}

//	从老位置(was_col,was_ln)移动到新位置(new_col,new_ln)
static void rel_move(int was_col, int was_ln, int new_col, int new_ln)
{
	if (new_ln >= t_lines || new_col >= t_columns) //越界,返回
		return;
	tc_col = new_col;
	tc_line = new_ln;
	if ((new_col == 0) && (new_ln == was_ln + 1)) { //换行
		terminal_putchar('\n');
		if (was_col != 0) //到第一列位置,返回
			terminal_putchar('\r');
		return;
	}
	if ((new_col == 0) && (new_ln == was_ln)) { //不换行,到第一列位置,并返回
		if (was_col != 0)
			terminal_putchar('\r');
		return;
	}
	if (was_col == new_col && was_ln == new_ln)
		return;
	if (new_col == was_col - 1 && new_ln == was_ln) {
		terminal_putchar(Ctrl('H'));
		return;
	}
	do_move(new_col, new_ln);
}

// 标准输出buf中的数据,	ds,de表示数据的区间,sso,eso也是
//		但当它们没有交集时,以ds,de为准
//		有交集时,取合集
//			但下限以ds为准,上限以de为准				跟直接取ds,de有什么区别?
///		对o_standup,o_standdown作用不太清楚
static void standoutput(unsigned char *buf, int ds, int de, int sso, int eso)
{
	int st_start, st_end;
	if (eso <= ds || sso >= de) {
		terminal_write_cached(buf + ds, de - ds);
		return;
	}
	st_start = Max(sso, ds);
	st_end = Min(eso, de);
	if (sso > ds)
		terminal_write_cached(buf + ds, sso - ds);
	ansi_cmd(ANSI_CMD_SO);
	terminal_write_cached(buf + st_start, st_end - st_start);
	ansi_cmd(ANSI_CMD_SE);
	if (de > eso)
		terminal_write_cached(buf + eso, de - eso);
}

/**
 * Redraw the screen.
 */
void redoscr(void)
{
	if (dumb_term)
		return;
	ansi_cmd(ANSI_CMD_CL);
	tc_col = 0;
	tc_line = 0;
	int i;
	struct screenline *s;
	for (i = 0; i < scr_lns; i++) {
		s = big_picture + (i + roll) % scr_lns;
		if (s->len == 0)
			continue;
		rel_move(tc_col, tc_line, 0, i);
		if (s->mode & STANDOUT)
			standoutput(s->data, 0, s->len, s->sso, s->eso);
		else
			terminal_write_cached(s->data, s->len);
		tc_col += s->len;
		if (tc_col >= t_columns) {
			if (!automargins) {
				tc_col -= t_columns;
				tc_line++;
				if (tc_line >= t_lines)
					tc_line = t_lines - 1;
			} else {
				tc_col = t_columns - 1;
			}
		}
		s->mode &= ~(MODIFIED);
		s->oldlen = s->len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	docls = NA;
	scrollcnt = 0;
	terminal_flush();
}

//刷新缓冲区,重新显示屏幕?
void refresh(void)
{
	register int i, j;
	register struct screenline *bp = big_picture;
	if (!terminal_input_buffer_empty())
		return;
	if ((docls) || (abs(scrollcnt) >= (scr_lns - 3))) {
		redoscr();
		return;
	}
	if (scrollcnt < 0) {
		rel_move(tc_col, tc_line, 0, 0);
		while (scrollcnt < 0) {
			ansi_cmd(ANSI_CMD_SR);
			scrollcnt++;
		}
	}
	if (scrollcnt > 0) {
		rel_move(tc_col, tc_line, 0, t_lines - 1);
		while (scrollcnt > 0) {
			terminal_putchar('\n');
			scrollcnt--;
		}
	}
	for (i = 0; i < scr_lns; i++) {
		j = i + roll;
		while (j >= scr_lns)
			j -= scr_lns;
		if (bp[j].mode & MODIFIED && bp[j].smod < bp[j].len) {
			bp[j].mode &= ~(MODIFIED); //若被修改,则输出
			if (bp[j].emod >= bp[j].len)
				bp[j].emod = bp[j].len - 1;
			rel_move(tc_col, tc_line, bp[j].smod, i);
			if (bp[j].mode & STANDOUT)
				standoutput(bp[j].data, bp[j].smod, bp[j].emod + 1,
						bp[j].sso, bp[j].eso);
			else
				terminal_write_cached(&bp[j].data[bp[j].smod],
						bp[j].emod - bp[j].smod + 1);
			tc_col = bp[j].emod + 1;
			if (tc_col >= t_columns) {
				if (automargins) {
					tc_col -= t_columns;
					tc_line++;
					if (tc_line >= t_lines)
						tc_line = t_lines - 1;
				} else
					tc_col = t_columns - 1;
			}
		}
		if (bp[j].oldlen > bp[j].len) {
			rel_move(tc_col, tc_line, bp[j].len, i);
			ansi_cmd(ANSI_CMD_CE);
		}
		bp[j].oldlen = bp[j].len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	terminal_flush();
}

/**
 * Move to given position.
 * @param y Line number.
 * @param x Column number.
 */
void move(int y, int x)
{
	cur_col = x;
	cur_ln = y < 0 ? y + t_lines : y;
}

/**
 * Get current position.
 * @param[out] y Line number.
 * @param[out] x Column number.
 */
void getyx(int *y, int *x)
{
	*y = cur_ln;
	*x = cur_col;
}

/**
 * Reset screen and move to (0, 0).
 */
void clear(void)
{
	if (dumb_term)
		return;
	roll = 0;
	docls = YEA;
	downfrom = 0;
	struct screenline *slp;
	int i;
	for (i = 0; i < scr_lns; i++) {
		slp = big_picture + i;
		slp->mode = 0;
		slp->len = 0;
		slp->oldlen = 0;
	}
	move(0, 0);
}

//清除big_picture中的第i行,将mode与len置0
void clear_whole_line(int i)
{
	register struct screenline *slp = &big_picture[i];
	slp->mode = slp->len = 0;
	slp->oldlen = 79;
}

/**
 * Clear to end of current line.
 */
void clrtoeol(void)
{
	if (dumb_term)
		return;
	standing = false;
	struct screenline *slp = big_picture + (cur_ln + roll) % scr_lns;
	if (cur_col <= slp->sso)
		slp->mode &= ~STANDOUT;
	if (cur_col > slp->len)
		memset(slp->data + slp->len, ' ', cur_col - slp->len + 1);
	slp->len = cur_col;
}

//从当前行清除到最后一行
void clrtobot(void)
{
	register struct screenline *slp;
	register int i, j;
	if (dumb_term)
		return;
	for (i = cur_ln; i < scr_lns; i++) {
		j = i + roll;
		while (j >= scr_lns)
			//求j%scr_lns ? 因为减法比取余时间少?
			j -= scr_lns;
		slp = &big_picture[j];
		slp->mode = 0;
		slp->len = 0;
		if (slp->oldlen > 0)
			slp->oldlen = 255;
	}
}

static char nullstr[] = "(null)";

/**
 * Output a character.
 * @param c The character.
 * @return 1, 0 on ansi control codes.
 */
int outc(int c)
{
	static bool inansi;
#ifndef BIT8
	c &= 0x7f;
#endif

	if (inansi) {
		if (c == 'm') {
			inansi = false;
			return 0;
		}
		return 0;
	}
	if (c == KEY_ESC && !iscolor) {
		inansi = true;
		return 0;
	}

	if (dumb_term) {
		if (!isprint2(c)) {
			if (c == '\n') {
				terminal_putchar('\r');
			} else {
				if (c != KEY_ESC || !showansi)
					c = '*';
			}
		}
		terminal_putchar(c);
		return 1;
	}

	struct screenline *slp = big_picture + (cur_ln + roll) % scr_lns;
	unsigned int col = cur_col;

	if (!isprint2(c)) {
		if (c == '\n' || c == '\r') {
			if (standing) {
				slp->eso = Max(slp->eso, col);
				standing = false;
			}
			if (col > slp->len)
				memset(slp->data + slp->len, ' ', col - slp->len + 1);
			slp->len = col;
			cur_col = 0;
			if (cur_ln < scr_lns)
				cur_ln++;
			return 1;
		} else {
			if (c != KEY_ESC || !showansi)
				c = '*';
		}
	}

	if (col > slp->len)
		memset(slp->data + slp->len, ' ', col - slp->len);
	if ((slp->mode & MODIFIED) != MODIFIED) {
		slp->smod = (slp->emod = col);
	} else {
		if (col > slp->emod)
			slp->emod = col;
		if (col < slp->smod)
			slp->smod = col;
	}
	slp->mode |= MODIFIED;
	slp->data[col] = c;
	col++;
	if (col > slp->len)
		slp->len = col;

	if (col >= scr_cols) {
		if (standing && slp->mode & STANDOUT) {
			standing = false;
			slp->eso = Max(slp->eso, col);
		}
		col = 0;
		if (cur_ln < scr_lns)
			cur_ln++;
	}
	cur_col = col; /* store cur_col back */
	return 1;
}

/**
 * Output a string.
 * @param str The string.
 */
void outs(const char *str)
{
	while (*str != '\0') {
		outc(*str++);
	}
}

/**
 * Print first n bytes of a string.
 * @param str The string.
 * @param n Maximum output bytes.
 * @param ansi Whether ansi control codes should be excluded in length or not.
 */
static void outns(const char *str, int n, bool ansi)
{
	if (!ansi) {
		while (*str != '\0' && n > 0) {
			outc(*str++);
			n--;
		}
	} else {
		while (*str != '\0' && n > 0) {
			n -= outc(*str++);
		}
		outs("\033[m");
	}
}

int dec[] = { 1000000000, 100000000, 10000000, 1000000, 100000, 10000,
		1000, 100, 10, 1 };

/*以ANSI格式输出可变参数的字符串序列*/
void prints(const char *fmt, ...)
{
	va_list ap;
	char *bp;
	int count, hd, indx;
	va_start(ap, fmt);
	while (*fmt != '\0') {
		if (*fmt == '%') {
			int sgn = 1;
			int sgn2 = 1;
			int val = 0;
			int len, negi;
			bool long_ = false;
			fmt++;
			switch (*fmt) {
				case '-':
					while (*fmt == '-') {
						sgn *= -1;
						fmt++;
					}
					break;
				case 'l':
					long_ = true;
					++fmt;
					break;
				case '.':
					sgn2 = 0;
					fmt++;
					break;
			}
			while (isdigit(*fmt)) {
				val *= 10;
				val += *fmt - '0';
				fmt++;
			}
			switch (*fmt) {
				case 's':
					bp = va_arg(ap, char *);
					if (bp == NULL)
						bp = nullstr;
					if (val) {
						register int slen = strlen(bp);
						if (!sgn2) {
							if (val <= slen)
								outns(bp, val, true);
							else
								outns(bp, slen, true);
						} else if (val <= slen)
							outns(bp, val, false);
						else if (sgn > 0) {
							for (slen = val - slen; slen > 0; slen--)
								outc(' ');
							outs(bp);
						} else {
							outs(bp);
							for (slen = val - slen; slen > 0; slen--)
								outc(' ');
						}
					} else
						outs(bp);
					break;
				case 'd':
					{
						int64_t n;
						if (long_)
							n = va_arg(ap, int64_t);
						else
							n = va_arg(ap, int);

						negi = NA;
						if (n < 0) {
							negi = YEA;
							n *= -1;
						}
						for (indx = 0; indx < 10; indx++)
							if (n >= dec[indx])
								break;
						if (n == 0)
							len = 1;
						else
							len = 10 - indx;
						if (negi)
							len++;
						if (val >= len && sgn > 0) {
							register int slen;
							for (slen = val - len; slen > 0; slen--)
								outc(' ');
						}
						if (negi)
							outc('-');
						hd = 1, indx = 0;
						while (indx < 10) {
							count = 0;
							while (n >= dec[indx]) {
								count++;
								n -= dec[indx];
							}
							indx++;
							if (indx == 10)
								hd = 0;
							if (hd && !count)
								continue;
							hd = 0;
							outc('0' + count);
						}
						if (val >= len && sgn < 0) {
							register int slen;
							for (slen = val - len; slen > 0; slen--)
								outc(' ');
						}
					}
					break;
				case 'c': {
						int i = va_arg(ap, int);
						outc(i);
					}
					break;
				case '\0':
					goto endprint;
				default:
					outc(*fmt);
					break;
			}
			fmt++;
			continue;
		}
		outc(*fmt);
		fmt++;
	}
	va_end(ap);
	endprint: return;
}

/**
 * Scroll down one line.
 */
void scroll(void)
{
	if (dumb_term) {
		prints("\n");
		return;
	}
	scrollcnt++;
	roll++;
	if (roll >= scr_lns)
		roll -= scr_lns;
	move(scr_lns - 1, 0);
	clrtoeol();
}

//将big_picture输出位置1,标准输出区间为(cur_col,cur_col)
void standout(void)
{
	register struct screenline *slp;
	register int ln;
	if (dumb_term)
		return;
	if (!standing) {
		ln = cur_ln + roll;
		while (ln >= scr_lns)
			ln -= scr_lns;
		slp = &big_picture[ln];
		standing = YEA;
		slp->sso = cur_col;
		slp->eso = cur_col;
		slp->mode |= STANDOUT;
	}
}
//	如果standing为真,将当前行在big_picture中的映射设成真
//		并将eso设成eso,cur_col的最大值
void standend(void)
{
	register struct screenline *slp;
	register int ln;
	if (dumb_term)
		return;
	if (standing) {
		ln = cur_ln + roll;
		while (ln >= scr_lns)
			ln -= scr_lns;
		slp = &big_picture[ln];
		standing = NA;
		slp->eso = Max(slp->eso, cur_col);
	}
}
//	根据mode来决定 保存或恢复行line的内容
//		最多只能保存一行,否则会被抹去
void saveline(int line, int mode) /* 0,2 : save, 1,3 : restore */
{
	register struct screenline *bp = big_picture;
	static char tmp[2][256];
	int x, y;

	switch (mode) {
		case 0:
		case 2:
			strlcpy(tmp[mode/2], (const char *)bp[line].data, LINELEN);
			tmp[mode/2][bp[line].len]='\0';
			break;
		case 1:
		case 3:
			getyx(&x, &y);
			move(line, 0);
			clrtoeol();
			refresh();
			prints("%s", tmp[(mode-1)/2]);
			move(x, y);
			refresh();
	}
}

/* Added by Ashinmarch on 2007.12.01,
 * to support multi-line msgs
 * It is used to save screen lines overwrited by msgs
 */
void saveline_buf(int line, int mode)//0:save 1:restore
{
    static char temp[MAX_MSG_LINE * 2 + 2][LINELEN];
    struct screenline *bp = big_picture;
    int x, y;
    
    switch (mode) {
        case 0:
            strncpy(temp[line], (const char *)bp[line].data, LINELEN);
            temp[line][bp[line].len] = '\0';
            break;
        case 1:
            getyx(&x, &y);
            move(line, 0);
            clear_whole_line(line);
            prints("%s", temp[line]);
            move(x,y);
            break;
    }
}
