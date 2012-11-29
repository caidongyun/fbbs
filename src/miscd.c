#include "bbs.h"
#include "fbbs/cfg.h"
#include "fbbs/helper.h"
#include "fbbs/pool.h"

extern int b_closepolls(void);

//退出时执行的函数
void do_exit() {
	flush_ucache();
}

int main(int argc, char *argv[]) {
	chdir(BBSHOME); //进入BBS用户主目录
	setuid(BBSUID); //将进程的 用户ID
	setgid(BBSGID); //组ID设置成BBS
	setreuid(BBSUID, BBSUID); //设置有效用户ID	
	setregid(BBSGID, BBSGID); //有效组ID为BBS

	if (argc <= 1) {
		printf("usage: daemon | flushed | reload\n");
		exit(0);
	}

	if ( !strcasecmp(argv[1], "daemon") ) { // miscd daemon
		switch (fork()) { //后台程序:需要创建一个子进程,由子进程杀死父进程
			case -1: //
				printf("cannot fork\n");
				exit(0);
				break;
			case 0: // 子进程
				break;
			default:
				exit(0); //父进程
				break;
		}

		initialize_environment(INIT_DB);

		if (load_ucache() != 0) { //将用户的数据映射到内存
			printf("load ucache error\n");
			exit(-1);
		}

		if (resolve_boards() < 0)
			exit(-1);
		atexit(do_exit); //注册退出前运行的函数.正常退出前须执行此函数

		while (1) { //循环
			b_closepolls(); //关闭投票
			flush_ucache(); //将用户在内存中的数据写回.PASSWDS
			sleep(60 * 15); //睡眠十分钟,即每十五分钟同步一次.        
		}
	} else if ( !strcasecmp(argv[1], "flushed") ) { //miscd flushed
		if (resolve_ucache() == -1)
			exit(1);
		flush_ucache();
	} else if ( !strcasecmp(argv[1], "reload") ) { //miscd reload
		if (load_ucache() != 0) {
			printf("load ucache error\n");
			exit(-1);
		}
	} else {
		printf("usage: daemon | flushed | reload\n");
		exit(0);
	}

	return 0;
}
