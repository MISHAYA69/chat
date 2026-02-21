#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <conio.h>      // _kbhit, _getch
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define BUFFER_SIZE 256

struct ClientInfo {
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
SOCKET listen_sock = INVALID_SOCKET;

// ---------- Прототипы ----------
bool initWinsock();
bool createListeningSocket();
void printClientList();
void acceptNewClient();
void removeClient(SOCKET sock);
void cleanup();
void printServerIPs(); // новая функция

// ---------- Определения функций ----------

bool initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
    return true;
}

bool createListeningSocket() {
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listen_sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_sock);
        return false;
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_sock);
        return false;
    }

    return true;
}

void printServerIPs() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host) {
            std::cout << "Server IP addresses:\n";
            for (int i = 0; host->h_addr_list[i] != nullptr; ++i) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(addr));
                std::cout << "  " << inet_ntoa(addr) << "\n";
            }
        }
    } else {
        std::cout << "Could not determine server IPs.\n";
    }
}

void printClientList() {
    std::cout << "\n--- Connected clients (" << clients.size() << ") ---\n";
    if (clients.empty()) {
        std::cout << "(none)\n";
    } else {
        for (size_t i = 0; i < clients.size(); ++i) {
            std::cout << i + 1 << ". " << clients[i].name
                      << " (socket " << clients[i].socket << ")\n";
        }
    }
    std::cout << "--------------------------------\n";
    std::cout << "> ";
    fflush(stdout);
}

void acceptNewClient() {
    sockaddr_in clientAddr{};
    int addrLen = sizeof(clientAddr);
    SOCKET client_sock = accept(listen_sock, (sockaddr*)&clientAddr, &addrLen);
    if (client_sock == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        std::cerr << "Failed to receive client name" << std::endl;
        closesocket(client_sock);
        return;
    }
    buffer[bytes] = '\0';
    std::string name(buffer);
    name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
    name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());

    if (name.empty()) name = "Anonymous";

    clients.push_back({client_sock, name});
    std::cout << "\n[+] Client connected: " << name << " (socket " << client_sock << ")\n";
    printClientList();
}

void removeClient(SOCKET sock) {
    auto it = std::find_if(clients.begin(), clients.end(),
                           [sock](const ClientInfo& ci) { return ci.socket == sock; });
    if (it != clients.end()) {
        std::cout << "\n[-] Client disconnected: " << it->name << " (socket " << sock << ")\n";
        closesocket(it->socket);
        clients.erase(it);
        printClientList();
    }
}

void cleanup() {
    std::cout << "\nCleaning up resources...\n";
    for (auto& client : clients) {
        closesocket(client.socket);
    }
    clients.clear();
    if (listen_sock != INVALID_SOCKET) {
        closesocket(listen_sock);
        listen_sock = INVALID_SOCKET;
    }
    WSACleanup();
}

// ---------- Главная функция ----------
int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "=== TCP Client Manager ===\n";
    
    if (!initWinsock()) return 1;
    if (!createListeningSocket()) {
        WSACleanup();
        return 1;
    }

    printServerIPs(); // выводим IP сервера
    std::cout << "Listening on port " << PORT << "\n";
    std::cout << "Commands: 'quit' - exit server, 'list' - show clients\n\n";

    std::string cmd_buffer;
    bool running = true;

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);
        SOCKET max_sock = listen_sock;

        for (const auto& client : clients) {
            FD_SET(client.socket, &readfds);
            if (client.socket > max_sock) max_sock = client.socket;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 мс

        int activity = select(0, &readfds, nullptr, nullptr, &tv);
        if (activity == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEINTR) {
                std::cerr << "select error: " << error << std::endl;
                break;
            }
        }

        // 1. Новое подключение
        if (activity > 0 && FD_ISSET(listen_sock, &readfds)) {
            acceptNewClient();
        }

        // 2. Данные от клиентов
        for (int i = clients.size() - 1; i >= 0; --i) {
            SOCKET sock = clients[i].socket;
            if (FD_ISSET(sock, &readfds)) {
                char buffer[BUFFER_SIZE];
                int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                if (bytes <= 0) {
                    removeClient(sock);
                    continue;
                } else {
                    buffer[bytes] = '\0';
                    std::string msg(buffer);
                    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
                    msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());

                    if (!msg.empty()) {
                        if (msg == "exit") {
                            std::cout << "\nClient " << clients[i].name << " sent exit command\n";
                            removeClient(sock);
                            continue;
                        } else {
                            std::cout << "\n[" << clients[i].name << "]: " << msg << "\n> ";
                            fflush(stdout);
                        }
                    }
                }
            }
        }

        // 3. Ввод команд с клавиатуры
        while (_kbhit()) {
            char ch = _getch();
            if (ch == '\r' || ch == '\n') {
                if (!cmd_buffer.empty()) {
                    if (cmd_buffer == "quit") {
                        std::cout << "\nShutting down server...\n";
                        running = false;
                        break;
                    } else if (cmd_buffer == "list") {
                        printClientList();
                    } else {
                        std::cout << "\nUnknown command: " << cmd_buffer << "\n> ";
                        fflush(stdout);
                    }
                    cmd_buffer.clear();
                }
            } else if (ch == '\b' || ch == 127) {
                if (!cmd_buffer.empty()) {
                    cmd_buffer.pop_back();
                    std::cout << "\b \b";
                    fflush(stdout);
                }
            } else {
                cmd_buffer.push_back(ch);
                std::cout << ch;
                fflush(stdout);
            }
        }
    }

    cleanup();
    return 0;
}