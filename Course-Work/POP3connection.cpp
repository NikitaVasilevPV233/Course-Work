#include "POP3Connection.h"
#include <filesystem>
#include <iostream>
#include <fstream>

POP3Connection::POP3Connection(SOCKET clientSocket)
    : clientSocket(clientSocket), state(State::AUTHORIZATION), authenticated(false) {
}

void POP3Connection::handleClient() {
    sendResponse("+OK POP3 Server Ready\r\n");
    std::string line;

    while (state != State::UPDATE) {
        line = recvLine();
        if (line.empty()) break;
        parser.parse(line);
        std::string cmd = parser.getCommand();
        std::string arg = parser.getArgument();
        handleCommand(cmd, arg);
        if (state == State::TRANSACTION) {
            loadMessages();
        }
    }

    if (state == State::UPDATE) {
        for (size_t i = 0; i < messages.size(); ++i) {
            if (toDelete[i]) {
                std::filesystem::remove(messages[i]);
            }
        }
    }
}

void POP3Connection::sendResponse(const std::string& resp) {
    send(clientSocket, resp.c_str(), static_cast<int>(resp.size()), 0);
}

std::string POP3Connection::recvLine() {
    char buf[1024];
    int len = recv(clientSocket, buf, sizeof(buf) - 1, 0);
    if (len <= 0) return {};
    buf[len] = '\0';
    std::string line(buf);
    if (!line.empty() && line.back() == '\n') line.pop_back();
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

void POP3Connection::loadMessages() {
    messages.clear();
    toDelete.clear();
    const std::string dir = "mailbox";
    if (!std::filesystem::exists(dir)) return;
    for (auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            messages.push_back(entry.path().string());
        }
    }

    std::sort(messages.begin(), messages.end());
    toDelete.resize(messages.size(), false);
}

void POP3Connection::handleCommand(const std::string& cmd, const std::string& arg) {
    if (state == State::AUTHORIZATION) {
        if (cmd == "USER") {
            username = arg;
            sendResponse("+OK User accepted\r\n");
        }
        else if (cmd == "PASS") {
            if (arg == "1234" && !username.empty()) {
                authenticated = true;
                state = State::TRANSACTION;
                loadMessages();
                sendResponse("+OK Authentication successful\r\n");
            }
            else {
                sendResponse("-ERR Invalid credentials\r\n");
            }
        }
        else {
            sendResponse("-ERR Command not allowed in AUTHORIZATION state\r\n");
        }
    }
    else if (state == State::TRANSACTION) {
        if (cmd == "STAT") {
            size_t count = messages.size();
            uintmax_t totalSize = 0;
            for (auto& file : messages) totalSize += std::filesystem::file_size(file);
            sendResponse("+OK " + std::to_string(count) + " " + std::to_string(totalSize) + "\r\n");
        }
        else if (cmd == "LIST") {
            sendResponse("+OK Listing follows\r\n");
            for (size_t i = 0; i < messages.size(); ++i) {
                auto size = std::filesystem::file_size(messages[i]);
                sendResponse(std::to_string(i + 1) + " " + std::to_string(size) + "\r\n");
            }
            sendResponse(".\r\n");
        }
        else if (cmd == "RETR") {
            try {
                int idx = std::stoi(arg);
                if (idx < 1 || idx > static_cast<int>(messages.size())) {
                    sendResponse("-ERR No such message\r\n");
                }
                else {
                    std::ifstream ifs(messages[idx - 1], std::ios::in);
                    sendResponse("+OK " + std::to_string(std::filesystem::file_size(messages[idx - 1])) + " octets\r\n");
                    std::string line;
                    while (std::getline(ifs, line)) {
                        if (!line.empty() && line[0] == '.') line = "." + line;
                        sendResponse(line + "\r\n");
                    }
                    sendResponse(".\r\n");
                    ifs.close();
                }
            }
            catch (...) {
                sendResponse("-ERR Invalid message number\r\n");
            }
        }
        else if (cmd == "DELE") {
            try {
                int idx = std::stoi(arg);
                if (idx < 1 || idx > static_cast<int>(messages.size())) {
                    sendResponse("-ERR No such message\r\n");
                }
                else {
                    toDelete[idx - 1] = true;
                    sendResponse("+OK Message marked for deletion\r\n");
                }
            }
            catch (...) {
                sendResponse("-ERR Invalid message number\r\n");
            }
        }
        else if (cmd == "NOOP") {
            sendResponse("+OK\r\n");
        }
        else if (cmd == "QUIT") {
            sendResponse("+OK Goodbye\r\n");
            state = State::UPDATE;
        }
        else if (cmd == "RSET") {
            std::fill(toDelete.begin(), toDelete.end(), false);
            sendResponse("+OK Deletion marks cleared\r\n");
        }
        else {
            sendResponse("-ERR Unknown command\r\n");
        }
    }
}