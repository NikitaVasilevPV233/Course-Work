﻿#pragma once

#include <winsock2.h>
#include <string>
#include <vector>

class SMTPServer {
public:
    SMTPServer(int port);
    void start();

private:
    int port;
    SOCKET listenSocket;
    bool checkCredentials(const std::string& username, const std::string& password);
    std::string decodeBase64(const std::string& encoded);
    void sendResponse(SOCKET clientSocket, const std::string& resp);
    void handleClient(SOCKET clientSocket);
};