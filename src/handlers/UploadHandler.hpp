#ifndef WEBSERV_HANDLERS_UPLOADHANDLER_HPP
#define WEBSERV_HANDLERS_UPLOADHANDLER_HPP

#include "handlers/Router.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

namespace UploadHandler
{
    HttpResponse handle(const HttpRequest& request, const Route& route);
}

#endif
