// Copyright 2024 <Evosi>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Invalid port number. Please specify a port between 1 and 65535.\n");
        exit(1);
    }

    int serverSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    socklen_t addrLen = sizeof(serverAddr);

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        exit(1);
    }

    printf("Server running at %s:%d\n", server_ip, port);

    while (true) {
        int recvLen = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&serverAddr, &addrLen);
        if (recvLen == -1) {
            perror("Failed to receive data");
            continue;
        }

        buffer[recvLen] = '\0';
        printf("Received request for path: %s\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Exiting.");
            sendto(serverSocket, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&serverAddr, addrLen);
            break;
        }

        if (access(buffer, F_OK) == -1) {
            snprintf(buffer, BUFFER_SIZE, "Error: Path does not exist");
        } else {
            struct statvfs stat;
            if (statvfs(buffer, &stat) == -1) {
                snprintf(buffer, BUFFER_SIZE, "Error: Failed to retrieve disk status");
            } else {
                snprintf(buffer, BUFFER_SIZE, "Server response: Free: %lu, Used: %lu", 
                         stat.f_bfree * stat.f_frsize, (stat.f_blocks - stat.f_bfree) * stat.f_frsize);
            }
        }

        sendto(serverSocket, buffer, strlen(buffer), 0,
               (struct sockaddr *)&serverAddr, addrLen);
    }

    close(serverSocket);
}
