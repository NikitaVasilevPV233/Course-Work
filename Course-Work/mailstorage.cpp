#include "MailStorage.h"
#include <filesystem>
#include <fstream>
#include <chrono>

void MailStorage::save(const SMTPMessage& msg) {
    std::filesystem::create_directory("mailbox");
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::string filename = "mailbox/msg_" + std::to_string(timestamp) + ".eml";
    static int counter = 0;
    std::string filename = "mailbox/msg_" + std::to_string(counter++) + ".eml";
    std::ofstream ofs(filename, std::ios::out);
    ofs << "From: " << msg.from << "\r\n";
    for (const auto& rcpt : msg.to) {
        ofs << "To: " << rcpt << "\r\n";
    }
    ofs << "\r\n";
    ofs << msg.data;
    ofs.close();
}