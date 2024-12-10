// Copyright 2024 <Evosi>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>  // Для printf
#include <cstdlib>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"  // IP-адрес сервера
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char path[BUFFER_SIZE];
    int serverAddrLen = sizeof(serverAddr);

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Создание UDP-сокета
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        printf("Invalid address or address not supported.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    while (true) {
        // Запрос пути у пользователя
        printf("Enter the path to check (or 'exit' to quit): ");
        fgets(path, BUFFER_SIZE, stdin);
        path[strcspn(path, "\n")] = '\0';  // Удаляем символ новой строки

        // Если введено 'exit', завершаем работу
        if (strcmp(path, "exit") == 0) {
            break;
        }

        // Отправка пути на сервер
        if (sendto(clientSocket, path, strlen(path), 0,
            (struct sockaddr *)&serverAddr, serverAddrLen) == SOCKET_ERROR) {
            printf("Failed to send data. Error Code: %d\n",
            WSAGetLastError());
            continue;
        }

        // Получение ответа от сервера
        int recvLen = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&serverAddr, &serverAddrLen);
        if (recvLen == SOCKET_ERROR) {
            printf("Failed to receive data. Error Code: %d\n",
            WSAGetLastError());
            continue;
        }

        // Завершаем строку нулевым символом и выводим ответ
        buffer[recvLen] = '\0';
        printf("Server response: %s\n", buffer);
    }

    // Закрытие сокета и завершение работы
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
