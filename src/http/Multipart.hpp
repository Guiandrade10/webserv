#ifndef WEBSERV_HTTP_MULTIPART_HPP
#define WEBSERV_HTTP_MULTIPART_HPP

#include <string>
#include <vector>

struct MultipartPart
{
    std::string name;
    std::string fileName;
    std::string contentType;
    std::string content;
};

namespace Multipart
{
    bool extractBoundary(const std::string& contentTypeHeader, std::string& boundary);
    bool parse(const std::string& body, const std::string& boundary,
               std::vector<MultipartPart>& parts);
}

#endif
