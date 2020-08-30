#ifndef IMA_SERVER_H
#define IMA_SERVER_H

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1111
#define MAX_CLIENT_NUMBER  100
#define BUF_SIZE 256

typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
} client_t;

void print_ip_addr(struct sockaddr_in addr);

void clients_list(void);

void client_info(int uid);

void queue_add(client_t *cl);

void queue_rm(int uid);

void *handle_client(void *arg);

void *cmd_thread(void *arg);

void send_command(char *s, int uid);

int is_client_exist(int uid);

void str_trim(char* str, int length);

void printf_help(void);

#endif