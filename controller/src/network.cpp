#include "network.h"

bool make_fd_non_blocking(int fd)
{
    auto flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int create_listen_socket(int port)
{
    auto sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Error creating socket");
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::close(sock);
        throw std::runtime_error("Error binding socket");
    }

    if (!make_fd_non_blocking(sock)) {
        ::close(sock);
        throw std::runtime_error("Error making socket non-blocking");
    }

    if (::listen(sock, SOMAXCONN) == -1) {
        ::close(sock);
        throw std::runtime_error("Error listening on socket");
    }

    return sock;

}
