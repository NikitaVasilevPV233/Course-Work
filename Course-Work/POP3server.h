#pragma once
#include <winsock2.h>
#include <atomic>

class POP3Server {
public:
    explicit POP3Server(int port);
    ~POP3Server();
    void start();
    void stop();

private:
    void run();
    static void clientThread(SOCKET clientSocket);

    int port;
    std::atomic<bool> isRunning;
    SOCKET listenSocket;
};