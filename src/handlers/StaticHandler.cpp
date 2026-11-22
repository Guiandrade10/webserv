#include "handlers/StaticHandler.hpp"

#include "handlers/AutoIndex.hpp"
#include "handlers/ErrorPages.hpp"
#include "http/MimeTypes.hpp"
#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"

#include <vector>

namespace StaticHandler
{
    bool findIndex(const std::string& directory, const LocationConfig& location, std::string& indexPath);
}

namespace
{
    HttpResponse serveFile(const std::string& path, const Route& route)
    {
        size_t size = 0;
        if (!FileUtils::isReadable(path) || !FileUtils::fileSize(path, size))
            return ErrorPages::build(403, route.server);
        HttpResponse response;
        response.setFileBody(path, 0, size, MimeTypes::forPath(path));
        return response;
    }

    HttpResponse serveDirectory(const HttpRequest& request, const Route& route,
                                const std::string& directory)
    {
        std::string indexPath;
        if (StaticHandler::findIndex(directory, *route.location, indexPath))
            return serveFile(indexPath, route);
        if (!route.location->autoindex())
            return ErrorPages::build(404, route.server);
        std::string html;
        if (!AutoIndex::render(directory, request.path(), html))
            return ErrorPages::build(403, route.server);
        HttpResponse response;
        response.setBody(html, "text/html");
        return response;
    }
}

namespace StaticHandler
{
    bool findIndex(const std::string& directory, const LocationConfig& location, std::string& indexPath)
    {
        const std::vector<std::string>& indexFiles = location.indexFiles();
        for (size_t i = 0; i < indexFiles.size(); ++i)
        {
            std::string candidate = PathUtils::join(directory, indexFiles[i]);
            if (FileUtils::isRegularFile(candidate))
            {
                indexPath = candidate;
                return true;
            }
        }
        return false;
    }

    HttpResponse serve(const HttpRequest& request, const Route& route, const std::string& fsPath)
    {
        if (!FileUtils::exists(fsPath))
            return ErrorPages::build(404, route.server);
        if (FileUtils::isDirectory(fsPath))
            return serveDirectory(request, route, fsPath);
        if (FileUtils::isRegularFile(fsPath))
            return serveFile(fsPath, route);
        return ErrorPages::build(403, route.server);
    }
}
