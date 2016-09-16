//
// Created by jdelcano on 2016-09-15.
//

#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>

#include "Socket.h"

bool Socket::bind(const char *port) {

    struct addrinfo hints, *servInfo, *p;
    int result;
    int yes{1};

    // Preparation
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // My IP

    if ((result = getaddrinfo(NULL, port, &hints, &servInfo)) != 0) {
        fprintf(stderr, "ERROR in call 'getAddrInfo': %s\n", gai_strerror(result));
        return false;
    }
    
    // Now we have to "run the list" trying to make a connection
    for (p = servInfo; p != NULL ; p->ai_next) {
        if ((clientFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("ERROR in call 'socket'\n");
            continue;
        }

        if (setsockopt(clientFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("ERROR in call 'setSockOpt'\n");
            return false;
        }

        if (::bind(clientFD, p->ai_addr, p->ai_addrlen) == -1) {
            // TODO this.close(clientFD);
            perror("Failed to bind, trying to reconnect...\n");
            continue;
        }

        break;  // If we're here the binding is done
    }

    if (p == NULL) {
        fprintf(stderr, "Unable to bind\n");
        return false;
    }

    freeaddrinfo(servInfo);  // To prevent memory leaks

    return true;
}

bool Socket::listen() {
    if (::listen(clientFD, backlog) == -1) {
        perror("ERROR in call 'listen'\n");
        return false;
    }
    return true;
}

bool Socket::accept() {
    socklen_t addrSize = sizeof theirAddr;  // Not very sure how theirAddr works...

    if ((serverFD = ::accept(clientFD, (struct sockaddr *)&theirAddr, &addrSize)) == -1) {
        perror("ERROR in the call 'accept'\n");
        return false;
    }

    char readableIP[INET6_ADDRSTRLEN];
    inet_ntop(theirAddr.ss_family, get_in_addr((struct sockaddr *)&theirAddr), readableIP, sizeof readableIP);
    printf("Server: Connection established with '%s'\n", readableIP);

    return true;
}

// Beej's code, get sockaddr, IPv4 or IPv6:
void* Socket::get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool Socket::fromClientToProxy(std::string &str) {
    str = "";
    char buffer[512];

    long byteCount{recv(clientFD, buffer, sizeof buffer, 0)};
    if (byteCount == -1) {
        perror("ERROR in call 'recv' (from client to proxy)\n");
        return false;
    }
    printf("recv()'d %ld bytes of data in buffer\n", byteCount);

    std::string tmp(buffer, byteCount);
    str.append(tmp);

    return true;
}