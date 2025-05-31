#include "POP3Server.h"
#include "POP3Connection.h"
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

POP3Server::POP3Server(int port) : port(port), isRunning(false), listenSocket(INVALID_SOCKET) {}

POP3Server::~POP3Server() {
    stop();
    WSACleanup();
}

void POP3Server::start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed in POP3Server" << std::endl;
        return;
    }
    isRunning = true;
    run();
}

void POP3Server::stop() {
    isRunning = false;
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }
}

void POP3Server::run() {
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "POP3: Socket creation failed: " << WSAGetLastError() << std::endl;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "POP3: Bind failed: " << WSAGetLastError() << std::endl;
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "POP3: Listen failed: " << WSAGetLastError() << std::endl;
        return;
    }

    std::cout << "POP3 Server listening on port " << port << std::endl;

    while (isRunning) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;
        std::thread(clientThread, clientSocket).detach();
    }
}

void POP3Server::clientThread(SOCKET clientSocket) {
    POP3Connection conn(clientSocket);
    conn.handleClient();
    closesocket(clientSocket);
}