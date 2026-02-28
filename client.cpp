#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <locale>
#include <codecvt>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define DEFAULT_SERVER "127.0.0.1"

// Функция для конвертации UTF-8 в CP866
std::string utf8_to_cp866(const std::string& utf8_str) {
    if (utf8_str.empty()) return "";
    
    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
    if (wlen == 0) return utf8_str;
    
    wchar_t* wstr = new wchar_t[wlen];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wstr, wlen);
    
    // UTF-16 -> CP866
    int clen = WideCharToMultiByte(866, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (clen == 0) {
        delete[] wstr;
        return utf8_str;
    }
    
    char* cstr = new char[clen];
    WideCharToMultiByte(866, 0, wstr, -1, cstr, clen, NULL, NULL);
    
    std::string result(cstr);
    delete[] wstr;
    delete[] cstr;
    
    return result;
}

// Функция для конвертации CP866 в UTF-8
std::string cp866_to_utf8(const std::string& cp866_str) {
    if (cp866_str.empty()) return "";
    
    // CP866 -> UTF-16
    int wlen = MultiByteToWideChar(866, 0, cp866_str.c_str(), -1, NULL, 0);
    if (wlen == 0) return cp866_str;
    
    wchar_t* wstr = new wchar_t[wlen];
    MultiByteToWideChar(866, 0, cp866_str.c_str(), -1, wstr, wlen);
    
    // UTF-16 -> UTF-8
    int clen = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (clen == 0) {
        delete[] wstr;
        return cp866_str;
    }
    
    char* cstr = new char[clen];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, cstr, clen, NULL, NULL);
    
    std::string result(cstr);
    delete[] wstr;
    delete[] cstr;
    
    return result;
}

int main(int argc, char* argv[]) {
    // Устанавливаем кодировку консоли на CP866 для корректного ввода/вывода
    SetConsoleOutputCP(866);
    SetConsoleCP(866);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::string name;
    std::string server_ip = DEFAULT_SERVER;

    // Разбор аргументов командной строки
    if (argc > 1) {
        // Аргументы командной строки обычно в CP866, конвертируем в UTF-8
        name = cp866_to_utf8(argv[1]);
        if (argc > 2) {
            server_ip = argv[2]; // IP адрес не конвертируем
        }
    } else {
        std::cout << utf8_to_cp866("Enter your name: ");
        std::getline(std::cin, name);
        name = cp866_to_utf8(name);
        
        std::cout << utf8_to_cp866("Enter server IP (default ") << DEFAULT_SERVER << "): ";
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

    // Отправляем имя серверу (в UTF-8)
    send(sock, (name + "\n").c_str(), name.size() + 1, 0);
    
    std::cout << utf8_to_cp866("Connected as ") << utf8_to_cp866(name) 
              << utf8_to_cp866(" to server ") << server_ip 
              << utf8_to_cp866(". Type 'exit' to quit.\n");

    // Цикл отправки сообщений
    std::string input;
    while (true) {
        std::cout << utf8_to_cp866("> ");
        std::getline(std::cin, input);
        
        // Конвертируем ввод из CP866 в UTF-8 перед отправкой
        std::string utf8_input = cp866_to_utf8(input);
        send(sock, (utf8_input + "\n").c_str(), utf8_input.size() + 1, 0);

        if (utf8_input == "exit") {
            std::cout << utf8_to_cp866("Disconnecting...\n");
            break;
        }
    }

    Sleep(100);
    closesocket(sock);
    WSACleanup();
    return 0;
}