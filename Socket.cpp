//
// Created by jdelcano on 2016-09-15.
//

#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>
#include <zconf.h>

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
        if ((fileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[*] Socket error: in call 'socket'\n");
            continue;
        }

        if (setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("ERROR in call 'setSockOpt'\n");
            return false;
        }

        if (::bind(fileDescriptor, p->ai_addr, p->ai_addrlen) == -1) {
            this->close();
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
    if (::listen(fileDescriptor, backlog) == -1) {
        perror("ERROR in call 'listen'\n");
        return false;
    }
    return true;
}

int Socket::accept() {
    int serverFD;
    socklen_t addrSize = sizeof theirAddr;  // Not very sure how theirAddr works...

    if ((serverFD = ::accept(fileDescriptor, (struct sockaddr *)&theirAddr, &addrSize)) == -1) {
        perror("ERROR in the call 'accept'\n");
    }
    else {
        char readableIP[INET6_ADDRSTRLEN];
        inet_ntop(theirAddr.ss_family, get_in_addr((struct sockaddr *)&theirAddr), readableIP, sizeof readableIP);
        printf("[ SOCKET ]: Connection established with IP: '%s'\n", readableIP);
    }

    return serverFD;
}

// Beej's code, get sockaddr, IPv4 or IPv6:
void* Socket::get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void Socket::setFD(int fd) {
    fileDescriptor = fd;
}

void Socket::close() {
    if (fileDescriptor != -1) {
        ::close(fileDescriptor);
        setFD(-1);
    }
}

bool Socket::getHeader(std::string &header) {
    size_t headerMaxSize{8192};  // 8K or 16K should be good enough according to stackOverflow
    long numBytes;
    char buffer[headerMaxSize];

    memset(&buffer, 0, headerMaxSize);
    if ((numBytes = recv(fileDescriptor, buffer, headerMaxSize, MSG_PEEK)) == -1) {
        perror("ERROR: first recv() inside getHeader()");
        return false;
    }

    std::string str(buffer, numBytes);
    unsigned long headerLenght{str.find("\r\n\r\n") + 4};
    if (headerLenght > (numBytes + 4)) {
        printf("HTTP header not found.\n");
        return false;
    }

    char buf[headerLenght + 1];
    memset(&buf, 0, headerLenght + 1);
    if ((numBytes = recv(fileDescriptor, buf, headerLenght, 0)) == -1) {
        perror("ERROR: second recv() inside getHeader()");
        return false;
    }

    header.clear();
    std::string tmp(buf, numBytes);
    header.append(tmp);
    return true;
}

bool Socket::connect(const char *hostname) {
    struct addrinfo hints, *servInfo, *p;
    int result;

    // Preparation
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // My IP

    if ((result = getaddrinfo(hostname, "http" , &hints, &servInfo)) != 0) {
        fprintf(stderr, "ERROR in call 'getAddrInfo': %s\n", gai_strerror(result));
        return false;
    }

    // Now we have to "run the list" trying to make a connection
    for (p = servInfo; p != NULL ; p->ai_next) {
        if ((fileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[*] Socket error: in call 'socket'\n");
            continue;
        }

        if (::connect(fileDescriptor, p->ai_addr, p->ai_addrlen) == -1) {
            this->close();
            perror("Failed to bind, trying to reconnect...\n");
            continue;
        }

        break;  // If we're here the connect is done
    }

    if (p == NULL) {
        fprintf(stderr, "Unable to connect with the host\n");
        return false;
    }

    char readableIP[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), readableIP, sizeof readableIP);
    printf("[ NetNinny ]: Connection established with the host: '%s'\n", readableIP);

    freeaddrinfo(servInfo);  // To prevent memory leaks

    return true;
}
