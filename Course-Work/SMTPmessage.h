#pragma once
#include <string>
#include <vector>

struct SMTPMessage {
    std::string from;
    std::vector<std::string> to;
    std::string data;
};