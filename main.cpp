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

        if (!connectionSocket.bind(PORT)) {
            // ...
        }

        if (!connectionSocket.listen()) {
            // ...
        }

        printf("Server: Listening on port '%s' waiting for connections...\n", PORT);

        while (1) {

            if (!connectionSocket.accept()) {
                // ...
            }

            // get message from the client to the proxy
            std::string clientSearch;
            if (!connectionSocket.fromClientToProxy(clientSearch)) {
                // ...
            }
            printf("Server: Browser search: %s\n", clientSearch.c_str());
        }

        // TODO Close connections
    }
}