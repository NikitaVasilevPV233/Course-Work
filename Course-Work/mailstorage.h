#pragma once
#include "SMTPMessage.h"
#include <string>

class MailStorage {
public:
    static void save(const SMTPMessage& msg);
};