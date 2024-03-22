#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"
#include "client_socket.h"

#define BUFFER_SIZE 4096

struct client_socket_event_data {
    struct epoll_event_handler* backend_handler;
};

struct epoll_event_handler* connect_to_backend(struct epoll_event_handler* client_handler, int epoll_fd, char* backend_host, char* backend_port_str)
{
    struct addrinfo hints;

    //set up our hints struct. We only want to read and write to files, no datagrams (packets of data) are intended to be created/used
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; //could be IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //we want to expect streaming sockets specifically

    struct addrinfo *addrs;
    int getaddrinfo_error;

    //Let's get an internet address we can connect to
    getaddrinfo_error = getaddrinfo(backend_host, backend_port_str, &hints, &addrs);
    if (getaddrinfo_error != 0) {
        fprintf(stderr, "Couldn't find an internet address for the reverse proxy: %s\n", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    struct addrinfo *addrs_iter;
    int backend_socket_fd;

    //We now have a list of addresses that meet our requirements. Let's check each one and find the first one available.
    for (addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next) 
    {
        //Let's attempt to create a socket with the curr address. We "continue" if this fails.
        backend_socket_fd = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
        if (backend_socket_fd == -1) {
            continue;
        }
        //Now that we have a socket, let's try to connect! If this is successful, we are done here.
        if (connect(backend_socket_fd, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1) { 
            break;
        }

        //If we are not successful with connecting, let's close this socket and try the next one.
        close(backend_socket_fd);
    }
    
    //If addres_iter is still NULL, that means we couldn't make a connection at all.
    if (addrs_iter == NULL) {
        fprintf(stderr, "Couldn't establish a connection for the reverse proxy\n");
        exit(1);
    }

    //Otherwise if successful, let's free all of the unused addresses.
    freeaddrinfo(addrs);

    struct epoll_event_handler* backend_socket_event_handler;
    backend_socket_event_handler = create_backend_socket_handler(backend_socket_fd, client_handler);
    add_epoll_handler(epoll_fd, backend_socket_event_handler, EPOLLIN | EPOLLRDHUP);

    return backend_socket_event_handler;
}


//This function gets called when the server accepts the connection to the client. Then it uses this handler to handle this event.
struct epoll_event_handler* create_client_socket_handler(int client_socket_fd, int epoll_fd, char* backend_host, char* backend_port_str)
{
    make_socket_non_blocking(client_socket_fd);
    struct client_socket_event_data* closure = malloc(sizeof(struct client_socket_event_data));

    struct epoll_event_handler* result = malloc(sizeof(struct epoll_event_handler));
    result->fd = client_socket_fd;
    result->handle = handle_client_socket_event;
    result->closure = closure;

    //We will find a connection for the backend that we want for this client then add this backend event to the epoll instance
    closure->backend_handler = connect_to_backend(result, epoll_fd, backend_host, backend_port_str);
    return result;
}


void handle_client_socket_event(struct epoll_event_handler* self, uint32_t events)
{
    struct client_socket_event_data* closure = (struct client_socket_event_data* ) self->closure;

    char buffer[BUFFER_SIZE];
    int bytes_read;

    //If there's data coming in
    if (events & EPOLLIN) {
        //We are using level-triggered epoll. So, this buffer size is not considered a real "limit".
        //If the data received surpasses the buffer size, the data transfer will simply be put on hold.
        //We will come back to it later.
        //In contrast, if we use edge-triggered epoll, then the data transfer that is more than the buffer size
        //will be lost.
        bytes_read = read(self->fd, buffer, BUFFER_SIZE);
        //No data received, so we just return.
        if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            printf("ERROR WHILE RECEIVING RECEIVED FOR CLIENT\n");
            return;
        }
        //If we receive nothing, but there's no other indication, the remote end probably closed its connection.
        if (bytes_read == 0 || bytes_read == -1) {
            printf("NO BYTES RECEIVED FOR CLIENT, CLOSING SOCKET\n");
            close_backend_socket(closure->backend_handler);
            close_client_socket(self);
            return;
        }

        //Let's print what we received from the backend
        printf("CLIENT SERVER RECEIVED THIS MESSAGE:\n");
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%c", buffer[i]);  
        }
        printf("\n");

        //Successful read! Send it to the backend
        write(closure->backend_handler->fd, buffer, bytes_read);
    }

    //If an error occurs via the event bitmask (any err or perhaps a sudden closure of a connection), close client and backend socket connections.
    //NOTE: close means to close connection, clear all resources, then completely disconnect from (or destroy) the socket.
    //Shutdown means to just simply close the connection, but remain connected to the socket.
    if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
        close_backend_socket(closure->backend_handler);
        close_client_socket(self);
        return;
    }

    return;
}

//cleanup
void close_client_socket(struct epoll_event_handler* self)
{
    close(self->fd);
    free(self->closure);
    free(self);
}