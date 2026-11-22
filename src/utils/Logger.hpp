#ifndef WEBSERV_UTILS_LOGGER_HPP
#define WEBSERV_UTILS_LOGGER_HPP

#include <string>

namespace Logger
{
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
}

#endif
