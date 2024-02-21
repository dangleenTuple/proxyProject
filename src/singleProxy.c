#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#define MAX_LISTEN_BACKLOG 1 //how many connections we are allowing at a time
#define BUFFER_SIZE 4096

typedef union epoll_data {
    void    *ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event_handler {
    int fd;
    void (*handle)(struct epoll_event_handler*, uint32_t);
    void* closure;
};

int main(int argc, char *argv[]) {
    //Check for arguments on the command line then store them
    char *server_port_str;
    char *reverseProxy_addr;
    char *reverseProxy_port_str;
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_port> <backend_addr> <backend_port>\n", argv[0]);
        exit(1);
    }
    server_port_str = argv[1];
    reverseProxy_addr = argv[2];
    reverseProxy_port_str = argv[3];

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Couldn't create epoll FD");
        exit(1);
    }

    struct epoll_event_handler* server_socket_event_handler;
    server_socket_event_handler = create_server_socket_handler(epoll_fd, server_port_str, reverseProxy_addr, reverseProxy_port_str);
    add_epoll_handler(epoll_fd, server_socket_event_handler, EPOLLIN);
    printf("Started.  Listening on port %s.\n", server_port_str);
    do_epoll_wait(epoll_fd);

    make_socket_non_blocking(server_socket_fd);

    return 0;
}