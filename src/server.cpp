#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <algorithm>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef verbose
#define verbose 0
#endif

#ifndef stress
#define stress 0
#endif


const int MAX_CLIENTS = 256;
const int MAX_BUFLEN = 1024;

int main(int argc, char **argv) {
    srand(time(NULL));

    // Handle Arguments
    uint16_t listenPort = 0;
    if (argc != 2 || sscanf(argv[1], "%hu", &listenPort) != 1) {
        fprintf(stderr, "! Error : handling arguments\n");
        return -1;
    } else if (verbose) {
        fprintf(stderr, "= Successfully handle arguments\n");
    }

    // TCP socket -> bind -> listen
    int serverSocket;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "! Error : creating socket\n");
        return -1;
    } else if (verbose) {
        fprintf(stderr, "= Successfully create socket\n");
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(listenPort);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "! Error : binding server socket and server\n");
        return -1;
    } else if (verbose) {
        fprintf(stderr, "= Successfully bind server socket and server\n");
    }

    if (listen(serverSocket, 20) < 0) {
        fprintf(stderr, "! Error : listening\n");
        return -1;
    } else if (verbose) {
        fprintf(stderr, "= Successfully listen\n");
    }

    char buf[MAX_BUFLEN + 1];
    int buflen, maxfd, numClients, clientfds[MAX_CLIENTS] = {}, tmpfd;
    fd_set fds;

    struct sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);
    while (true) {
        // Init
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);
        maxfd = serverSocket;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientfds[i] != 0) {
                FD_SET(clientfds[i], &fds);
                maxfd = std::max(maxfd, clientfds[i]);
            }
        }
        
        // Select
        int activity = select(maxfd + 1, &fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            fprintf(stderr, "! Error : selecting\n");
            return -1;
        } else if (verbose) {
            fprintf(stderr, "= Successfully select\n");
        }

        // New Connection
        if (FD_ISSET(serverSocket, &fds)) {
            if ((tmpfd = accept(serverSocket, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen)) < 0) {
                fprintf(stderr, "! Error : accepting\n");
                return -1;
            } else if (verbose) {
                fprintf(stderr, "= Successfully accept\n");
            }
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientfds[i] == 0) {
                    clientfds[i] = tmpfd;
                    break;
                }
            }
            if (verbose) {
                fprintf(stderr, "> New Connection -> FD : %d; IP : %s; PORT : %d\n", \
                        tmpfd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));   
            }
        }

        // Perform Operations
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if ((tmpfd = clientfds[i]) != 0 && FD_ISSET(tmpfd, &fds)) {
                buflen = recv(tmpfd, buf, MAX_BUFLEN, 0);
                if (buflen < 0) {
                    fprintf(stderr, "! Error : Recving\n");
                } else if (buflen == 0) {
                    if (verbose) {
                        getpeername(tmpfd, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen);  
                        fprintf(stderr, "@ Connection Lost -> IP : %s; PORT : %d\n", \
                                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                    }
                    close(tmpfd);
                    clientfds[i] = 0;
                } else {
                    if (stress && rand() % 10 < 2) {
                        if (verbose) {
                            fprintf(stderr, "\% Server fake dead for 5000us\n");
                        }
                        usleep(500 * 1000);
                    }
                    buf[buflen] = '\0';
                    send(tmpfd, buf, strlen(buf), 0);
                    if (verbose) {
                        getpeername(tmpfd, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen);  
                        fprintf(stderr, "* Echo Message -> IP : %s; PORT : %d\n",\
                                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                        fprintf(stderr, "\t\tMSG: \'%s\'\n", buf);
                    }
                }
            }
        }
    }
    return 0;
}

