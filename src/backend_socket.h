#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

extern void handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events);

extern struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, struct epoll_event_handler* client_handler);

extern void close_backend_socket(struct epoll_event_handler* handler);