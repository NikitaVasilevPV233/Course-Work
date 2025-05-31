#include "SMTPConnection.h"
#include "MailStorage.h"
#include <iostream>

SMTPConnection::SMTPConnection(SOCKET clientSocket) : clientSocket(clientSocket) {}

void SMTPConnection::handleClient() {
    sendResponse("220 Simple SMTP Server Ready\r\n");
    std::string line;
    while (true) {
        line = recvLine();
        if (line.empty()) break;
        parser.parse(line);
        std::string cmd = parser.getCommand();

        if (cmd == "HELO" || cmd == "EHLO") {
            sendResponse("250 Hello\r\n");
        }
        else if (cmd == "MAIL") {
            message.from = parser.getArguments();
            sendResponse("250 OK\r\n");
        }
        else if (cmd == "RCPT") {
            message.to.push_back(parser.getArguments());
            sendResponse("250 OK\r\n");
        }
        else if (cmd == "DATA") {
            sendResponse("354 End data with <CR><LF>.<CR><LF>\r\n");
            std::string data;
            while (true) {
                line = recvLine();
                if (line == ".") break;
                data += line + "\r\n";
            }
            message.data = data;
            MailStorage::save(message);
            sendResponse("250 OK: Message received\r\n");
        }
        else if (cmd == "QUIT") {
            sendResponse("221 Bye\r\n");
            break;
        }
        else {
            sendResponse("500 Unrecognized command\r\n");
        }
    }
    closesocket(clientSocket);
}

void SMTPConnection::sendResponse(const std::string& resp) {
    send(clientSocket, resp.c_str(), static_cast<int>(resp.size()), 0);
}

std::string SMTPConnection::recvLine() {
    char buf[1024];
    int len = recv(clientSocket, buf, sizeof(buf) - 1, 0);
    if (len <= 0) return {};
    buf[len] = '\0';
    std::string line(buf);
    if (!line.empty() && line.back() == '\n') line.pop_back();
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}