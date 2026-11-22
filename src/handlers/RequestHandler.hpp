#ifndef WEBSERV_HANDLERS_REQUESTHANDLER_HPP
#define WEBSERV_HANDLERS_REQUESTHANDLER_HPP

#include "cgi/CgiProcess.hpp"
#include "handlers/Router.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

#include <string>

struct ConnectionInfo
{
    std::string remoteAddress;
    int         remotePort;
    std::string serverAddress;
    int         serverPort;
};

class HandlerResult
{
public:
    static HandlerResult fromResponse(const HttpResponse& response);
    static HandlerResult fromCgi(const CgiJob& job);
    bool isCgi() const;
    const HttpResponse& response() const;
    const CgiJob& cgiJob() const;

private:
    HandlerResult();
    bool         _isCgi;
    HttpResponse _response;
    CgiJob       _job;
};

namespace RequestHandler
{
    HandlerResult handle(const HttpRequest& request, const Route& route,
                         const ConnectionInfo& connection);
}

#endif
