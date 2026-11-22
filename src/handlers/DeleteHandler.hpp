#ifndef WEBSERV_HANDLERS_DELETEHANDLER_HPP
#define WEBSERV_HANDLERS_DELETEHANDLER_HPP

#include "handlers/Router.hpp"
#include "http/HttpResponse.hpp"

#include <string>

namespace DeleteHandler
{
    HttpResponse handle(const std::string& fsPath, const Route& route);
}

#endif
