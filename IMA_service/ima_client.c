#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <dbus/dbus.h>
#include "err_proc.h"

#define SERVER_PORT 1111
#define SERVER_ADDRESS "127.0.0.1"
#define BUF_SIZE 256


const char *const INTERFACE_NAME = "org.app.imaDbus";
const char *const SERVER_BUS_NAME = "org.app.imaServer";
const char *const CLIENT_BUS_NAME = "org.app.imaClient";
const char *const SERVER_OBJECT_PATH_NAME = "/org/app/imaServObj";
const char *const CLIENT_OBJECT_PATH_NAME = "/org/app/imaCliObj";

static char ima_resp[BUF_SIZE];
DBusError dbus_error;

int send_dbus_cmd(DBusConnection *conn, char* method, char* filename);
void check_and_abort(DBusError *error);
void str_trim(char* str, int length);
void connect_to_server(void);
int check_pidfile(void);
int config_signal_handlers();
void fatal_sig_handler(int sig);
void term_handler(int sig);

int main()
{
	pid_t pid;

	chdir("/");

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

	openlog("IMA_client", LOG_PID|LOG_CONS|LOG_NDELAY|LOG_NOWAIT, LOG_LOCAL0);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	int stdio_fd = open("/dev/null", O_RDWR);
	dup(stdio_fd);
	dup(stdio_fd);

	connect_to_server();

	return 0;
}

void connect_to_server(void)
{
	char in_buf[BUF_SIZE] = {0};
	char out_buf[BUF_SIZE]= {0};
	char tmp_buf[BUF_SIZE]= {0};
	DBusConnection *dbus_conn;

	char command[8];
	char filename[BUF_SIZE];
	char* tok;
	int i = 0;

    dbus_error_init (&dbus_error);

    dbus_conn = dbus_bus_get (DBUS_BUS_SESSION, &dbus_error);

    check_and_abort(&dbus_error);

    if(!dbus_conn)
    {
    	printf("[ERROR] Fail to connect to the server.");
        exit(1);
    } 

	int fd = Socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	
	Inet_pton(AF_INET, SERVER_ADDRESS, &addr.sin_addr);
	
	Connect(fd, (struct sockaddr *) &addr, sizeof(addr));

	while(1)
	{
		int res = read(fd, in_buf, sizeof(in_buf));
		if (res == -1) perror("[ERROR] Fail to read from server");
		if (res > 0)
		{
			if(strncmp(in_buf, "disconnect", 10) == 0)
			{
				write(fd, "Disconnected.", 13);
				close(fd);
				exit(0);
			}

			if(strncmp(in_buf, "status", 6) == 0)
			{
				if(send_dbus_cmd(dbus_conn, "status", NULL))
				{
					sprintf(out_buf, "STATUS: %s", ima_resp);
					write(fd, out_buf, strlen(out_buf));
					bzero(ima_resp, sizeof(ima_resp));
				}
				else write(fd, "Failed to send command to DBus.", 31);
			}

			if((strncmp(in_buf, "addfile", 7) == 0) || (strncmp(in_buf, "rmfile", 6) == 0))
			{
				str_trim(in_buf, strlen(in_buf));
				sprintf(tmp_buf, "%s", in_buf);
				tok = strtok(tmp_buf, " ");

				while(tok != NULL)
				{
					if(i == 0)
						sprintf(command, "%s", tok);;
					if(i == 1)
						sprintf(filename, "%s", tok);	

					tok = strtok(NULL, " ");
					i++;
				}
				
				if((i == 1) || (i > 2)) 
				{
					write(fd, "Wrong arguments. Usage: addfile|rmfile <filename> ", 50);
					continue;
				}

				i = 0;

				if(send_dbus_cmd(dbus_conn, command, filename))
				{

					sprintf(out_buf, "[%s]: %s", command, ima_resp);
					write(fd, out_buf, sizeof(out_buf));
					bzero(ima_resp, sizeof(ima_resp));
				}
				else write(fd, "Failed to send command to DBus.", 31);
			}
		}
		bzero(in_buf, BUF_SIZE);
		bzero(out_buf, BUF_SIZE);
		bzero(tmp_buf, BUF_SIZE);
		sleep(0.5);
	}

	return;
}

int send_dbus_cmd(DBusConnection *conn, char* method, char* filename)
{
	int ret;

    while (1) 
    {
	    ret = dbus_bus_request_name (conn, CLIENT_BUS_NAME, 0, &dbus_error);

	    if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) 
	       break;

	    if (ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE) 
	    {
	       sleep (1);
	       continue;
	    }
	    
	    check_and_abort(&dbus_error);
	}

	DBusMessage *request;
	DBusMessageIter iter;

	if(strncmp(method, "status", 6) == 0)
	{
		if ((request = dbus_message_new_method_call (SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME, 
                       INTERFACE_NAME, "status")) == NULL) 
		{
			syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_message_new_method_call");
	        exit (1);
        }
	}

    if(strncmp(method, "addfile", 7) == 0)
    {
    	if ((request = dbus_message_new_method_call (SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME, 
                       INTERFACE_NAME, "addFile")) == NULL) 
		{
			syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_message_new_method_call");
	        exit (1);
        }

        dbus_message_iter_init_append (request, &iter);
        if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &filename)) 
        {
            syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_message_iter_append_basic");
            exit (1);
        }
    }

    if(strncmp(method, "rmfile", 6) == 0)
    {
    	if ((request = dbus_message_new_method_call (SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME, 
                       INTERFACE_NAME, "rmFile")) == NULL) 
		{
	        syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_message_new_method_call");
	        exit (1);
        }

        dbus_message_iter_init_append (request, &iter);
        if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &filename)) 
        {
            syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_message_iter_append_basic");
            exit (1);
        }
    }


    DBusPendingCall *pending_return;
    if (!dbus_connection_send_with_reply (conn, request, &pending_return, -1)) 
    {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_connection_send_with_reply");
        exit (1);
    }

    if (pending_return == NULL) 
    {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Pending return is NULL");
        exit (1);
    }

    dbus_connection_flush (conn);
                
    dbus_message_unref (request);	

    dbus_pending_call_block (pending_return);

    DBusMessage *reply;
    if ((reply = dbus_pending_call_steal_reply (pending_return)) == NULL) 
    {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_pending_call_steal_reply");
        exit (1);
    }

    dbus_pending_call_unref	(pending_return);

    char *s;


    if (!dbus_message_get_args (reply, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID))
    {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Did not get arguments in reply");
        exit (1);
    }
    sprintf(ima_resp, "%s", s);
    dbus_message_unref (reply);	

    if (dbus_bus_release_name (conn, CLIENT_BUS_NAME, &dbus_error) == -1) 
    {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] Error in dbus_bus_release_name");
        exit (1);
    }

    return 1;
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

void check_and_abort(DBusError *error) 
{
    if (dbus_error_is_set(error)) {
    	syslog(LOG_LOCAL0|LOG_INFO,"[ERROR] %s", error->message);
        abort();
    }
}

void str_trim(char* str, int length)
{
	for(int i =0; i < length; i++)
	{
		if(str[i] == '\n')
		{
			str[i] = '\0';
			break;
		}
	}
}
