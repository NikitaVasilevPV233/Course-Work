#pragma once
#include <string>

class POP3Parser {
public:
    void parse(const std::string& line);
    std::string getCommand() const;
    std::string getArgument() const;

private:
    std::string command;
    std::string argument;
};