#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER "127.0.0.1"
#define PORT 8888

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER, &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string name;
    if (argc > 1) {
        name = argv[1];
    } else {
        std::cout << "Enter your name: ";
        std::getline(std::cin, name);
    }
    if (name.empty()) name = "Anonymous";

    // Отправляем имя (добавляем '\n' для удобства сервера)
    send(sock, (name + "\n").c_str(), name.size() + 1, 0);
    std::cout << "Connected as " << name << ". Type 'exit' to quit.\n";

    // Цикл отправки сообщений
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        send(sock, (input + "\n").c_str(), input.size() + 1, 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}