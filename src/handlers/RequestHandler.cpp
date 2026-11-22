#include "handlers/RequestHandler.hpp"

#include "cgi/CgiEnvironment.hpp"
#include "handlers/DeleteHandler.hpp"
#include "handlers/ErrorPages.hpp"
#include "handlers/StaticHandler.hpp"
#include "handlers/UploadHandler.hpp"
#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <set>

HandlerResult::HandlerResult() : _isCgi(false), _response(), _job()
{
}

HandlerResult HandlerResult::fromResponse(const HttpResponse& response)
{
    HandlerResult result;
    result._response = response;
    return result;
}

HandlerResult HandlerResult::fromCgi(const CgiJob& job)
{
    HandlerResult result;
    result._isCgi = true;
    result._job = job;
    return result;
}

bool HandlerResult::isCgi() const { return _isCgi; }
const HttpResponse& HandlerResult::response() const { return _response; }
const CgiJob& HandlerResult::cgiJob() const { return _job; }

namespace
{
    const char* const kKnownMethods[] = {
        "GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", NULL
    };

    bool isKnownMethod(const std::string& method)
    {
        for (size_t i = 0; kKnownMethods[i] != NULL; ++i)
        {
            if (method == kKnownMethods[i])
                return true;
        }
        return false;
    }

    std::string allowHeader(const std::set<std::string>& methods)
    {
        std::string value;
        for (std::set<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it)
        {
            if (!value.empty())
                value += ", ";
            value += *it;
        }
        return value;
    }

    HttpResponse redirect(const LocationConfig& location)
    {
        HttpResponse response;
        response.setStatus(location.redirectCode());
        response.setHeader("Location", location.redirectTarget());
        std::string target = StringUtils::htmlEscape(location.redirectTarget());
        response.setBody("<!DOCTYPE html>\n<html><body><p>Moved to <a href=\"" + target + "\">"
                         + target + "</a>.</p></body></html>\n", "text/html");
        return response;
    }

    bool tryCgi(const HttpRequest& request, const Route& route, const ConnectionInfo& connection,
                const std::string& fsPath, HandlerResult& result)
    {
        if (request.method() != "GET" && request.method() != "POST")
            return false;
        std::string target = fsPath;
        std::string indexPath;
        if (FileUtils::isDirectory(fsPath) && StaticHandler::findIndex(fsPath, *route.location, indexPath))
            target = indexPath;
        const std::string* interpreter = route.location->findCgiInterpreter(PathUtils::extension(target));
        if (interpreter == NULL)
            return false;
        if (!FileUtils::exists(target))
            result = HandlerResult::fromResponse(ErrorPages::build(404, route.server));
        else if (!FileUtils::isRegularFile(target))
            result = HandlerResult::fromResponse(ErrorPages::build(403, route.server));
        else
        {
            CgiJob job;
            job.interpreter = *interpreter;
            job.scriptPath = target;
            job.workingDirectory = PathUtils::parentDirectory(target);
            job.bodyPath = request.bodyPath();
            job.environment = CgiEnvironment::build(request, route, target, connection);
            result = HandlerResult::fromCgi(job);
        }
        return true;
    }

    HttpResponse handleFile(const HttpRequest& request, const Route& route, const std::string& fsPath)
    {
        if (request.method() == "GET")
            return StaticHandler::serve(request, route, fsPath);
        if (request.method() == "POST")
        {
            if (!route.location->uploadsEnabled())
                return ErrorPages::build(403, route.server);
            return UploadHandler::handle(request, route);
        }
        return DeleteHandler::handle(fsPath, route);
    }
}

namespace RequestHandler
{
    HandlerResult handle(const HttpRequest& request, const Route& route,
                         const ConnectionInfo& connection)
    {
        const LocationConfig& location = *route.location;
        if (location.hasRedirect())
            return HandlerResult::fromResponse(redirect(location));
        if (!isKnownMethod(request.method()))
            return HandlerResult::fromResponse(ErrorPages::build(501, route.server));
        if (!location.allowsMethod(request.method()))
        {
            HttpResponse response = ErrorPages::build(405, route.server);
            response.setHeader("Allow", allowHeader(location.allowedMethods()));
            return HandlerResult::fromResponse(response);
        }
        std::string fsPath = Router::toFilesystemPath(location, request.path());
        HandlerResult cgi = HandlerResult::fromResponse(HttpResponse());
        if (tryCgi(request, route, connection, fsPath, cgi))
            return cgi;
        return HandlerResult::fromResponse(handleFile(request, route, fsPath));
    }
}
