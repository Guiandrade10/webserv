#ifndef WEBSERV_CGI_CGIENVIRONMENT_HPP
#define WEBSERV_CGI_CGIENVIRONMENT_HPP

#include "handlers/RequestHandler.hpp"
#include "handlers/Router.hpp"
#include "http/HttpRequest.hpp"

#include <string>
#include <vector>

namespace CgiEnvironment
{
    std::vector<std::string> build(const HttpRequest& request, const Route& route,
                                   const std::string& scriptPath, const ConnectionInfo& connection);
}

#endif
