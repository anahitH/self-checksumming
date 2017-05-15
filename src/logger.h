#pragma once

#include <string>

namespace selfchecksum {

class logger
{
public:
    logger() = default;
    logger(const logger& logger) = delete;
    logger& operator= (const logger& logger) = delete;

public:
    void log_message(const std::string& msg) const;
    void log_error(const std::string& error) const;
};

}

