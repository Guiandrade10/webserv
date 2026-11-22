#include "config/ConfigParser.hpp"
#include "config/ServerConfig.hpp"
#include "core/EventLoop.hpp"
#include "core/Signals.hpp"
#include "utils/Constants.hpp"
#include "utils/Logger.hpp"

#include <exception>
#include <new>
#include <iostream>
#include <vector>

namespace
{
    void logConfiguration(const std::string& path, const std::vector<ServerConfig>& servers)
    {
        Logger::info("configuration loaded from " + path);
        for (size_t i = 0; i < servers.size(); ++i)
        {
            Logger::info(servers[i].describe());
            const std::vector<LocationConfig>& locations = servers[i].locations();
            for (size_t j = 0; j < locations.size(); ++j)
                Logger::info("    " + locations[j].describe());
        }
    }
}

int main(int argc, char** argv)
{
    if (argc > 2)
    {
        std::cerr << "usage: ./webserv [configuration file]" << std::endl;
        return 1;
    }
    std::string configPath = (argc == 2) ? argv[1] : kDefaultConfigPath;
    try
    {
        ConfigParser parser;
        std::vector<ServerConfig> servers = parser.parseFile(configPath);
        logConfiguration(configPath, servers);
        Signals::install();
        EventLoop loop(servers);
        loop.run();
    }
    catch (const std::bad_alloc&)
    {
        std::cerr << "webserv: out of memory" << std::endl;
        return 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "webserv: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
