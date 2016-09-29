/*
 * The Socket class encapsulates a few of the standard socket programming
 * functions and wrap them with some error handling for commodity
 */

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
        fprintf(stderr, "[!] ERROR in call 'getAddrInfo': %s\n", gai_strerror(result));
        return false;
    }

    // Now we have to "run the list" trying to make a connection
    for (p = servInfo; p != NULL ; p = p->ai_next) {
        if ((file_descriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[!] ERROR in call 'socket'\n");
            continue;
        }

        if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("[!] ERROR in call 'setSockOpt'\n");
            return false;
        }

        if (::bind(file_descriptor, p->ai_addr, p->ai_addrlen) == -1) {
            this->close();
            perror("[!] ERROR: Failed to bind, trying to reconnect...\n");
            continue;
        }

        break;  // If we're here probably the binding is done
    }

    if (p == NULL) {
        fprintf(stderr, "Unable to bind\n");
        return false;
    }

    freeaddrinfo(servInfo);  // To prevent memory leaks

    return true;
}

bool Socket::listen() {
    if (::listen(file_descriptor, backlog) == -1) {
        perror("[!] ERROR in call 'listen'\n");
        return false;
    }
    return true;
}

int Socket::accept(bool debug) {
    int server_fd;
    socklen_t addr_size = sizeof their_addr;

    if ((server_fd = ::accept(file_descriptor, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
        perror("[!] ERROR in the call 'accept'\n");
    }
    else {
        char readableIP[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), readableIP, sizeof readableIP);
        if (debug)
            printf("[ NetNinny ]: Connection established with IP: '%s'\n", readableIP);
    }

    return server_fd;
}

// Beej's code, get sockaddr, IPv4 or IPv6:
void* Socket::get_in_addr(struct sockaddr *sa) {

    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void Socket::close() {
    if (file_descriptor != -1) {
        ::close(file_descriptor);
        set_fd(-1);
    }
}

bool Socket::get_header(std::string &header) {
    size_t header_max_size{8192};  // 8K or 16K should be good enough according to stackOverflow
    long num_bytes;
    char buffer[header_max_size];

    memset(&buffer, 0, header_max_size);
    if ((num_bytes = recv(file_descriptor, buffer, header_max_size, MSG_PEEK)) == -1) {
        perror("[!] ERROR: first recv() inside get_header()");
        return false;
    }

    std::string str(buffer, num_bytes);
    unsigned long header_lenght{str.find("\r\n\r\n") + 4};
    if (header_lenght > (num_bytes + 4)) {
        printf("HTTP header not found.\n");
        return false;
    }

    char buf[header_lenght + 1];
    memset(&buf, 0, header_lenght + 1);
    if ((num_bytes = recv(file_descriptor, buf, header_lenght, 0)) == -1) {
        perror("[!] ERROR: second recv() inside get_header()");
        return false;
    }

    header.clear();
    std::string tmp(buf, num_bytes);
    header.append(tmp);
    return true;
}

bool Socket::connect(const char *hostname, bool debug) {
    struct addrinfo hints, *servInfo, *p;
    int result;

    // Preparation
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // My IP

    if ((result = getaddrinfo(hostname, "http" , &hints, &servInfo)) != 0) {
        fprintf(stderr, "[!] ERROR in call 'getAddrInfo': %s\n", gai_strerror(result));
        return false;
    }

    // Now we have to "run the list" trying to make a connection
    for (p = servInfo; p != NULL ; p->ai_next) {
        if ((file_descriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[*] Socket error: in call 'socket'\n");
            continue;
        }

        if (::connect(file_descriptor, p->ai_addr, p->ai_addrlen) == -1) {
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

    char readable_ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), readable_ip, sizeof readable_ip);
    if (debug)
        printf("[ NetNinny ]: Connection established with the host: '%s'\n", readable_ip);

    freeaddrinfo(servInfo);  // To prevent memory leaks

    return true;
}

bool Socket::send_header(const std::string &header) {
    bool status{false};
    if (this->file_descriptor != -1) {
        if (send(file_descriptor, header.c_str(), header.size(), 0) == -1) {
            perror("[!] ERROR: in send() inside send_header().\n");
        }
        else {
            status = true;
        }
    }
    return status;
}

size_t Socket::get_max_size() const {
    return this->maxSize;
}

bool Socket::receive_text_data(std::string &data) {
    unsigned long bytes_moved{get_max_size()};
    char buffer[bytes_moved];

    while (bytes_moved != 0) {
        memset(buffer, 0, get_max_size());
        if ((bytes_moved = recv(file_descriptor, buffer, get_max_size(), 0)) == -1) {
            perror("ERROR: receiving data in receive_text_data()\n");
            return false;
        }
        std::string tmp(buffer, bytes_moved);
        data.append(tmp);
    }
    return true;
}

// Getters and setters

int Socket::get_fd() {
    return file_descriptor;
}

void Socket::set_fd(int fd) {
    file_descriptor = fd;
}
