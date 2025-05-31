#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "POP3Parser.h"

class POP3Connection {
public:
    explicit POP3Connection(SOCKET clientSocket);
    void handleClient();

private:
    enum class State { AUTHORIZATION, TRANSACTION, UPDATE };
    SOCKET clientSocket;
    POP3Parser parser;
    State state;
    std::string username;
    bool authenticated;

    std::vector<std::string> messages;
    std::vector<bool> toDelete;

    void sendResponse(const std::string& resp);
    std::string recvLine();
    void loadMessages();
    void handleCommand(const std::string& cmd, const std::string& arg);
};