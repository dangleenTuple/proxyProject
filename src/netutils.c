#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "netutils.h"


void make_socket_non_blocking(int socket_fd)
{
    int flags;

    //Get the file control settings using fcntl
    flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("Couldn't get socket flags");
        exit(1);
    }

    //Set the file control settings using fcntl and NONBLOCK bitmask
    flags |= O_NONBLOCK;
    if (fcntl(socket_fd, F_SETFL, flags) == -1) {
        perror("Couldn't set socket flags");
        exit(-1);
    }
}