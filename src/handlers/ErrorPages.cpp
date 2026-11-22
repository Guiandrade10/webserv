#include "handlers/ErrorPages.hpp"

#include "http/StatusCodes.hpp"
#include "utils/Constants.hpp"
#include "utils/FileUtils.hpp"
#include "utils/StringUtils.hpp"

#include <map>

namespace
{
    std::string defaultPage(int status)
    {
        std::string title = StringUtils::toString(status) + " " + StatusCodes::reasonPhrase(status);
        return "<!DOCTYPE html>\n"
               "<html><head><meta charset=\"utf-8\"><title>" + title + "</title></head>\n"
               "<body><h1>" + title + "</h1><hr><p>" + kServerSoftware + "</p></body></html>\n";
    }
}

namespace ErrorPages
{
    HttpResponse build(int status, const ServerConfig* server)
    {
        HttpResponse response;
        response.setStatus(status);
        if (server != NULL)
        {
            std::map<int, std::string>::const_iterator page = server->errorPages().find(status);
            std::string content;
            if (page != server->errorPages().end() && FileUtils::isReadable(page->second)
                && FileUtils::readFile(page->second, content, kMaxErrorPageSize))
            {
                response.setBody(content, "text/html");
                return response;
            }
        }
        response.setBody(defaultPage(status), "text/html");
        return response;
    }
}
