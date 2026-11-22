#ifndef WEBSERV_HANDLERS_ERRORPAGES_HPP
#define WEBSERV_HANDLERS_ERRORPAGES_HPP

#include "config/ServerConfig.hpp"
#include "http/HttpResponse.hpp"

namespace ErrorPages
{
    HttpResponse build(int status, const ServerConfig* server);
}

#endif
