#include "SMTPServer.h"
#include <ws2tcpip.h>
#include <windows.h>
#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#pragma comment(lib, "ws2_32.lib")

SMTPServer::SMTPServer(int port) : port(port), listenSocket(INVALID_SOCKET) 
{
}

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool isBase64(unsigned char c) {
    return (std::isalnum(c) || (c == '+') || (c == '/'));
}

std::string SMTPServer::decodeBase64(const std::string& encoded) {
    int in_len = static_cast<int>(encoded.size());
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (isBase64(encoded[in_]) || encoded[in_] == '=')) {
        char_array_4[i++] = static_cast<unsigned char>(encoded[in_]); in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = static_cast<unsigned char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
            char_array_3[1] = static_cast<unsigned char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
            char_array_3[2] = static_cast<unsigned char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (int j2 = i; j2 < 4; j2++)
            char_array_4[j2] = 0;
        for (int j2 = 0; j2 < 4; j2++)
            char_array_4[j2] = static_cast<unsigned char>(base64_chars.find(char_array_4[j2]));

        char_array_3[0] = static_cast<unsigned char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
        char_array_3[1] = static_cast<unsigned char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
        char_array_3[2] = static_cast<unsigned char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

        for (int j2 = 0; j2 < i - 1; j2++)
            ret += char_array_3[j2];
    }

    return ret;
}

bool SMTPServer::checkCredentials(const std::string& username, const std::string& password) {
    return (username == "test" && password == "1234");
}

void SMTPServer::sendResponse(SOCKET clientSocket, const std::string& resp) {
    std::string out = resp;
    if (out.size() < 2 || (out[out.size() - 2] != '\r' || out[out.size() - 1] != '\n')) {
        out += "\r\n";
    }
    send(clientSocket, out.c_str(), static_cast<int>(out.size()), 0);
}

void SMTPServer::handleClient(SOCKET clientSocket) {
    sendResponse(clientSocket, "220 Welcome to Simple SMTP Server");

    std::string state;
    std::string currentUser;
    std::string currentPass;
    std::string mailFrom;
    std::vector<std::string> recipients;
    std::string dataBuffer;
    bool receivingData = false;

    char buffer[1024];
    std::string inputBuffer;

    while (true) {
        int len = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;
        buffer[len] = '\0';
        inputBuffer += buffer;

        size_t pos;
        while ((pos = inputBuffer.find("\r\n")) != std::string::npos) {
            std::string line = inputBuffer.substr(0, pos);
            inputBuffer.erase(0, pos + 2);

            std::cout << "[CLIENT] " << line << std::endl;

            if (receivingData) {
                if (line == ".") {
                    receivingData = false;
                    std::cout << "=== Incoming message ===\n";
                    std::cout << "From: " << mailFrom << "\n";
                    for (auto& r : recipients) std::cout << "To: " << r << "\n";
                    std::cout << dataBuffer << "\n========================\n";

                    sendResponse(clientSocket, "250 Message accepted for delivery");
                    mailFrom.clear();
                    recipients.clear();
                    dataBuffer.clear();
                    state = "AUTH_OK";
                }
                else {
                    dataBuffer += line + "\r\n";
                }
                continue;
            }

            if (line.rfind("EHLO", 0) == 0) {
                sendResponse(clientSocket, "250-localhost");
                sendResponse(clientSocket, "250-AUTH LOGIN PLAIN");
                continue;
            }

            else if (line.rfind("HELO", 0) == 0) {
                sendResponse(clientSocket, "250 Hello");
                continue;
            }

            else if (line.rfind("AUTH PLAIN", 0) == 0) {
                size_t pos = line.find(' ');
                pos = line.find(' ', pos + 1);
                if (pos == std::string::npos) {
                    sendResponse(clientSocket, "334");
                    state = "AUTH_PLAIN_NEXT";
                    continue;
                }
                else {
                    std::string b64 = line.substr(pos + 1);
                    std::string decoded = decodeBase64(b64);
                    size_t firstNull = decoded.find('\0');
                    size_t secondNull = decoded.find('\0', firstNull + 1);
                    std::string user = decoded.substr(firstNull + 1, secondNull - firstNull - 1);
                    std::string pass = decoded.substr(secondNull + 1);
                    if (checkCredentials(user, pass)) {
                        sendResponse(clientSocket, "235 Authentication successful");
                        state = "AUTH_OK";
                    }
                    else {
                        sendResponse(clientSocket, "535 Authentication failed");
                        break;
                    }
                    continue;
                }
            }

            else if (state == "AUTH_PLAIN_NEXT") {
                std::string decoded = decodeBase64(line);
                size_t firstNull = decoded.find('\0');
                size_t secondNull = decoded.find('\0', firstNull + 1);
                std::string user = decoded.substr(firstNull + 1, secondNull - firstNull - 1);
                std::string pass = decoded.substr(secondNull + 1);
                if (checkCredentials(user, pass)) {
                    sendResponse(clientSocket, "235 Authentication successful");
                    state = "AUTH_OK";
                }
                else {
                    sendResponse(clientSocket, "535 Authentication failed");
                    break;
                }
                continue;
            }

            else if (line.rfind("AUTH LOGIN", 0) == 0) {
                sendResponse(clientSocket, "334 VXNlcm5hbWU6");
                state = "AUTH_USER";
                continue;
            }

            else if (state == "AUTH_USER") {
                currentUser = decodeBase64(line);
                sendResponse(clientSocket, "334 UGFzc3dvcmQ6");
                state = "AUTH_PASS";
                continue;
            }

            else if (state == "AUTH_PASS") {
                currentPass = decodeBase64(line);
                if (checkCredentials(currentUser, currentPass)) {
                    sendResponse(clientSocket, "235 Authentication successful");
                    state = "AUTH_OK";
                }
                else {
                    sendResponse(clientSocket, "535 Authentication failed");
                    break;
                }
                continue;
            }

            else if (line.rfind("MAIL FROM:", 0) == 0) {
                if (state != "AUTH_OK") {
                    sendResponse(clientSocket, "530 Authentication required");
                }
                else {
                    mailFrom = line.substr(10);
                    sendResponse(clientSocket, "250 OK");
                    state = "MAIL";
                }
                continue;
            }

            else if (line.rfind("RCPT TO:", 0) == 0) {
                if (state != "MAIL" && state != "RCPT") {
                    sendResponse(clientSocket, "503 Bad sequence of commands");
                }
                else {
                    std::string rcpt = line.substr(8);
                    recipients.push_back(rcpt);
                    sendResponse(clientSocket, "250 OK");
                    state = "RCPT";
                }
                continue;
            }

            else if (line == "DATA") {
                if (state != "RCPT") {
                    sendResponse(clientSocket, "503 Bad sequence of commands");
                }
                else {
                    sendResponse(clientSocket, "354 End data with <CR><LF>.<CR><LF>");
                    receivingData = true;
                    state = "DATA";
                }
                continue;
            }

            else if (line == "QUIT") {
                sendResponse(clientSocket, "221 Bye");
                closesocket(clientSocket);
                return;
            }

            else {
                sendResponse(clientSocket, "500 Command unrecognized");
                continue;
            }
        }
    }

    closesocket(clientSocket);
}


void SMTPServer::start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_port = htons(port);
    if (InetPtonA(AF_INET, "127.0.0.1", &service.sin_addr) != 1) {
        std::cerr << "InetPtonA failed\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (bind(listenSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "SMTP Server listening on port " << port << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;
        std::thread(&SMTPServer::handleClient, this, clientSocket).detach();
    }

    closesocket(listenSocket);
    WSACleanup();
}