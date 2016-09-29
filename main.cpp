/*
 * NetNinny:
 * A proxy that prevents your browser to show contents that might insult your intelligence.
 * And also Norrköping. Because why not.
 */

#include <vector>
#include <algorithm>

#include "Socket.h"


// Useful resources
const std::vector<std::string> BLACKLIST{"spongebob", "britney spears", "paris hilton", "norrköping", "norrkoping"};
const std::string BAD_URL{"HTTP/1.1 302 Found\r\nLocation: http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error1.html\r\n\r\n"};
const std::string BAD_CNT{"HTTP/1.1 302 Found\r\nLocation: http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error2.html\r\n\r\n"};
bool debug{false};


// FUNCTIONS USED BY THE PROXY:

// The actual "proxy" feature, handles the request and response of the browser and server
void execute(Socket& client_socket);
// Extracts a copy of the host part from the full header
bool get_host(const std::string &header, std::string &host);
// Modify the header, setting the connection type from 'keep-alive' to 'closed'
std::string handle_header(const std::string &header, const std::string &host);
// Checks the content-length field in the header
bool has_content(const std::string &header);
// Let's the content flow from one socket to the other
void deliver_content(Socket &from, Socket &to);
// Extracts a copy of the GET and HOST field from the header for filtering later
std::string get_request(const std::string &header);
// Receives a string and checks if it contains any banned words
bool has_illegal_contents(const std::string &str, const std::vector<std::string> banned_words);
// Checks if the type of the content is suitable for filtering
bool has_text_to_filter(const std::string &header);
// A function that tries to prevent the duplicate address problem (NOT WORKING)
void prevent_double_url(std::string &header);


// Implementations:
int main(int argc, const char *argv[]) {

    // We expect to get the port number from the arguments
    const char* PORT;
    if (argc < 2 || argc > 3) {
        printf("USAGE:         %s PORT\nDEBUG MODE:    %s PORT -d\n", argv[0], argv[0]);
        return 1;
    }
    else {
        PORT = argv[1];
        if (argc == 3) {
            std::string mode = argv[2];
            if (mode == "-d")
                debug = true;
            else {
                printf("Unknown argument.\n");
                printf("USAGE:         %s PORT\nDEBUG MODE:    %s PORT -d\n", argv[0], argv[0]);
                return 1;
            }
        }

        // Creates a socket and waits for a connection
        Socket connection_socket;

        if (!connection_socket.bind(PORT)) {
            // ...
        }
        if (!connection_socket.listen()) {
            // ...
        }
        if (debug)
            printf("[ NetNinny ]: Listening on port '%s' waiting for connections...\n", PORT);

        while (1) {
            Socket client_socket;
            int file_descriptor;
            file_descriptor = connection_socket.accept(debug);
            if (file_descriptor == -1) {
                continue;
            }

            client_socket.set_fd(file_descriptor);
            // Should be a valid socket

            if (!fork()) {
                connection_socket.close();
                execute(client_socket);  // Here starts the fun!
                exit(EXIT_SUCCESS);
            }
        }
    }
}

void execute(Socket& client_socket) {
    // - - - - - - - - - - - - - - - - - - - - - -
    // TRANSITION FROM BROWSER -> PROXY -> SERVER
    // - - - - - - - - - - - - - - - - - - - - - -

    // Get HTTP header
    std::string header;
    if (!client_socket.get_header(header)) {
        perror("[!] ERROR: in execute(). Unable to get the HTTP header.\n");
        return;
    }
    if (debug)
        printf("[ NetNinny ]: Received header from the browser:\n%s\n", header.c_str());


    // Filter the GET request
    std::string request{get_request(header)};
    if (has_illegal_contents(request, BLACKLIST)) {
        // If we pick up an illegal URL, then we redirect to the proper error page
        printf("[ NetNinny ]: Bad URL detected, redirecting...\n");
        client_socket.send_header(BAD_URL);
    }


    // Get the host from the header
    std::string host;
    if (!get_host(header, host)) {
        //perror("[!] ERROR: in execute(). Unable to get the host.\n");
        // Commented because it'n not a real error, sometimes the proxy picks empty host names
        return;
    }


    // Connect to the host
    Socket server_socket;
    if (!server_socket.connect(host.c_str(), debug)) {
        // ...
    }


    // Modify header
    header = handle_header(header, host);
    if (debug)
        printf("[ NetNinny ]: Modified header sent to the server:\n%s\n", header.c_str());


    // Send to server
    server_socket.send_header(header);
    // Has content?
    if (has_content(header)) {
        deliver_content(client_socket, server_socket);
    }

    // - - - - - - - - - - - - - - - - - - - - - -
    // TRANSITION FROM SERVER -> PROXY -> BROWSER
    // - - - - - - - - - - - - - - - - - - - - - -

    if (!server_socket.get_header(header)) {
        perror("[!] ERROR: in execute(). Invalid HTTP header.\n");
        return;
    }
    if (debug)
        printf("[ NetNinny ]: Received header from the server:\n%s\n", header.c_str());


    // Filter again
    if (has_text_to_filter(header)) {
        std::string data;
        if (server_socket.receive_text_data(data)) {
            if (has_illegal_contents(data, BLACKLIST)) {
                printf("[ NetNinny ]: Bad content detected, redirecting...\n");
                header = BAD_CNT;
            }
            client_socket.send_header(header);
            // As we have already downloaded the content we cannot use the deliver_content() function again, so
            // we have to send the content as a simple string to the browser. The send_header() function is also
            // suitable for this purpose.
            client_socket.send_header(data);

        }
    }
    else {
        client_socket.send_header(header);
        deliver_content(server_socket, client_socket);
    }


    // Close connections
    client_socket.close();
    server_socket.close();

}

