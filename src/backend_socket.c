#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>

#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"
#include "client_socket.h"

#define BUFFER_SIZE 4096


struct backend_socket_event_data {
    struct epoll_event_handler* client_handler;
};

struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, struct epoll_event_handler* client_handler)
{
    make_socket_non_blocking(backend_socket_fd);

    struct backend_socket_event_data* closure = malloc(sizeof(struct backend_socket_event_data));
    closure->client_handler = client_handler;

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = backend_socket_fd;
    result->handle = handle_backend_socket_event;
    result->closure = closure;
    return result;
}

//Look at notes in client ("handle_client_socket_event"), same exact function there.
void handle_backend_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    printf("Handling backend server event\n");
    struct backend_socket_event_data* closure = (struct backend_socket_event_data*) self->closure;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    if (events & EPOLLIN) {
        printf("BYTES RECEIVED FOR BACKEND\n");
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            printf("ERROR NO BYTES RECEIVED FOR BACKEND\n");
            return;
        }
        if (bytes_read == 0 || bytes_read == -1) {
            printf("NO BYTES RECEIVED FOR BACKEND\n"); //////////
            close_client_socket(closure->client_handler);
            close_backend_socket(self);
            return;
        }

        //Let's print what we received from the backend
        printf("BACKEND SERVER RECEIVED THIS MESSAGE:\n");
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%02x ", (unsigned int) buffer[i]);    
        }
        printf("\n");

        write(closure->client_handler->fd, buffer, bytes_read);
    }
    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close_client_socket(closure->client_handler);
        close_backend_socket(self);
        return;
    }
}

//Cleanup!
void close_backend_socket(struct epoll_event_handler* handler)
{
    close(handler->fd);
    free(handler->closure);
    free(handler);
}
