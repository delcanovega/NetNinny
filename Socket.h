//
// Created by jdelcano on 2016-09-15.
//

#ifndef NETNINNY_SOCKET_H
#define NETNINNY_SOCKET_H

#include <string>
#include <sys/socket.h>

class Socket {

public:

    bool bind(const char* port);
    bool listen();
    int accept();
    void setFD(int fd);
    void close();
    bool getHeader(std::string& header);
    bool connect(const char* hostname);
    bool sendHeader(const std::string& header);
    size_t getMaxSize() const;
    long receivePacket(char* packet) const;
    long sendPacket(char* packet, long numBytes) const;

private:

    void *get_in_addr(struct sockaddr *sa);

    int fileDescriptor;
    int backlog;
    size_t maxSize{4096};
    struct sockaddr_storage theirAddr;

};


#endif //NETNINNY_SOCKET_H
