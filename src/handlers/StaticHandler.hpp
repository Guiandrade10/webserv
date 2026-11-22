#ifndef WEBSERV_HANDLERS_STATICHANDLER_HPP
#define WEBSERV_HANDLERS_STATICHANDLER_HPP

#include "handlers/Router.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

#include <string>

namespace StaticHandler
{
    HttpResponse serve(const HttpRequest& request, const Route& route, const std::string& fsPath);
    bool findIndex(const std::string& directory, const LocationConfig& location, std::string& indexPath);
}

#endif
