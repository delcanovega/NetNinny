//
// Created by jdelcano on 2016-09-15.
//

#include <cstdio>
#include <zconf.h>

#include "Socket.h"

void execute(Socket& clientSocket);
bool getHost(const std::string& header, std::string& host);
std::string handleHeader(const std::string& header, const std::string& host);
bool hasContent(const std::string& header);


int main(int argc, const char *argv[]) {

    // We expect to get ONLY the port number from the arguments
    const char* PORT;
    if (argc != 2) {
        perror("ERROR: Port number didn't found in the arguments vector");
    }
    else {
        PORT = argv[1];

        // TODO create a class 'Socket' that encapsulates the common features
        Socket connectionSocket;

        if (!connectionSocket.bind(PORT)) {
            // ...
        }

        if (!connectionSocket.listen()) {
            // ...
        }

        printf("[ NetNinny ]: Listening on port '%s' waiting for connections...\n", PORT);

        while (1) {
            Socket clientSocket;
            int fileDescriptor;
            fileDescriptor = connectionSocket.accept();
            if (fileDescriptor == -1) {
                continue;
            }

            clientSocket.setFD(fileDescriptor);
            // Should be a valid socket

            if (!fork()) {
                connectionSocket.close();
                execute(clientSocket);  // Here starts the fun!
                break;
            }

            else {
                sleep(1);
            }
        }
    }
}

void execute(Socket& clientSocket) {
    // Get HTTP header
    std::string header;
    if (!clientSocket.getHeader(header)) {
        printf("ERROR: in execute(). Unable to get the HTTP header.\n");
        return;
    }
    printf("[ NetNinny ]: Received header:\n%s\n", header.c_str());

    // Filter

    // Get the host from the header
    std::string host;
    if (!getHost(header, host)) {
        printf("ERROR: in execute(). Unable to get the host.\n");
        return;
    }

    // Connect to the host
    Socket serverSocket;
    if (!serverSocket.connect(host.c_str())) {
        // ...
    }

    // Modify header
    header = handleHeader(header, host);
    printf("[ NetNinny ]: Modified header:\n%s\n", header.c_str());
    if (hasContent(header)) {
        // Handle content
    }

}

bool getHost(const std::string& header, std::string& host) {
    std::string goal{"Host: "};
    long goalIndex{header.find(goal)};
    if (goalIndex == std::string::npos) {
        return false;
    }

    unsigned long hostIndex{goalIndex + goal.size()};    // From this position on the host name starts
    unsigned long lenght{header.find("\r", hostIndex)};  // [0, '\r')
    lenght -= hostIndex;                                 // [hostStarts, '\r')

    host.clear();
    host = header.substr(hostIndex, lenght);
    return true;
}

std::string handleHeader(const std::string& header, const std::string& host) {
    std::string handledHeader{header};
    std::string goal{"GET http://" + host};
    unsigned long index{host.find(goal)};
    if (index != std::string::npos) {
        // If we can't find it we try to reconstruct it
        handledHeader = "GET " + host.substr(goal.length());
    }

    goal.clear();
    goal = "Proxy-Connection";
    index = handledHeader.find(goal);
    if (index == std::string::npos) {
        goal.clear();
        goal = "Connection: ";
        index = handledHeader.find(goal);
        if (index == std::string::npos) {
            return handledHeader;
        }
    }

    // Change the connection value from 'keep-alive' to 'close'
    long lenght{handledHeader.find("\r", index)};  // [index, "\r")
    lenght -= index;                               // [0, "\r" - index)
    std::string prefix{handledHeader.substr(0, index)};
    std::string sufix{handledHeader.substr(index + lenght, handledHeader.size())};
    handledHeader.clear();
    handledHeader = prefix + "Connection: close" + sufix;

    return handledHeader;
}

bool hasContent(const std::string& header) {
    std::string goal{"Content-Length: "};
    long index{header.find(goal)};

    if (index != std::string::npos) {
        char value{header[index + goal.size()]};
        if (value == '0') {
            return false;
        }
        else {
            return true;
        }
    }

    return false;
}