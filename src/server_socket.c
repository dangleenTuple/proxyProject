#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>

#include "netutils.h"
#include "epollinterface.h"
#include "server_socket.h"
#include "client_socket.h"

#define MAX_LISTEN_BACKLOG 4096


struct server_socket_event_data {
    int epoll_fd;
    char* reverseProxy_addr;
    char* reverseProxy_port_str;
};


struct epoll_event_handler* create_server_socket_handler(int epoll_fd, char* server_port_str, char* reverseProxy_addr, char* reverseProxy_port_str)
{
    int server_socket_fd;
    server_socket_fd = create_and_bind(server_port_str);
    make_socket_non_blocking(server_socket_fd);
    listen(server_socket_fd, MAX_LISTEN_BACKLOG);
    struct server_socket_event_data* closure = &(struct server_socket_event_data) {
        .epoll_fd = epoll_fd,
        .reverseProxy_addr = reverseProxy_addr,
        .reverseProxy_port_str = reverseProxy_port_str,
    };
    struct epoll_event_handler* result =  &(struct epoll_event_handler) {
        .fd = server_socket_fd,
        .handle = handle_server_socket_event,
        .closure = closure,
    };

    return result;
}

void handle_client_connection(int epoll_fd, int client_socket_fd, char* reverseProxy_host, char* reverseProxy_port_str)
{
    struct epoll_event_handler* client_socket_event_handler;
    client_socket_event_handler = create_client_socket_handler(client_socket_fd, epoll_fd, reverseProxy_host, reverseProxy_port_str);
    // EPOLLRDHUP means that the client closed their connection OR only half closed their connection (only allows one-way, write-only connection. Reading is closed)
    // NOT TO BE CONFUSED WITH EPOLLHUP which means an unexpected closing of a socket happened
    // EPOLLIN gets triggered when trying to read from socket but it returns immediately instead of blocking.
    // Them being sent like this means that client sent some data and then closed (either half or completely)
    add_epoll_handler(epoll_fd, client_socket_event_handler, EPOLLIN | EPOLLRDHUP);
}

void handle_server_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct server_socket_event_data* closure = (struct server_socket_event_data*) self->closure;

    int client_socket_fd;
    while (1) {
        client_socket_fd = accept(self->fd, NULL, NULL);
        if (client_socket_fd == -1) {
            //EAGAIN: Resource is temporarily not available. Try again.
            //EWOULDBLOCK: Socket would traditionally be blocking in this state, but it is not due to "non-blocking" mode
            //On most systems, these would be the same value. In that, for a non-blocking socket, it is trying to communicate to try to accept another time.
            //In our case, let's break because this should mean that there are no more incoming client connections to accept.
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            } else {
                perror("Could not accept");
                exit(1);
            }
        }

        handle_client_connection(closure->epoll_fd, client_socket_fd, closure->reverseProxy_addr, closure->reverseProxy_port_str);
    }
}