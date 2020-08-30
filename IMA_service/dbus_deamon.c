#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dbus/dbus.h>
#include "dbus_func.h"


int config_signal_handlers();
void fatal_sig_handler(int sig);
void term_handler(int sig);

int main()
{
	pid_t pid;

	//chdir("/");

	pid = fork();

	switch(pid)
	{
		case 0:
			break;
		case -1:
				printf("Fork error\n");//ошибка создания нового процесса (маловероятная)
				return -1;
		default:
				exit(0);
	}

	if(setsid() < 0) return -1;
	
	signal(SIGHUP, SIG_IGN);

	config_signal_handlers();

	openlog("IMA_deamon", LOG_PID|LOG_CONS|LOG_NDELAY|LOG_NOWAIT, LOG_LOCAL0);
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	int stdio_fd = open("/dev/null", O_RDWR);
	dup(stdio_fd);
	dup(stdio_fd);

	dbus_listen();

	return 0;
}

int config_signal_handlers(void)
{
	struct sigaction sigtermSA;

	//игнорируемые сигналы
	signal(SIGUSR1,SIG_IGN);
	signal(SIGUSR2,SIG_IGN);	
	signal(SIGPIPE,SIG_IGN);
	signal(SIGALRM,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGURG,SIG_IGN);
	signal(SIGXCPU,SIG_IGN);
	signal(SIGXFSZ,SIG_IGN);
	signal(SIGVTALRM,SIG_IGN);
	signal(SIGPROF,SIG_IGN);
	signal(SIGIO,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);

	//устанавливаем обработчик для сигналов
	signal(SIGQUIT,fatal_sig_handler);
	signal(SIGILL,fatal_sig_handler);
	signal(SIGTRAP,fatal_sig_handler);
	signal(SIGABRT,fatal_sig_handler);
	signal(SIGIOT,fatal_sig_handler);
	signal(SIGBUS,fatal_sig_handler);
	signal(SIGFPE,fatal_sig_handler);
	signal(SIGSEGV,fatal_sig_handler);
	signal(SIGSTKFLT,fatal_sig_handler);
	signal(SIGCONT,fatal_sig_handler);
	signal(SIGPWR,fatal_sig_handler);
	signal(SIGSYS,fatal_sig_handler);

	//сигналы для управления демоном
	//SIGTERM
	sigtermSA.sa_handler=term_handler;
	sigemptyset(&sigtermSA.sa_mask);
	sigtermSA.sa_flags=0;
	sigaction(SIGTERM,&sigtermSA,NULL);

	return 0;
}

void fatal_sig_handler(int sig)
{
	syslog(LOG_LOCAL0|LOG_INFO, "caugth signal: %s - exiting", strsignal(sig));
	closelog();
	exit(0);
}

void term_handler(int sig)
{
	syslog(LOG_LOCAL0|LOG_INFO, "caugth signal: %s - exiting", strsignal(sig));
	closelog();
	exit(0);
}

