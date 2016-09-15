#include <netdb.h>
#include <cstdio>   // C++ version of <stdio.h>
#include <cstdlib>  // C++ version of <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <bits/sigaction.h>
#include <csignal>  // C++ version of <signal.h>
#include <cerrno>   // C++ version of <errno.h>
#include <wait.h>
#include <arpa/inet.h>
#include <sstream>

const int BACKLOG{5};
const int MAXDATASIZE{300};

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, const char *argv[]) {

    int sockfd, transfd;  // sockfd is the socket for establish the communication, transfd is the one with the data
    struct addrinfo hints, *servInfo;
    struct sockaddr_storage their_addr;  // connector's address information
    int yes{1};
    const char *PORT;
    struct sigaction sa;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

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

    // Server loop waiting for connections and messages
    printf("server: waiting for connections...\n");

    while(1) {
        sin_size = sizeof their_addr;
        transfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (transfd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Now we got a connection! Let's try to receive a message
        std::stringstream incomingMessage;
        long numBytes{MAXDATASIZE};
        char buffer[MAXDATASIZE];

        // If the message is too long we're going to keep receiving small parts until it's done
        while (numBytes >= MAXDATASIZE - 1) {
            memset(&buffer, 0, MAXDATASIZE);
            if ((numBytes = recv(transfd, buffer, (MAXDATASIZE - 1), 0)) == -1) {
                perror("recv");
                exit(1);
            }
            incomingMessage << buffer;
        }
        printf("the message is: '%s'\n", incomingMessage.str().c_str());

        close(transfd);

        // And the server waits looking for another connection
        // break;
    }
}