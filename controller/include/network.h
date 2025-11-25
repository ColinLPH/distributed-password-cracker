#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

constexpr int MAX_EPOLL_EVENTS = 100;

bool make_fd_non_blocking(int fd);
int create_listen_socket(int port);

#endif // NETWORK_H
