// Copyright 2024 <Evosi>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
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

    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    socklen_t serverAddrLen = sizeof(serverAddr);

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serverAddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address or address not supported.\n");
        close(clientSocket);
        exit(1);
    }

    // Output connection message
    printf("Connecting to server at %s:%d\n", server_ip, port);

    while (true) {
        printf("Enter the path to check (or 'exit' to quit): ");
        fgets(path, BUFFER_SIZE, stdin);
        path[strcspn(path, "\n")] = '\0';

        if (strcmp(path, "exit") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Exiting.");
            sendto(clientSocket, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&serverAddr, serverAddrLen);
            break;
        }

        if (access(path, F_OK) == -1) {
            perror("Path does not exist");
            snprintf(buffer, BUFFER_SIZE, "Error: Path does not exist");
            sendto(clientSocket, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&serverAddr, serverAddrLen);
            continue;
        }

        if (sendto(clientSocket, path, strlen(path), 0,
                   (struct sockaddr *)&serverAddr, serverAddrLen) == -1) {
            perror("Failed to send data");
            continue;
        }

        int recvLen = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&serverAddr, &serverAddrLen);
        if (recvLen == -1) {
            perror("Failed to receive data");
            continue;
        }

        buffer[recvLen] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(clientSocket);
    return 0;
}
