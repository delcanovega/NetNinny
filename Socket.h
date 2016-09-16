//
// Created by jdelcano on 2016-09-15.
//

#ifndef NETNINNY_SOCKET_H
#define NETNINNY_SOCKET_H

#include <string>
#include <sys/socket.h>

class Socket {

    int clientFD;
    int serverFD;
    int backlog;
    struct sockaddr_storage theirAddr;

public:

    bool bind(const char* port);
    bool listen();
    bool accept();
    bool fromClientToProxy(std::string& str);

private:

    void *get_in_addr(struct sockaddr *sa);

};


#endif //NETNINNY_SOCKET_H
