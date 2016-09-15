//
// Created by jdelcano on 2016-09-15.
//

#include <cstdio>

#include "Socket.h"

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

        // TODO Bind the socket

        // TODO Listen

        // TODO Accept loop

            // TODO Recieve message

        // TODO Close connections
    }
}