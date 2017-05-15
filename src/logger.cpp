#include "logger.h"

#include <iostream>

namespace selfchecksum {

void logger::log_message(const std::string& msg) const
{
    std::cout << msg << std::endl;
}

void logger::log_error(const std::string& error) const
{
    std::cerr << error << std::endl;
}

}

