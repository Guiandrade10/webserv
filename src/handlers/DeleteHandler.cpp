#include "handlers/DeleteHandler.hpp"

#include "handlers/ErrorPages.hpp"
#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"

#include <cstdio>

namespace DeleteHandler
{
    HttpResponse handle(const std::string& fsPath, const Route& route)
    {
        if (!FileUtils::exists(fsPath))
            return ErrorPages::build(404, route.server);
        if (FileUtils::isDirectory(fsPath))
            return ErrorPages::build(403, route.server);
        if (!FileUtils::isWritable(PathUtils::parentDirectory(fsPath)))
            return ErrorPages::build(403, route.server);
        if (std::remove(fsPath.c_str()) != 0)
            return ErrorPages::build(500, route.server);
        HttpResponse response;
        response.setStatus(204);
        return response;
    }
}
