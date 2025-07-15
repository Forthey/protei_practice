#ifndef CONNECTIONSHANDLER_H
#define CONNECTIONSHANDLER_H

#include <iostream>
#include <unordered_map>
#include <functional>
#include <cerrno>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


class ConnectionsHandler {
public:
    using ReceiveCallback = std::function<std::string(std::string)>;

private:
    static constexpr int MAX_EVENTS = 1000;
    static constexpr int BUF_SIZE = 1024;

    int const port;

    int const epoll_fd;
    int const listen_fd;
    epoll_event ev{}, events[MAX_EVENTS]{};

    static int set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

public:
    explicit ConnectionsHandler(int const port)
        : port(port), epoll_fd(epoll_create1(0)), listen_fd(socket(AF_INET, SOCK_STREAM, 0)) {
        if (listen_fd < 0) {
            perror("socket");
            return;
        }
        set_nonblocking(listen_fd);

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {.s_addr = INADDR_ANY},
            .sin_zero = {0},
        };

        if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            perror("bind");
            return;
        }
        if (::listen(listen_fd, SOMAXCONN) < 0) {
            perror("listen");
            return;
        }

        ev.events = EPOLLIN;
        ev.data.fd = listen_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
    }

    void listen(ReceiveCallback const &receive_callback) {
        std::cout << "listening on port " << port << std::endl;

        std::unordered_map<int, std::string> buffers;
        while (true) {
            int nf = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            // std::cerr << "epoll_wait returned nf=" << nf << "\n";
            if (nf < 0) {
                perror("epoll_wait");
                break;
            }
            for (int n = 0; n < nf; ++n) {
                if (events[n].data.fd == listen_fd) {
                    sockaddr_in cli_addr{};
                    socklen_t cli_len = sizeof(cli_addr);
                    int conn_fd = accept(listen_fd, reinterpret_cast<sockaddr *>(&cli_addr), &cli_len);
                    if (conn_fd < 0) {
                        continue;
                    }
                    set_nonblocking(conn_fd);
                    ev.events = EPOLLIN;
                    ev.data.fd = conn_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) < 0) {
                        perror("epoll_ctl");
                    }
                } else {
                    int fd = events[n].data.fd;
                    bool closed = false;
                    while (true) {
                        char buf[BUF_SIZE];
                        long len = read(fd, buf, BUF_SIZE - 1);
                        if (len > 0) {
                            buffers[fd].append(buf, len);
                        } else if (len == 0) {
                            closed = true;
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            }
                            perror("read");
                            closed = true;
                            break;
                        }
                    }
                    if (closed) {
                        std::string const &data(buffers[fd]);

                        std::string out = receive_callback(data);

                        write(fd, out.c_str(), out.size());

                        shutdown(fd, SHUT_WR);

                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        buffers.erase(fd);
                    }
                }
            }
        }
    }

    ~ConnectionsHandler() {
        close(listen_fd);
        close(epoll_fd);
    }
};

#endif //CONNECTIONSHANDLER_H
