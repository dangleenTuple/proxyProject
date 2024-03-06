#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "epollinterface.h"

#define MAX_EPOLL_EVENTS 10


void add_epoll_handler(int epoll_fd, struct epoll_event_handler* handler, uint32_t event_mask)
{
    struct epoll_event event;
    event.data.ptr = handler;
    event.events = event_mask;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &event) == -1) {
        perror("Couldn't register server socket with epoll");
        exit(-1);
    }
}

void do_epoll_wait(int epoll_fd)
{
    struct epoll_event current_epoll_event;

    while (1) {
        struct epoll_event_handler* handler;
        //What this does is ask for one event (1), with no timeout (-1)
        //You could pass in an array and ask for a max number of events instead of just putting one.
        //However, in the scenario that one of the client connections close before we even get to it in the epoll data structure, it would render this event useless
        //and even cause the server to crash.
        //There is more than one solution to this problem, but for now, we can simply just block for only one event at a time.
        epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
        handler = (struct epoll_event_handler*) current_epoll_event.data.ptr;
        handler->handle(handler, current_epoll_event.events);
    }

}