#include "client.h"

#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>

#include <pthread.h>
#include <iostream>
#include <assert.h>
#include <queue>
#include <vector>
#include <tuple>
#include <sstream>

using namespace std;

//Regular bold text
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define SERVER_PORT 8001
const LL buff_sz = 1048576;

pair<string, int> read_string_from_socket(int fd, int bytes) {
    std::string output;
    output.resize(bytes);

    size_t bytes_received = read(fd, &output[0], bytes - 1);
    if (bytes_received <= 0) {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        exit(-1);
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);

    return {output, bytes_received};
}

size_t send_string_on_socket(int fd, const string &s) {
    size_t bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0) {
        cerr << "Failed to SEND DATA on socket.\n";
        exit(-1);
    }

    return bytes_sent;
}

int get_socket_fd(struct sockaddr_in *ptr) {
    struct sockaddr_in server_obj = *ptr;

    // socket() creates an endpoint for communication and returns a file
    //        descriptor that refers to that endpoint.  The file descriptor
    //        returned by a successful call will be the lowest-numbered file
    //        descriptor not currently open for the process.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Error in socket creation for CLIENT");
        exit(-1);
    }
    int port_num = SERVER_PORT;
    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num); //convert to big-endian order

    // Converts an IP address in numbers-and-dots notation into either a
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    //https://stackoverflow.com/a/20778887/6427607

    /* connect to server */
    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0) {
        perror("Problem in connecting to the server");
        exit(-1);
    }

    return socket_fd;
}

void *client_request(void *args) {
    vector<string> request = *((vector<string> *) args);
    if (request.empty()) {
        cerr << "Invalid request\n";
        return nullptr;
    }
    int waitTime = stoi(request[0]);
    sleep(waitTime);

    string to_send;
    for (int i = 1; i < request.size(); i++)
        to_send += request[i] + " ";
    if (to_send.empty()) {
        cerr << "Invalid request\n";
        return nullptr;
    }
    struct sockaddr_in server_obj{};
    int socket_fd = get_socket_fd(&server_obj);
//    printf(BGRN "Connected to server\n" ANSI_RESET);

    send_string_on_socket(socket_fd, to_send);
    int num_bytes_read;
    string output_msg;
    tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);
//    cout << "Received: " << output_msg << endl;
    cout << output_msg << endl;
//    cout << "====" << endl;
    return nullptr;
}

int main(int argc, char *argv[]) {
    int numRequests;
    string numR;
    getline(cin, numR);
    numRequests = stoi(numR);

    auto clients = (pthread_t *) malloc(numRequests * sizeof(pthread_t));
    assert(clients);

    vector<vector<string>> args(numRequests);
    for (int i = 0; i < numRequests; i++) {
        string temp;
        getline(cin, temp);

        if (temp.empty()) {
            cerr << "Failed to read\n";
            continue;
        }

        istringstream iss(temp);
        string word;
        while (iss >> word) {
            if (!word.empty())
                args[i].push_back(word);
        }
    }

    for (int i = 0; i < numRequests; i++)
        pthread_create(&clients[i], nullptr, client_request, (void *) &args[i]);
    for (int i = 0; i < numRequests; i++)
        pthread_join(clients[i], nullptr);
    return 0;
}