bool get_host(const std::string &header, std::string &host) {
    std::string goal{"Host: "};
    unsigned long goal_index{header.find(goal)};
    if (goal_index == std::string::npos) {
        return false;
    }

    unsigned long host_index{goal_index + goal.size()};   // From this position on the host name starts
    unsigned long length{header.find("\r", host_index)};  // [0, '\r')
    length -= host_index;                                 // [hostStarts, '\r')

    host.clear();
    host = header.substr(host_index, length);
    return true;
}

std::string handle_header(const std::string &header, const std::string &host) {
    std::string new_header{header};
    std::string goal{"GET http://" + host};
    unsigned long index{host.find(goal)};
    if (index != std::string::npos) {
        new_header = "GET " + host.substr(goal.length());
    }  // This might be causing the duplicate directions, not sure

    // Let's change the type of connection from 'keep-alive' to 'close' (if we need to)
    goal.clear();
    goal = "Proxy-Connection";
    index = new_header.find(goal);
    if (index == std::string::npos) {
        goal.clear();
        goal = "Connection: ";
        index = new_header.find(goal);
        if (index == std::string::npos) {
            return new_header;  // In this here there is no need
        }
    }

    unsigned long length{new_header.find("\r", index)};  // [index, "\r")
    length -= index;                                     // [0, "\r" - index)
    std::string prefix{new_header.substr(0, index)};
    std::string suffix{new_header.substr(index + length, new_header.size())};
    new_header.clear();  // Erases the content of the string
    new_header = prefix + "Connection: close" + suffix;

    return new_header;
}

bool has_content(const std::string &header) {
    std::string goal{"Content-Length: "};
    unsigned long index{header.find(goal)};

    if (index != std::string::npos) {
        // If the first number of the content length is != 0
        if (header[index + goal.size()] != '0') {
            // Then there's content!
            return true;
        }
    }

    return false;
}

void deliver_content(Socket &from, Socket &to) {
    bool in_course{true};
    long bytes_moved{-1};
    char packet[from.get_max_size()];  // It's basically a buffer

    // We send fragments until we're done or there's an error
    while (in_course) {
        memset(packet, 0, from.get_max_size());  // Erases the content of the buffer

        bytes_moved = recv(from.get_fd(), packet, from.get_max_size(), 0);
        if (bytes_moved == -1) {
            perror("ERROR: in recv() inside deliver_content().\n");
            in_course = false;
        }
        else if (bytes_moved == 0) {
            in_course = false;
        }
        else {
            bytes_moved = send(to.get_fd(), packet, bytes_moved, 0);
            if (bytes_moved == -1) {
                perror("ERROR: in send() inside deliver_content().\n");
                in_course = false;
            }
        }
    }
}

std::string get_request(const std::string &header) {
    // We are only interested in the GET and Host part, so let's get a substr() of that two fields
    unsigned long tmp{header.find("Host:")};
    unsigned long end{header.find("\r\n", tmp)};

    return header.substr(0, end);
}

bool has_illegal_contents(const std::string &str, const std::vector<std::string> banned_words) {
    std::string str_copy{str};
    std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(), ::tolower);
    // We create a copy of the string and transform it to lowercase, that way the filter is not case sensitive
    // (the original string is not modified)

    for (unsigned int i{0}; i < banned_words.size(); ++i) {
        unsigned long result{str_copy.find(banned_words.at(i))};
        if (result != std::string::npos)
            return true;
    }
    return false;
}

bool has_text_to_filter(const std::string &header) {
    // We only want to filter text files
    std::string goal{"Content-Type: text/"};
    unsigned long index{header.find(goal)};

    if (index != std::string::npos) {
        goal = "gzip";  // But we're not interested in compressed files
        index = header.find(goal);
        if (index == std::string::npos)
            return true;  // So if it's text and it's not compressed, we can handle it
    }

    return false;
}

void prevent_double_url(std::string &header) {
    std::string goal{"http://"};
    unsigned long first_match{header.find(goal)};
    if (first_match != std::string::npos) {
        unsigned long second_match{header.find(goal, first_match + 1)};
        if (second_match != std::string::npos) {
            std::string first_part{header.substr(0, second_match)};
            unsigned long pos{header.find("HTTP", second_match + 3)};
            std::string second_part{header.substr(pos)};
            header = first_part + second_part;
        }
    }
}
