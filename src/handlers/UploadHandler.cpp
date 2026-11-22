#include "handlers/UploadHandler.hpp"

#include "handlers/ErrorPages.hpp"
#include "http/Multipart.hpp"
#include "utils/Constants.hpp"
#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <ctime>
#include <vector>

namespace
{
    HttpResponse created(const LocationConfig& location, const std::vector<std::string>& names)
    {
        HttpResponse response;
        response.setStatus(201);
        std::string html = "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>201 Created"
                           "</title></head>\n<body><h1>Uploaded</h1>\n<ul>\n";
        for (size_t i = 0; i < names.size(); ++i)
        {
            std::string url = PathUtils::join(location.path(), StringUtils::urlEncodePathSegment(names[i]));
            if (i == 0)
                response.setHeader("Location", url);
            html += "<li><a href=\"" + StringUtils::htmlEscape(url) + "\">"
                  + StringUtils::htmlEscape(names[i]) + "</a></li>\n";
        }
        response.setBody(html + "</ul>\n</body></html>\n", "text/html");
        return response;
    }

    bool generateName(const std::string& directory, std::string& name)
    {
        std::time_t now = std::time(NULL);
        std::tm* local = std::localtime(&now);
        char stamp[32];
        if (local == NULL || std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", local) == 0)
            return false;
        for (int i = 1; i <= kMaxGeneratedNameAttempts; ++i)
        {
            name = std::string("upload_") + stamp + "_" + StringUtils::toString(i) + ".bin";
            if (!FileUtils::exists(PathUtils::join(directory, name)))
                return true;
        }
        return false;
    }

    HttpResponse saveMultipart(const HttpRequest& request, const Route& route,
                               const std::string& directory)
    {
        std::string boundary;
        if (!Multipart::extractBoundary(request.header("content-type"), boundary))
            return ErrorPages::build(400, route.server);
        if (request.bodySize() > kMaxInMemoryMultipart)
            return ErrorPages::build(413, route.server);
        std::string body;
        if (request.bodySize() > 0 && !FileUtils::readFile(request.bodyPath(), body, kMaxInMemoryMultipart))
            return ErrorPages::build(500, route.server);
        std::vector<MultipartPart> parts;
        if (!Multipart::parse(body, boundary, parts))
            return ErrorPages::build(400, route.server);
        std::vector<std::string> names;
        std::vector<const MultipartPart*> files;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (parts[i].fileName.empty())
                continue;
            std::string name;
            if (!PathUtils::sanitizeFileName(parts[i].fileName, name))
                return ErrorPages::build(400, route.server);
            names.push_back(name);
            files.push_back(&parts[i]);
        }
        if (files.empty())
            return ErrorPages::build(400, route.server);
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (!FileUtils::writeFile(PathUtils::join(directory, names[i]), files[i]->content))
                return ErrorPages::build(500, route.server);
        }
        return created(*route.location, names);
    }

    HttpResponse saveRaw(const HttpRequest& request, const Route& route, const std::string& directory)
    {
        std::string rest = request.path().substr(route.location->path().size());
        std::string name = PathUtils::baseName(rest);
        if (name.empty())
        {
            if (!generateName(directory, name))
                return ErrorPages::build(500, route.server);
        }
        else if (!PathUtils::sanitizeFileName(name, name))
            return ErrorPages::build(400, route.server);
        std::string destination = PathUtils::join(directory, name);
        if (FileUtils::isDirectory(destination))
            return ErrorPages::build(403, route.server);
        bool saved = (request.bodySize() == 0) ? FileUtils::writeFile(destination, "")
                                               : FileUtils::copyFile(request.bodyPath(), destination);
        if (!saved)
            return ErrorPages::build(500, route.server);
        return created(*route.location, std::vector<std::string>(1, name));
    }
}

namespace UploadHandler
{
    HttpResponse handle(const HttpRequest& request, const Route& route)
    {
        const std::string& directory = route.location->uploadStore();
        if (!FileUtils::isDirectory(directory) || !FileUtils::isWritable(directory))
            return ErrorPages::build(403, route.server);
        std::string type = StringUtils::toLower(StringUtils::trim(request.header("content-type")));
        if (StringUtils::startsWith(type, "multipart/form-data"))
            return saveMultipart(request, route, directory);
        return saveRaw(request, route, directory);
    }
}
