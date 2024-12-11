// Copyright 2024 <Evosi>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <iostream>
#include <queue>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 5

std::queue<int> client_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void* worker_thread(void* arg) {
    while (true) {
        int client_socket;

        pthread_mutex_lock(&queue_mutex);
        while (client_queue.empty()) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        client_socket = client_queue.front();
        client_queue.pop();
        pthread_mutex_unlock(&queue_mutex);

        char buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        ssize_t recv_len = recvfrom(client_socket, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("Error receiving data");
            continue;
        }

        buffer[recv_len] = '\0';
        printf("Received path: %s\n", buffer);

        // Проверяем существование директории
        if (access(buffer, F_OK) != 0) {
            snprintf(buffer, BUFFER_SIZE, "Error: Directory does not exist");
            sendto(client_socket, buffer, strlen(buffer), 0,
                   (struct sockaddr*)&client_addr, addr_len);
            fprintf(stderr, "Directory does not exist: %s\n", buffer);
            continue;
        }

        // Проверяем, является ли путь директорией
        struct stat path_stat;
        if (stat(buffer, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
            snprintf(buffer, BUFFER_SIZE, "Error: Path is not a directory");
            sendto(client_socket, buffer, strlen(buffer), 0,
                   (struct sockaddr*)&client_addr, addr_len);
            fprintf(stderr, "Path is not a directory: %s\n", buffer);
            continue;
        }

        // Получаем информацию о файловой системе
        struct statvfs stat;
        if (statvfs(buffer, &stat) != 0) {
            snprintf(buffer, BUFFER_SIZE, "Error: Unable to retrieve file system information");
        } else {
            int64_t free_space = static_cast<int64_t>(stat.f_bfree) * stat.f_bsize;
            int64_t total_space = static_cast<int64_t>(stat.f_blocks) * stat.f_bsize;
            int64_t used_space = total_space - free_space;
            snprintf(buffer, BUFFER_SIZE, "Free: %ld, Used: %ld", free_space, used_space);
        }

        // Отправляем ответ клиенту
        sendto(client_socket, buffer, strlen(buffer), 0,
            (struct sockaddr*)&client_addr, addr_len);
        printf("Sent response: %s\n", buffer);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    int server_socket;
    struct sockaddr_in server_addr;

    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind");
        close(server_socket);
        exit(1);
    }

    printf("Server is running on port %d\n", port);

    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    while (true) {
        int client_socket = server_socket;
        pthread_mutex_lock(&queue_mutex);
        client_queue.push(client_socket);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    close(server_socket);
    return 0;
}