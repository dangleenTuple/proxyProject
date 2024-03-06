#include <unistd.h>

extern struct epoll_event_handler* create_server_socket_handler(int epoll_fd, char* server_port_str, char* reverseProxy_addr, char* reverseProxy_port_str);