#include "bbs.h"
#include "fbbs/convert.h"
#include "fbbs/fbbs.h"
#include "fbbs/shop.h"
#include "fbbs/string.h"
#include "fbbs/title.h"
#include "fbbs/terminal.h"
#include "fbbs/tui_list.h"

static tui_list_loader_t tui_shop_loader(tui_list_t *p)
{
	if (p->data)
		shopping_list_free(p->data);
	p->data = shopping_list_load();
	p->eod = true;
	return (p->all = shopping_list_num_rows(p->data));
}

static tui_list_title_t tui_shop_title(tui_list_t *p)
{
	prints("\033[1;33;44m["SHORT_BBSNAME"�̳�]\033[K\033[m\n"
			" ����[\033[1;32m��\033[m,\033[1;32mRtn\033[m]"
			" ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m]"
			" �뿪[\033[1;32m��\033[m,\033[1;32me\033[m]"
			" ����[\033[1;32mh\033[m]\n"
			"\033[1;44m ���    �۸�   ��� / ��Ŀ\033[K\033[m\n");
}

static tui_list_display_t tui_shop_display(tui_list_t *p, int n)
{
	shopping_list_t *l = p->data;

	GBK_BUFFER(item, SHOPPING_ITEM_CCHARS);
	GBK_BUFFER(categ, SHOPPING_CATEGORY_CCHARS);
	convert_u2g(shopping_list_get_name(l, n), gbk_item);
	convert_u2g(shopping_list_get_category_name(l, n), gbk_categ);

	prints(" %4d %7d  %s / %s\n", n + 1,
			TO_YUAN_INT(shopping_list_get_price(l, n)), gbk_item, gbk_categ);
	return 0;
}

static int tui_title_buy(int type, int price)
{
	if (title_check_existence(session.uid)) {
		presskeyfor("���ѹ������Զ�����ݣ������ҵ���Ʒ�в鿴", t_lines - 1);
		return MINIUPDATE;
	}

	GBK_UTF8_BUFFER(title, TITLE_CCHARS);
	getdata(t_lines - 1, 0, "�������Զ������: ", gbk_title,
			sizeof(gbk_title), YEA, YEA);
	convert_g2u(gbk_title, utf8_title);
	if (validate_utf8_input(utf8_title, TITLE_CCHARS) <= 0)
		return MINIUPDATE;

	if (title_submit_request(type, utf8_title)) {
		presskeyfor("����ɹ����������ĵȴ���ˡ�", t_lines - 1);
	} else {
		presskeyfor("����ʧ��: �⻪�������ϵͳ����", t_lines - 1);
	}
	return MINIUPDATE;
}

static tui_list_handler_t tui_shop_handler(tui_list_t *p, int key)
{
	int type = shopping_list_get_id(p->data, p->cur);
	int price = shopping_list_get_price(p->data, p->cur);
	switch (type) {
		case SHOP_TITLE_90DAYS:
		case SHOP_TITLE_180DAYS:
		case SHOP_TITLE_365DAYS:
			return tui_title_buy(type, price);
		default:
			break;
	}
	return DONOTHING;
}

int tui_shop(void)
{
	tui_list_t t = {
		.data = NULL,
		.loader = tui_shop_loader,
		.title = tui_shop_title,
		.display = tui_shop_display,
		.handler = tui_shop_handler,
		.query = NULL,
	};

	tui_list(&t);

	shopping_list_free(t.data);
	return 0;
}

typedef struct goods_handler_t {
	const char *descr;
	int (*handler)(void);
} goods_handler_t;

typedef struct goods_handlers_t {
	size_t size;
	const goods_handler_t *handlers;
} goods_handlers_t;

static tui_list_loader_t tui_goods_loader(tui_list_t *p)
{
	p->eod = true;
	return (p->all = ((goods_handlers_t *)p->data)->size);
}

static tui_list_title_t tui_goods_title(tui_list_t *p)
{
	prints("\033[1;33;44m[�ҵ���Ʒ]\033[K\033[m\n"
			" �鿴���� [\033[1;32mEnter\033[m,\033[1;32m��\033[m] "
			"���� [\033[1;32m��\033[m,\033[1;32me\033[m]\n"
			"\033[1;44m  ���  ��Ʒ\033[K\033[m\n");
}

static tui_list_display_t tui_goods_display(tui_list_t *p, int n)
{
	const goods_handler_t *handler =
			((goods_handlers_t *)p->data)->handlers + n;
	prints("  %4d  %s\n", n + 1, handler->descr);
	return 0;
}

static tui_list_handler_t tui_goods_handler(tui_list_t *p, int key)
{
	const goods_handler_t *handler =
			((goods_handlers_t *)p->data)->handlers + p->cur;
	switch (key) {
		case '\n':
		case KEY_RIGHT:
			return (*handler->handler)();
		default:
			return DONOTHING;
	}
}

int tui_my_titles(void)
{
	db_res_t *res = db_exec_query(env.d, true,
			"SELECT id, title, add_time, expire, approved, paid FROM titles"
			" WHERE user_id = %d ORDER BY paid DESC", session.uid);
	if (!res || db_res_rows(res) < 1) {
		db_clear(res);
		presskeyfor("��Ŀǰû���Զ������", t_lines - 1);
		return MINIUPDATE;
	}

	clear();
	GBK_BUFFER(title, TITLE_CCHARS);
	int paid = db_get_integer(res, 0, 5);
	if (paid) {
		move(1, 0);
		convert_u2g(db_get_value(res, 0, 1), gbk_title);
		prints("�Զ������: %s\n����ʱ��:   %s\n", gbk_title,
				getdatestring(db_get_time(res, 0, 2), DATE_ZH));
		if (db_get_bool(res, 0, 4)) {
			prints("����ʱ��:   %s\n",
					getdatestring(db_get_time(res, 0, 3), DATE_ZH));
		} else {
			prints("������ˣ������ĵȴ�");
		}
	}

	int rows = db_res_rows(res);
	if (rows > 1) {
		move(6, 0);
		prints("�������������: \n");
	}

	for (int i = 1; i < db_res_rows(res); ++i) {
		convert_u2g(db_get_value(res, i, 1), gbk_title);
		prints("  [%d] %s\n", i, gbk_title);
	}

	db_clear(res);
	pressanykey();
	return FULLUPDATE;
}

int tui_goods(void)
{
	goods_handler_t handlers[] = {
		{ "�Զ������", tui_my_titles }
	};

	goods_handlers_t g = {
		.size = NELEMS(handlers),
		.handlers = handlers,
	};

	tui_list_t t = {
		.data = &g,
		.loader = tui_goods_loader,
		.title = tui_goods_title,
		.display = tui_goods_display,
		.handler = tui_goods_handler,
		.query = NULL,
	};

	tui_list(&t);
	return 0;
}
