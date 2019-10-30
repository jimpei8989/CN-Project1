#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef verbose
#define verbose 0
#endif

typedef std::vector<struct sockaddr_in> sockaddrList;

const int MAX_BUFLEN = 1024;

int handleArguments(int argc, char **argv, int &number, int &timeout, sockaddrList &servers) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            sscanf(argv[++i], "%d", &number);
        } else if (strcmp(argv[i], "-t") == 0) {
            sscanf(argv[++i], "%d", &timeout);
        } else {
            struct sockaddr_in server;
            struct hostent *he;
            char *addr = strtok(argv[i], ":");
            char *port = strtok(NULL, ":");

            server.sin_family = AF_INET;

            if ((he = gethostbyname(addr)) != NULL) {
                server.sin_addr = **(struct in_addr**)he->h_addr_list;
                server.sin_port = htons((uint16_t)atoi(port));
            } else {
                fprintf(stderr, "! Error : gethostbyname\n");
                return -1;
            }
            servers.push_back(server);
        }
    }
    if (verbose) {
        fprintf(stderr, "Numbers : %d\n", number);
        fprintf(stderr, "Timeout : %d\n", timeout);
        fprintf(stderr, "Servers :\n");
        for (size_t i = 0; i < servers.size(); i++) {
            fprintf(stderr, " - %s : %d\n", inet_ntoa(servers[i].sin_addr), ntohs(servers[i].sin_port));
        }
    }
    return 0;
}

int64_t uGetDuration(struct timeval a, struct timeval b) {
    return (int64_t)(b.tv_sec - a.tv_sec) * 1000000 + (b.tv_usec - a.tv_usec);
}

int ping(struct sockaddr_in serverAddr, int timeout, int number) {
    char serverIP[128], clientIP[128];
    strncpy(serverIP, inet_ntoa(serverAddr.sin_addr), 128);
    int serverPort = ntohs(serverAddr.sin_port);
    if (verbose) {
        fprintf(stderr, "| PINGING -> %s : %d\n", serverIP, serverPort);
    }

    int clientSocket;
    // TCP socket -> connect
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "! Error : creating socket (IP : %s; PORT : %d)\n", serverIP, serverPort);
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "! Error : connecting (IP : %s; PORT : %d)\n", serverIP, serverPort);
        return -1;
    }

    // Get Info
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    getsockname(clientSocket, (struct sockaddr*)&clientAddr, &addrLen);

    strncpy(clientIP, inet_ntoa(clientAddr.sin_addr), 128);
    int clientPort = ntohs(clientAddr.sin_port);

    fd_set fds;

    char buf[MAX_BUFLEN + 1] = "";
    
    struct timeval beginTS;
    gettimeofday(&beginTS, NULL);

    for (int i = 0; number == 0 || i < number; i++) {
        struct timeval sendTS, recvTS;
        gettimeofday(&sendTS, NULL);

        int64_t uFromBeginToSend = uGetDuration(beginTS, sendTS);

        // Send Pocket
        sprintf(buf, "^%d|%d|%s|%d|%s|%d", i, (int)uFromBeginToSend, serverIP, serverPort, clientIP, clientPort);
        send(clientSocket, buf, strlen(buf), 0);
        if (verbose) {
            fprintf(stderr, "[ Successfully Sent (INDEX : %d; TS : %ld)\n", i, uFromBeginToSend);
            fprintf(stderr, "\t\t~ SERVER IP : %s; SERVER PORT : %d\n", serverIP, serverPort);
            fprintf(stderr, "\t\t~ MSG: \'%s\'\n", buf);
        }

        // Select
        FD_ZERO(&fds);
        FD_SET(clientSocket, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeout * 1000;

        bool done = false;
        do {
            int activity = select(clientSocket + 1, &fds, NULL, NULL, &tv);
            if (activity == -1) {
                fprintf(stderr, "! Error : selecting (SERVER IP : %s; SERVER PORT : %d\n", serverIP, serverPort);
                return -1;
            } else if (activity == 0) {
                // Connection Timeout
                done = true;
                fprintf(stdout, "timeout when connect to %s:%d\n", serverIP, serverPort);
                if (verbose) {
                    fprintf(stderr, "@ Connection Timeout (SERVER IP : %s; SERVER PORT : %d)\n", serverIP, serverPort);
                }
            } else {
                if (FD_ISSET(clientSocket, &fds)) {
                    gettimeofday(&recvTS, NULL);
                    int64_t uRTT = uGetDuration(sendTS, recvTS);
                    int64_t mRTT = uRTT / 1000;

                    int64_t uFromBeginToRecv = uGetDuration(beginTS, recvTS);

                    memset(buf, 0, MAX_BUFLEN);
                    ssize_t buflen = recv(clientSocket, buf, MAX_BUFLEN, 0);
                    if (buflen == 0) {
                        // Connection Lost
                        fprintf(stderr, "@ Connection Lost -> IP : %s; PORT : %d\n", \
                                serverIP, serverPort);
                        close(clientSocket);
                        return -1;
                    } else {
                        // Parse Index Packet
                        char *forSentence, *forWord;
                        char *start = strtok_r(buf, "^", &forSentence);
                        do {
                            char *indexStr = strtok_r(start, "|", &forWord);
                            if (i == atoi(indexStr))
                                done = true;
                        } while ((start = strtok_r(NULL, "^", &forSentence)) != NULL);

                        // Receive Response
                        if (done && mRTT <= timeout) {
                            fprintf(stdout, "recv from %s:%d, RTT = %.3f msec\n",
                                    serverIP, serverPort, (float)uRTT / 1000);
                            if (verbose) {
                                fprintf(stderr, "] Successful Received (INDEX : %d; SEND TS : %ld; RECV TS : %ld)\n",
                                        i, uFromBeginToSend, uFromBeginToRecv);
                                fprintf(stderr, "\t\t~ SERVER IP : %s; SERVER PORT : %d\n",
                                        serverIP, serverPort);
                            }
                            usleep((uint32_t)(timeout * 1000 - mRTT));
                        }
                    }
                }
            }
        } while (!done);
    }
    close(clientSocket);
    return 0;
}

int main(int argc, char **argv) {
    int number = 0, timeout = 1000;
    sockaddrList servers;

    if (handleArguments(argc, argv, number, timeout, servers) != 0) {
        fprintf(stderr, "! Error : handling arguments\n");
        return -1;
    }

    for (size_t i = 0; i < servers.size(); i++) {
        int pid = fork();
        if (pid == 0) {
            if (ping(servers[i], timeout, number) == 0) {
                if (verbose) {
                    fprintf(stderr, "# Success : Ping IP : %s; PORT : %d\n",
                        inet_ntoa(servers[i].sin_addr), ntohs(servers[i].sin_port));   
                }
                return 0;
            } else {
                fprintf(stderr, "! Error : pinging (IP : %s; PORT : %d)\n",
                        inet_ntoa(servers[i].sin_addr), ntohs(servers[i].sin_port));   
                return -1;
            }
        }
    }

    // Wait Until All Childs Finish
    for (size_t i = 0; i < servers.size(); i++)
        wait(NULL);

    return 0;
}

