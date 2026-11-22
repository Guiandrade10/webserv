#ifndef WEBSERV_HANDLERS_ROUTER_HPP
#define WEBSERV_HANDLERS_ROUTER_HPP

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"

#include <string>
#include <vector>

struct Route
{
    const ServerConfig*   server;
    const LocationConfig* location;
    Route();
};

namespace Router
{
    const ServerConfig*   selectServer(const std::vector<const ServerConfig*>& candidates,
                                       const std::string& hostHeader);
    bool                  prefixMatches(const std::string& prefix, const std::string& path);
    const LocationConfig* selectLocation(const ServerConfig& server, const std::string& path);
    std::string           toFilesystemPath(const LocationConfig& location, const std::string& path);
    Route                 resolve(const std::vector<const ServerConfig*>& candidates,
                                  const HttpRequest& request);
}

#endif
