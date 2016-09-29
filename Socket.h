/*
 * The Socket class encapsulates a few of the standard socket programming
 * functions and wrap them with some error handling for commodity
 */

#ifndef NETNINNY_SOCKET_H
#define NETNINNY_SOCKET_H

#include <string>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>
#include <zconf.h>
#include <sys/socket.h>

class Socket {

public:

    // Same functionality as the common bind(), using our IP and stream sockets
    bool bind(const char* port);
    // Same as listen()
    bool listen();
    // Accepts a connection and returns the file descriptor of the socket
    int accept(bool debug);
    // Close a socket (kind of)
    void close();
    // Peek's for a header in the socket
    bool get_header(std::string &header);
    // Establish a connection with the host
    bool connect(const char* hostname, bool debug);
    // Sends a header and returns if it succeeds or not
    bool send_header(const std::string &header);
    // Get's the text content for filtering purposes
    bool receive_text_data(std::string &data);

    // Getters and setters
    int get_fd();
    void set_fd(int fd);
    size_t get_max_size() const;


private:

    void *get_in_addr(struct sockaddr *sa);

    int file_descriptor;
    int backlog{15};
    size_t maxSize{4096};
    struct sockaddr_storage their_addr;

};


#endif //NETNINNY_SOCKET_H
