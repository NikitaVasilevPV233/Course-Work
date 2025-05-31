#include "POP3Parser.h"
#include <sstream>

void POP3Parser::parse(const std::string& line) {
    std::istringstream iss(line);
    iss >> command;
    std::getline(iss, argument);
    if (!argument.empty() && argument[0] == ' ') {
        argument.erase(0, 1);
    }
}

std::string POP3Parser::getCommand() const {
    return command;
}

std::string POP3Parser::getArgument() const {
    return argument;
}