#pragma once
#include <string>

class SMTPParser {
public:
    void parse(const std::string& line);
    std::string getCommand() const;
    std::string getArguments() const;
private:
    std::string command;
    std::string arguments;
};