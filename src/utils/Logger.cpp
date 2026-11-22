#include "utils/Logger.hpp"

#include <ctime>
#include <iostream>

namespace
{
    std::string timestamp()
    {
        std::time_t now = std::time(NULL);
        std::tm* local = std::localtime(&now);
        char buffer[32];
        if (local == NULL || std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0)
            return "unknown time";
        return buffer;
    }

    void writeLine(std::ostream& stream, const char* level, const std::string& message)
    {
        stream << "[" << timestamp() << "] " << level << " " << message << std::endl;
    }
}

namespace Logger
{
    void info(const std::string& message)
    {
        writeLine(std::cout, "INFO ", message);
    }

    void warn(const std::string& message)
    {
        writeLine(std::cerr, "WARN ", message);
    }

    void error(const std::string& message)
    {
        writeLine(std::cerr, "ERROR", message);
    }
}
