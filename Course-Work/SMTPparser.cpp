#include "SMTPParser.h"
#include <sstream>

void SMTPParser::parse(const std::string& line) {
    std::istringstream iss(line);
    iss >> command;
    std::getline(iss, arguments);
    if (!arguments.empty() && arguments[0] == ' ') {
        arguments.erase(0, 1);
    }
}

std::string SMTPParser::getCommand() const {
    return command;
}

std::string SMTPParser::getArguments() const {
    return arguments;
}