#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include "err_proc.h"
#include "ima_server.h"

static _Atomic unsigned int cli_count = 0;
static int uid = 1;
//.


static client_t *clients[MAX_CLIENT_NUMBER];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


int main()
{
	int option = 1;
	pthread_t tid;
	//Socket
	int serv_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serv_addr.sin_port = htons(SERVER_PORT);

	//Setsockopt
	Setsockopt(serv_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &option, sizeof(option));
	//Bind
	Bind(serv_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	//Listen
	Listen(serv_fd, MAX_CLIENT_NUMBER);

	printf("====SERVER====\n");

	pthread_create(&tid, NULL, &cmd_thread, NULL);
	pthread_detach(tid);;

	while(1)
	{
		socklen_t cli_len = sizeof(cli_addr);
		int cli_fd = Accept(serv_fd, (struct sockaddr*)&cli_addr, &cli_len);

		if(cli_count >= MAX_CLIENT_NUMBER)
		{
			printf("Max clients reached. Rejected ");
			print_ip_addr(cli_addr);
			printf(":%d\n", ntohs(cli_addr.sin_port));
			close(cli_fd);
			continue;
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = cli_fd;
		cli->uid = uid++;

		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void *)cli);

	}
}

void print_ip_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d", 
		addr.sin_addr.s_addr & 0xFF,
		(addr.sin_addr.s_addr & 0xFF00) >> 8,
		(addr.sin_addr.s_addr & 0xFF0000) >> 16,
		(addr.sin_addr.s_addr & 0xFF000000) >> 24);
}

void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void queue_rm(int uid)
{
	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
	char buf[BUF_SIZE];
	char out[BUF_SIZE + 32];
	client_t *cli = (client_t *)arg;

	cli_count++;

	printf("\nNew client [UID=%d] connected.\n IP: ", cli->uid);
	print_ip_addr(cli->address);
	printf("\n PORT: %d\n", ntohs(cli->address.sin_port));
	printf("> ");

	while(1)
	{
		int res = read(cli->sockfd, buf, sizeof(buf));
		if(res == -1) perror("[ERROR] Fail to read from the client");
		if(res > 0)
		{
			sprintf(out, "\nMsg from UID=%d: %s\n[client %d]> ", cli->uid, buf, cli->uid);
			write(STDOUT_FILENO, out, strlen(out));
		}
		bzero(buf, BUF_SIZE);
		sleep(1);
	}

	close(cli->sockfd);
	queue_rm(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

void *cmd_thread(void *arg)
{
	char buf[BUF_SIZE];
	char tmp_buf[BUF_SIZE];
	char cmd_line[16];
	char* tok;
	int uid;
	int client_flag = 0;
	int i = 0;

	sprintf(cmd_line, "> ");
	while(1)
	{
		write(STDOUT_FILENO, cmd_line, sizeof(cmd_line));
		int res = read(STDIN_FILENO, buf, sizeof(buf));
		if(res == -1)
		{
			perror("[ERROR] Read failed");
			exit(EXIT_FAILURE);
		}

		if(strncmp(buf, "back", 4) == 0)
		{
			uid = 0;
			client_flag = 0;
			bzero(cmd_line, sizeof(cmd_line));
			sprintf(cmd_line, "> ");
			bzero(buf, BUF_SIZE);
			continue;
		}

		if(strncmp(buf, "help", 4) == 0)
		{
			printf_help();
			bzero(buf, BUF_SIZE);
			continue;
		}

		if(strncmp(buf, "list", 4) == 0)
		{
			clients_list();
			bzero(buf, BUF_SIZE);
			continue;
		}

		if(((strncmp(buf, "info", 4) == 0)) && client_flag)
		{
			client_info(uid);
			bzero(buf, BUF_SIZE);
			continue;
		}

		if(strncmp(buf, "exit", 4) == 0)
		{
			printf("Exit.\n");
			exit(0);
		}

		if(0 == strncmp(buf, "disconnect", 10) && client_flag == 1)
		{
			send_command("disconnect", uid);
			queue_rm(uid);
			uid = 0;
			bzero(buf, BUF_SIZE);
			continue;
		}

		if(client_flag && (strlen(buf) > 1))
		{
			if((strncmp(buf, "status", 6) == 0)
			|| (strncmp(buf, "addfile", 7) == 0)
			|| (strncmp(buf, "rmfile", 6) == 0))
			{
				printf("Sending command to UID=%d: %s", uid, buf);
				send_command(buf, uid);
				if(strncmp(buf, "disconnect", 10) == 0) 
				{
					queue_rm(uid);
					uid = 0;
				}
				bzero(buf, BUF_SIZE);
				continue;
			}
			else
			{
				printf("Unknown client command. Enter \"help\" to reference \n");
				bzero(buf, BUF_SIZE);
				continue;
			}
			
		}

		if(strncmp(buf, "client", 6) == 0)
		{

			str_trim(buf, strlen(buf));
			sprintf(tmp_buf, "%s", buf);
			tok = strtok(tmp_buf, " ");

			while(tok != NULL)
			{
				if(i == 1) uid = atoi(tok);
				tok = strtok(NULL, " ");
				i++;
			}

			if((i == 1) || (uid == 0) || (i > 2))
			{
				printf("Wrong arguments. Usage: client <UID> [i = %d uid = %d]\n", i, uid);
				bzero(tmp_buf, BUF_SIZE);
				bzero(buf, BUF_SIZE);
				i = 0;
				continue;
			}

			i = 0;

			if(uid != 0)
			{
				if(!is_client_exist(uid)) 
					printf("Client with this UID doesn't exist.\n");
				else
				{
					sprintf(cmd_line, "[client %d]> ", uid);
					client_flag = 1;
				}
			} 
			else
				printf("Failed to input UID, try again.\n");

			bzero(tmp_buf, BUF_SIZE);
			bzero(buf, BUF_SIZE);
			continue;
		}
		if(strlen(buf) > 1)
			printf("Unknown command. Enter \"help\" to reference \n");
		bzero(buf, BUF_SIZE);
	}

	return NULL;
}

void send_command(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				int res = write(clients[i]->sockfd, s, strlen(s));
				if(res == -1) perror("failed to write to descriptor");
				break;
			}					
		}
	}	
	pthread_mutex_unlock(&clients_mutex);
}

int is_client_exist(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	int exist = 0;
	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				exist = 1;
				break;
			}					
		}
	}

	pthread_mutex_unlock(&clients_mutex);

	return exist;
}

void clients_list(void)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(clients[i])
		{
				printf("UID %d\n", clients[i]->uid);
				printf(" IP: ");
				print_ip_addr(clients[i]->address);
				printf("\n PORT: %d\n", ntohs(clients[i]->address.sin_port));				
		}
	}			
	pthread_mutex_unlock(&clients_mutex);
}

void client_info(int uid)
{
	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENT_NUMBER; i++)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				printf(" IP: ");
				print_ip_addr(clients[i]->address);
				printf("\n PORT: %d\n", ntohs(clients[i]->address.sin_port));
				break;
			}					
		}
	}			
	pthread_mutex_unlock(&clients_mutex);	
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

void printf_help(void)
{
	printf(	"Usage:\n\t"
				"exit - terminates the program\n\t"
				"list - shows all connected client and their info\n\t"
				"client <UID> - client mode\n\n"
				"Client mode:\n\t"
				"back - exit from client mode\n\t"
				"info - shows client's IP and PORT\n\t"
				"status - IMA service status of client\n\t"
				"addfile <filename> - adds file to IMA service\n\t"
				"rmfile <filename> - removes file from IMA service\n");
}