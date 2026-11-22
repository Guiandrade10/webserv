#ifndef WEBSERV_CGI_CGIOUTPUTPARSER_HPP
#define WEBSERV_CGI_CGIOUTPUTPARSER_HPP

#include "http/HttpResponse.hpp"
#include "utils/TempFile.hpp"

namespace CgiOutputParser
{
    bool toResponse(const TempFile& output, HttpResponse& response);
}

#endif
