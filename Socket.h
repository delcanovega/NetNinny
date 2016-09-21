//
// Created by jdelcano on 2016-09-15.
//

#ifndef NETNINNY_SOCKET_H
#define NETNINNY_SOCKET_H

#include <string>
#include <sys/socket.h>

class Socket {

    int fileDescriptor;
    int backlog;
    struct sockaddr_storage theirAddr;

public:

    bool bind(const char* port);
    bool listen();
    int accept();
    void setFD(int fd);
    void close();
    bool getHeader(std::string& header);
    bool connect(const char* hostname);
    bool sendHeader(const std::string& header);

private:

    void *get_in_addr(struct sockaddr *sa);

};


#endif //NETNINNY_SOCKET_H
