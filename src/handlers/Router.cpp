#include "handlers/Router.hpp"

#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

Route::Route() : server(NULL), location(NULL)
{
}

namespace Router
{
    const ServerConfig* selectServer(const std::vector<const ServerConfig*>& candidates,
                                     const std::string& hostHeader)
    {
        if (candidates.empty())
            return NULL;
        std::string host = StringUtils::toLower(hostHeader);
        size_t colon = host.find(':');
        if (colon != std::string::npos)
            host.erase(colon);
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (!host.empty() && candidates[i]->hasServerName(host))
                return candidates[i];
        }
        return candidates[0];
    }

    bool prefixMatches(const std::string& prefix, const std::string& path)
    {
        if (prefix == "/" || path == prefix)
            return true;
        if (!StringUtils::startsWith(path, prefix))
            return false;
        return prefix[prefix.size() - 1] == '/' || path[prefix.size()] == '/';
    }

    const LocationConfig* selectLocation(const ServerConfig& server, const std::string& path)
    {
        const LocationConfig* best = NULL;
        const std::vector<LocationConfig>& locations = server.locations();
        for (size_t i = 0; i < locations.size(); ++i)
        {
            if (prefixMatches(locations[i].path(), path)
                && (best == NULL || locations[i].path().size() > best->path().size()))
                best = &locations[i];
        }
        return best;
    }

    std::string toFilesystemPath(const LocationConfig& location, const std::string& path)
    {
        return PathUtils::join(location.root(), path.substr(location.path().size()));
    }

    Route resolve(const std::vector<const ServerConfig*>& candidates, const HttpRequest& request)
    {
        Route route;
        route.server = selectServer(candidates, request.header("host"));
        if (route.server != NULL)
            route.location = selectLocation(*route.server, request.path());
        return route;
    }
}
