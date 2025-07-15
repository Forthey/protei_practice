#ifndef CONNECTIONSHANDLER_H
#define CONNECTIONSHANDLER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>


class ConnectionsHandler {
public:
    struct Context {
        std::string expr;
    };

    struct Connection {
        int fd;
        std::string sent;
        std::string received;
        bool done;
        Context ctx;
    };

    using ReceiveCallback = std::function<void(Context const &, std::string)>;

private:
    static constexpr int MAX_EVENTS_C = 1000;
    static constexpr int BUF_SIZE_C = 1024;

    std::string const server_addr;
    int const server_port;
    int const n;
    std::unordered_map<int, Connection> conns;

    int const epoll_fd;
    epoll_event ev{}, events[MAX_EVENTS_C]{};

    static int set_nonblocking_c(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

public:
    explicit ConnectionsHandler(std::string server_addr, int const server_port, int const n)
        : server_addr(std::move(server_addr)), server_port(server_port), n(n), epoll_fd(epoll_create1(0)) {
    }

    bool create_new_connection(Context const &ctx) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            exit(1);
        }
        set_nonblocking_c(sock);
        sockaddr_in serv_addr{
            .sin_family = AF_INET,
            .sin_port = htons(server_port),
            .sin_addr = {.s_addr = inet_addr(server_addr.c_str())},
            .sin_zero = {},
        };

        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        int rc = connect(sock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr));
        if (rc < 0 && errno != EINPROGRESS) {
            perror("connect failed");
            close(sock);
            return false;
        }

        conns[sock] = {
            .fd = sock,
            .sent = "",
            .received = "",
            .done = false,
            .ctx = ctx
        };

        ev.events = EPOLLOUT;
        ev.data.fd = sock;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
            perror("epoll_ctl");
            close(sock);
            return false;
        }

        return true;
    }

    static std::optional<std::string> read_all_data(int fd, Connection &c,
                                                    std::function<void(Connection &, int)> const &handle_shutdown) {
        while (true) {
            char tmp_buf[BUF_SIZE_C];
            long len = recv(fd, tmp_buf, BUF_SIZE_C - 1, 0);
            if (len > 0) {
                c.received.append(tmp_buf, len);
            } else if (len == 0) {
                handle_shutdown(c, fd);
                return c.received;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                perror("recv");
                handle_shutdown(c, fd);
                break;
            }
        }

        return std::nullopt;
    }

    void send_all(ReceiveCallback const &receive_callback) {
        std::size_t remaining = conns.size();

        auto handle_shutdown = [&](Connection &connection, int fd) {
            connection.done = true;
            remaining--;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
        };

        while (remaining > 0) {
            int nf = epoll_wait(epoll_fd, events, MAX_EVENTS_C, -1);
            // std::cout << "epoll_wait returned nf=" << nf << std::endl;
            if (nf < 0) {
                perror("epoll_wait");
                continue;
            }
            for (int i = 0; i < nf; ++i) {
                int fd = events[i].data.fd;
                if (conns.count(fd) <= 0) {
                    std::cerr << "epoll_wait returned invalid fd" << std::endl;
                    continue;
                }
                Connection &c = conns[fd];

                if (!c.done && (events[i].events & EPOLLOUT)) {
                    if (c.sent.size() < c.ctx.expr.size()) {
                        std::size_t left = c.ctx.expr.size() - c.sent.size();
                        std::size_t frag = 1 + (rand() % left);
                        std::string part = c.ctx.expr.substr(c.sent.size(), frag);
                        long sent_bytes = send(fd, part.c_str(), part.size(), 0);
                        if (sent_bytes < 0) {
                            perror("send");
                            handle_shutdown(c, fd);
                            continue;
                        }
                        c.sent.append(part.substr(0, sent_bytes));
                    }
                    if (c.sent.size() == c.ctx.expr.size()) {
                        ev.events = EPOLLIN;
                        ev.data.fd = fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
                            perror("epoll_ctl");
                        }
                        shutdown(fd, SHUT_WR);
                    }
                } else if (!c.done && (events[i].events & EPOLLIN)) {
                    auto data = read_all_data(fd, c, handle_shutdown);

                    if (data.has_value()) {
                        receive_callback(c.ctx, data.value());
                    }
                }
            }
        }
    }
};

#endif //CONNECTIONSHANDLER_H
