#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_LISTEN_BACKLOG 1 //how many connections we are allowing at a time
#define BUFFER_SIZE 4096

void handle_client_connection(int client_socket_fd, char *reverseProxy_host, char *reverseProxy_port_str)
{
    struct addrinfo services;

    //set up our services struct. We only want to read and write to files, no datagrams (packets of data) are intended to be created/used
    memset(&services, 0, sizeof(struct addrinfo));
    services.ai_family = AF_UNSPEC; //could be IPv4 or IPv6
    services.ai_socktype = SOCK_STREAM; //we want to expect streaming sockets specifically

    struct addrinfo *addrs;
    int getaddrinfo_error;

    //Let's get an internet address we can bind/connect to
    getaddrinfo_error = getaddrinfo(reverseProxy_host, reverseProxy_port_str, &services, &addrs);
    if (getaddrinfo_error != 0) {
        fprintf(stderr, "Couldn't find reverse proxy: %s\n", gai_strerror(getaddrinfo_error));
        exit(1);
    }

    struct addrinfo *addrs_iter;
    int reverseProxy_socket_fd;

    //We now have a list of addresses that meet our requirements. Let's check each one and find the first one available.
    for (addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next) 
    {
        //Let's attempt to create a socket with the curr address. We "continue" if this fails.
        reverseProxy_socket_fd = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
        if (reverseProxy_socket_fd == -1) {
            continue;
        }

        //Now that we have a socket, let's try to connect! If this is successful, we are done here.
        if (connect(reverseProxy_socket_fd, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1) { 
            break;
        }

        //If we are not successful with connecting, let's close this socket and try the next one.
        close(reverseProxy_socket_fd);
    }
    
    //If addres_iter is still NULL, that means we couldn't make a connection at all.
    if (addrs_iter == NULL) {
        fprintf(stderr, "Couldn't connect to reverse proxy");
        exit(1);
    }

    //Otherwise if successful, let's free all of the unused addresses.
    freeaddrinfo(addrs);

    //Proxy time!

    //Grab the headers from the client's fd
    char buffer[BUFFER_SIZE];
    int bytes_read;
    bytes_read = read(client_socket_fd, buffer, BUFFER_SIZE);
    //Now that we have the headers, we don't need anything else from the client.
    //Write what we have to our reverse proxy
    write(reverseProxy_socket_fd, buffer, bytes_read);

    //In the backend/proxy server, we will read the headers and then write them back into our client socket
    while (bytes_read = read(reverseProxy_socket_fd, buffer, BUFFER_SIZE)) {
        write(client_socket_fd, buffer, bytes_read);
    }

    close(client_socket_fd);

}

//In this example, we must bind() to associate the socket with its local address.
//bind() is used for defining the communication endpoint (sending data to and from that endpoint).
//When the server side binds, clients can use that address to connect to the server.
//Bind/connect are specific relationships that are important to differentiate

int main(int argc, char *argv[]) {
    //Check for arguments on the command line then store them
    char *server_port_str;
    char *reverseProxy_addr;
    char *reverseProxy_port_str;
    if (argc != 4) {
        fprintf(stderr, "Usage: %s   \n", argv[0]);
        exit(1);
    }
    server_port_str = argv[1];
    reverseProxy_addr = argv[2];
    reverseProxy_port_str = argv[3];

    //Create an addrinfo struct for the localhost
    struct addrinfo localhost;
    memset(&localhost, 0, sizeof(struct addrinfo));
    localhost.ai_family = AF_UNSPEC;
    localhost.ai_socktype = SOCK_STREAM;

    //Linux manpage for gettaddrinfo(2):
    //If the AI_PASSIVE flag is specified in hints.ai_flags, and node is NULL, 
    //then the returned socket addresses will be suitable for bind(2)ing a socket 
    //that will accept(2) connections. The returned socket address will contain
    //the "wildcard address" [...]. If node is not NULL, then the AI_PASSIVE flag is ignored.
    localhost.ai_flags = AI_PASSIVE;

    //Do the same getaddrinfo call for the localhost
    struct addrinfo *addrs;
    int getaddrinfo_error;
    getaddrinfo_error = getaddrinfo(NULL, server_port_str, &localhost, &addrs);

    //Now instead of trying to connect to them to make an outgoing connection, 
    //we try to bind so that we can accept incoming connections:
    struct addrinfo *addr_iter;
    int server_socket_fd;
    int so_reuseaddr;
    for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) {
        server_socket_fd = socket(addr_iter->ai_family, addr_iter->ai_socktype, addr_iter->ai_protocol);
        if (server_socket_fd == -1) {
            continue;
        }
        so_reuseaddr = 1;

        //Because we are initializing our server socket in the the main loop, the only way it gets closed
        //is if the OS times it out. This could mean restarting our program may take a long time.
        //Let's set the socket option to SO_REUSEADDR which 
        setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));
        if (bind(server_socket_fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0) 
        {
            break;
        }
        //NOTE: The risk in setting SO_REUSEADDR is that it creates an ambiguity
        //The metadata in a TCP packet's headers isn't sufficiently unique that the stack can reliably tell
        //whether the packet is stale and so should be dropped rather than be delivered to the new listener's
        //socket because it was clearly intended for a now-dead listener.
        //TODO: Look into different ways to solve this problem besides the default 2xMSL timeout!
        //Read: https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux

        close(server_socket_fd);
    }

    //Cleanup!
    if (addr_iter == NULL) {
        fprintf(stderr, "Couldn't bind\n");
        exit(1);
    }

    freeaddrinfo(addrs);

    //mark the socket so that it's "passive"
    //Meaning, it will listen for incoming connections instead of making outgoing connections
    listen(server_socket_fd, MAX_LISTEN_BACKLOG);

    //Proxy time!
    while (1) {
        //Load the first available connection coming to the server in the queue using accept
        int client_socket_fd = accept(server_socket_fd, NULL, NULL);
        //Did it work?
        if (client_socket_fd == -1) {
            perror("Could not accept");
            exit(1);
        }
        //Pass the fd of the connection that was successful into our proxy func
        handle_client_connection(client_socket_fd, reverseProxy_addr, reverseProxy_port_str);

    }

}