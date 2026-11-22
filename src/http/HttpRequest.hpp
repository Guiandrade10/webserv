#ifndef WEBSERV_HTTP_HTTPREQUEST_HPP
#define WEBSERV_HTTP_HTTPREQUEST_HPP

#include "utils/TempFile.hpp"

#include <cstddef>
#include <map>
#include <string>

class HttpRequest
{
public:
    HttpRequest();
    void reset();
    const std::string& method() const;
    const std::string& target() const;
    const std::string& path() const;
    const std::string& query() const;
    const std::string& version() const;
    const std::map<std::string, std::string>& headers() const;
    bool hasHeader(const std::string& lowerName) const;
    std::string header(const std::string& lowerName) const;
    bool keepAlive() const;
    size_t bodySize() const;
    const std::string& bodyPath() const;

    void setRequestLine(const std::string& method, const std::string& target,
                        const std::string& path, const std::string& query,
                        const std::string& version);
    bool addHeader(const std::string& lowerName, const std::string& value);
    bool appendBody(const char* data, size_t length);
    void finishBody();

private:
    HttpRequest(const HttpRequest&);
    HttpRequest& operator=(const HttpRequest&);
    bool connectionHas(const std::string& token) const;

    std::string                        _method;
    std::string                        _target;
    std::string                        _path;
    std::string                        _query;
    std::string                        _version;
    std::map<std::string, std::string> _headers;
    TempFile                           _body;
};

#endif
