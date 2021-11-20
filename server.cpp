#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

/////////////////////////////
#include <iostream>
#include <tuple>
#include <cassert>
#include <queue>
#include <map>
#include <sstream>
#include "globals.h"

using namespace std;
/////////////////////////////

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << (x) << endl
#define part cout << "-----------------------------------" << endl;

#define MAX_CLIENTS 4
#define PORT_ARG 8001

const int initial_msg_len = 256;

const LL buff_sz = 1048576;
pthread_mutex_t qLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t requestCV = PTHREAD_COND_INITIALIZER;
int requestCount = 0;
typedef struct request {
    int clientFd;
    int index;
    string req;
} Request;
queue<Request> requestQ;
pthread_mutex_t *keyLocks;
#define MAX_KEY 100

///////////////////////////////////////////////////
pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

///////////////////////////////

void handle_connection(int client_socket_fd) {
    Request newReq;
    newReq.clientFd = client_socket_fd;
    newReq.index = requestCount++;
    pthread_mutex_lock(&qLock);
    requestQ.push(newReq);
    pthread_cond_signal(&requestCV);
    pthread_mutex_unlock(&qLock);
}

map<string, int> requestArg;

[[noreturn]] void *service_client(void *input) {
    int ind = *((int *)input);
    while (true) {
        pthread_mutex_lock(&qLock);
        while (requestQ.empty())
            pthread_cond_wait(&requestCV, &qLock);
        Request currReq = requestQ.front();
        requestQ.pop();
        pthread_mutex_unlock(&qLock);
        string cmd;
        int received_num;
        cout << "im awake2\n";
        cout << currReq.clientFd << "\n";
        tie(cmd, received_num) = read_string_from_socket(currReq.clientFd, buff_sz);
        int ret_val = received_num;
        // debug(ret_val);
        // printf("Read something\n");
        cout << "im awake\n";
        if (ret_val <= 0) {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            close(currReq.clientFd);
            printf(BRED "Disconnected from client" ANSI_RESET "\n");
            continue;
        }
        cout << "here" << endl;
        cout << "Client sent : " << cmd << endl;
        currReq.req = cmd;
        string to_send;
        istringstream iss(currReq.req);
        vector<string> args;
        string temp;
        to_send += to_string(currReq.index) + ":" + to_string(ind) + ":";
        while (iss >> temp) {
            args.push_back(temp);
        }
        for (auto &i: args) cout << i << "|";
        cout << "\n";
        if (args.empty() || requestArg.find(args[0]) == requestArg.end() || requestArg[args[0]] != args.size()) {
            cout << "Invalid request\n";
        }
        else {
            if (args[0] == "insert") {
                int k = stoi(args[1]);
                if (k > 100 || k < 0) {
                    cerr << "Invalid request\n";
                } else if (serverDictionary.find(k) != serverDictionary.end()) {
                    to_send += "Key already exists";
                } else {
                    pthread_mutex_lock(&keyLocks[k]);
                    serverDictionary[k] = args[2];
                    pthread_mutex_unlock(&keyLocks[k]);
                    to_send += "Insertion successful";
                }
            } else if (args[0] == "delete") {
                int k = stoi(args[1]);
                if (k > 100 | k < 0) {
                    to_send += "Key out of bounds";
                } else if (serverDictionary.find(k) == serverDictionary.end()) {
                    to_send += "No such key exists";
                } else {
                    pthread_mutex_lock(&keyLocks[k]);
                    serverDictionary.erase(k);
                    pthread_mutex_unlock(&keyLocks[k]);
                    to_send += "Deletion successful";
                }
            } else if (args[0] == "update") {
                int k = stoi(args[1]);
                if (k > 100 || k < 0) {
                    cerr << "Invalid request\n";
                } else if (serverDictionary.find(k) == serverDictionary.end()) {
                    to_send += "No such key exists";
                } else {
                    pthread_mutex_lock(&keyLocks[k]);
                    serverDictionary[k] = args[2];
                    pthread_mutex_unlock(&keyLocks[k]);
                    to_send += args[2];
                }
            } else if (args[0] == "concat") {
                int k1 = stoi(args[1]), k2 = stoi(args[2]);
                if (k1 > 100 || k1 < 0 || k2 > 100 || k2 < 0) {
                    cerr << "Invalid request\n";
                } else if (serverDictionary.find(k1) == serverDictionary.end() ||
                           serverDictionary.find(k2) == serverDictionary.end()) {
                    to_send += "Concat failed as at least one of the keys does not exist";
                } else {
                    pthread_mutex_lock(&keyLocks[min(k1, k2)]);
                    pthread_mutex_lock(&keyLocks[max(k1, k2)]);
                    string val = serverDictionary[k1];
                    serverDictionary[k1] += serverDictionary[k2];
                    serverDictionary[k2] += val;
                    to_send += serverDictionary[k2];
                    pthread_mutex_unlock(&keyLocks[max(k1, k2)]);
                    pthread_mutex_unlock(&keyLocks[min(k1, k2)]);
                }
            } else if (args[0] == "fetch") {
                int k = stoi(args[1]);
                if (k > 100 | k < 0) {
                    to_send += "Key out of bounds";
                } else if (serverDictionary.find(k) == serverDictionary.end()) {
                    to_send += "No such key exists";
                } else {
                    pthread_mutex_lock(&keyLocks[k]);
                    to_send += serverDictionary[k];
                    pthread_mutex_unlock(&keyLocks[k]);
                }
            } else {
                to_send += "Invalid request";
            }
        }

        int sent_to_client = send_string_on_socket(currReq.clientFd, to_send);
        // debug(sent_to_client);
        if (sent_to_client == -1) {
            perror("Error while writing to client. Seems socket has been closed");
        }
        close(currReq.clientFd);
        printf(BRED "Disconnected from client" ANSI_RESET "\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        cerr << "Error: Missing worker thread argument\n";
        cerr << "Usage: ./server [N]\n";
        return 0;
    }
    else if (argc > 2) {
        cerr << "Error: Too many arguments\n";
        return 0;
    }
    requestArg["insert"] = 3;
    requestArg["delete"] = 2;
    requestArg["update"] = 3;
    requestArg["concat"] = 3;
    requestArg["fetch"] = 2;
    keyLocks = (pthread_mutex_t *) malloc((MAX_KEY + 1) * sizeof(pthread_mutex_t));
    assert(keyLocks);
    for (int i = 0; i <= MAX_KEY; i++) {
        keyLocks[i] = PTHREAD_MUTEX_INITIALIZER;
    }
    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj{}, client_addr_obj{};
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    //get welcoming socket
    //get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); //process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    //CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    int numWorkers = atoi(argv[1]);
    pthread_t *workers = (pthread_t *) malloc(numWorkers * sizeof(pthread_t));
    assert(workers);
    for (int i = 0; i < numWorkers; i++) {
        int rc = pthread_create(&workers[i], NULL, service_client, &i);
        assert(rc == 0);
    }

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    clilen = sizeof(client_addr_obj);

    while (true) {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
of the server process. When the server “hears” the knocking, it creates a new door—
more precisely, a new socket that is dedicated to that particular client.
        */
        //accept is a blocking call
        printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0) {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }

        printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        handle_connection(client_socket_fd);
    }

    close(wel_socket_fd);
    return 0;
}