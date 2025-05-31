#include "POP3Server.h"   
#include "SMTPServer.h"   
#include <thread>
#include <windows.h>      
#include <iostream>

int main() {
    POP3Server pop3(110);
    SMTPServer smtp(25);

    std::thread pop3Thread([&]() {
        pop3.start();
        });

    Sleep(100);
    smtp.start();
    pop3Thread.join();
    return 0;
}