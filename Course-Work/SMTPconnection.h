#pragma once
#include <winsock2.h>
#include <string>
#include "SMTPParser.h"
#include "SMTPMessage.h"

class SMTPConnection {
public:
    explicit SMTPConnection(SOCKET clientSocket);
    void handleClient();
private:
    SOCKET clientSocket;
    SMTPParser parser;
    SMTPMessage message;
    void sendResponse(const std::string& resp);
    std::string recvLine();
};
