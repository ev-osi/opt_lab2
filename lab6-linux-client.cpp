// Copyright 2024 <Evosi>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip> [port]\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = DEFAULT_PORT;

    if (argc > 2) {
        port = atoi(argv[2]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }

    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    socklen_t serverAddrLen = sizeof(serverAddr);

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serverAddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address or address not supported.\n");
        close(clientSocket);
        return 1;
    }

    // Output connection message
    printf("Connecting to server at %s:%d\n", server_ip, port);

    while (true) {
        printf("Enter the path to check (or 'exit' to quit): ");
        fgets(path, BUFFER_SIZE, stdin);
        path[strcspn(path, "\n")] = '\0';

        if (strcmp(path, "exit") == 0) {
            break;
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
