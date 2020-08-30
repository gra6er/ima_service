#ifndef ERR_PROC_H
#define ERR_PROC_H

#include <sys/types.h>
#include <sys/socket.h>

int Socket(int domain, int type, int protocol);

void Bind(int sock_fd, const struct sockaddr *addr, socklen_t addrlen);

void Listen(int sock_fd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

void Connect(int sock_fd, const struct sockaddr *addr, socklen_t addrlen);

void Inet_pton(int af, const char *src, void *dst);

void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

#endif