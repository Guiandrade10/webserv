#include "cgi/CgiEnvironment.hpp"

#include "utils/Constants.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <cctype>
#include <map>

namespace
{
    void add(std::vector<std::string>& environment, const std::string& name, const std::string& value)
    {
        environment.push_back(name + "=" + value);
    }

    std::string headerVariable(const std::string& lowerName)
    {
        std::string name = "HTTP_";
        for (size_t i = 0; i < lowerName.size(); ++i)
        {
            char c = lowerName[i];
            name += (c == '-') ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return name;
    }

    std::string scriptName(const std::string& requestPath, const std::string& scriptPath)
    {
        std::string file = PathUtils::baseName(scriptPath);
        if (PathUtils::baseName(requestPath) == file)
            return requestPath;
        return PathUtils::join(requestPath, file);
    }

    std::string serverName(const HttpRequest& request, const ConnectionInfo& connection)
    {
        std::string host = request.header("host");
        size_t colon = host.find(':');
        if (colon != std::string::npos)
            host.erase(colon);
        return host.empty() ? connection.serverAddress : host;
    }
}

namespace CgiEnvironment
{
    std::vector<std::string> build(const HttpRequest& request, const Route& route,
                                   const std::string& scriptPath, const ConnectionInfo& connection)
    {
        (void)route;
        std::vector<std::string> environment;
        add(environment, "GATEWAY_INTERFACE", "CGI/1.1");
        add(environment, "SERVER_SOFTWARE", kServerSoftware);
        add(environment, "SERVER_PROTOCOL", request.version());
        add(environment, "SERVER_NAME", serverName(request, connection));
        add(environment, "SERVER_PORT", StringUtils::toString(connection.serverPort));
        add(environment, "REQUEST_METHOD", request.method());
        add(environment, "REQUEST_URI", request.target());
        add(environment, "SCRIPT_NAME", scriptName(request.path(), scriptPath));
        add(environment, "SCRIPT_FILENAME", scriptPath);
        add(environment, "PATH_TRANSLATED", scriptPath);
        add(environment, "PATH_INFO", request.path());
        add(environment, "QUERY_STRING", request.query());
        add(environment, "CONTENT_LENGTH", StringUtils::toString(request.bodySize()));
        if (request.hasHeader("content-type"))
            add(environment, "CONTENT_TYPE", request.header("content-type"));
        add(environment, "REMOTE_ADDR", connection.remoteAddress);
        add(environment, "REMOTE_PORT", StringUtils::toString(connection.remotePort));
        add(environment, "REDIRECT_STATUS", "200");
        add(environment, "PATH", "/usr/local/bin:/usr/bin:/bin");
        const std::map<std::string, std::string>& headers = request.headers();
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it)
        {
            if (it->first != "content-type" && it->first != "content-length")
                add(environment, headerVariable(it->first), it->second);
        }
        return environment;
    }
}
