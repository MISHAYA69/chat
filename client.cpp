#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define DEFAULT_SERVER "127.0.0.1"

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::string name;
    std::string server_ip = DEFAULT_SERVER;

    // Разбор аргументов командной строки
    if (argc > 1) {
        name = argv[1];                     // первый аргумент – имя
        if (argc > 2) {
            server_ip = argv[2];             // второй аргумент – IP сервера
        }
    } else {
        std::cout << "Enter your name: ";
        std::getline(std::cin, name);
        std::cout << "Enter server IP (default " << DEFAULT_SERVER << "): ";
        std::string input_ip;
        std::getline(std::cin, input_ip);
        if (!input_ip.empty()) {
            server_ip = input_ip;
        }
    }
    if (name.empty()) name = "Anonymous";

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip.c_str(), &server.sin_addr) != 1) {
        std::cerr << "Invalid server IP address" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Connection failed to " << server_ip << ":" << PORT << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Отправляем имя серверу
    send(sock, (name + "\n").c_str(), name.size() + 1, 0);
    std::cout << "Connected as " << name << " to server " << server_ip << ". Type 'exit' to quit.\n";

    // Цикл отправки сообщений
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        send(sock, (input + "\n").c_str(), input.size() + 1, 0);

        if (input == "exit") {
            std::cout << "Disconnecting...\n";
            break;
        }
    }

    Sleep(100); // небольшая задержка перед закрытием сокета
    closesocket(sock);
    WSACleanup();
    return 0;
}