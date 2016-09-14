#include <netdb.h>
#include <cstdio>   // C++ version of <stdio.h>
#include <cstdlib>  // C++ version of <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <bits/sigaction.h>
#include <signal.h>
#include <cerrno>   // C++ version of <errno.h>
#include <wait.h>

#define BACKLOG 5

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main(int argc, const char *argv[]) {

    int sockfd, transfd;  // sockfd is the socket for establish the communication, transfd is the one with the data
    struct addrinfo hints, *servInfo;
    struct sockaddr_storage their_addr;  // connector's address information
    int yes{1};
    const char *PORT;
    struct sigaction sa;

    // Error control for the inputs
    if (argc != 2) {
        printf(stderr, "usage: client hostname\n");
        exit(1);
    }

    PORT = argv[1];

    // Let's prepare the socket
    struct addrinfo *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // My IP adress

    if ((status = getaddrinfo(NULL, PORT, &hints, &servInfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // Now we loop through the results and try to bind the socket
    for (p = servInfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servInfo);  // We've finished with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // Listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // TODO server loop waiting for connections

    return 0;
}