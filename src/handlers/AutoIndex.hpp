#ifndef WEBSERV_HANDLERS_AUTOINDEX_HPP
#define WEBSERV_HANDLERS_AUTOINDEX_HPP

#include <string>

namespace AutoIndex
{
    bool render(const std::string& directory, const std::string& uriPath, std::string& html);
}

#endif
