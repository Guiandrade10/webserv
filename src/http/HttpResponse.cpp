#include "http/HttpResponse.hpp"

#include "http/StatusCodes.hpp"
#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

#include <ctime>

namespace
{
    std::string httpDate()
    {
        std::time_t now = std::time(NULL);
        std::tm* gmt = std::gmtime(&now);
        char buffer[64];
        if (gmt == NULL || std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt) == 0)
            return "Thu, 01 Jan 1970 00:00:00 GMT";
        return buffer;
    }

    bool statusHasBody(int status)
    {
        return status >= 200 && status != 204 && status != 304;
    }

    bool isManagedHeader(const std::string& name)
    {
        return StringUtils::iequals(name, "Content-Length") || StringUtils::iequals(name, "Connection");
    }
}

HttpResponse::HttpResponse()
    : _status(200), _headers(), _body(), _filePath(), _fileOffset(0), _fileLength(0)
{
}

void HttpResponse::setStatus(int code) { _status = code; }
int HttpResponse::status() const { return _status; }
bool HttpResponse::hasFileBody() const { return !_filePath.empty(); }
const std::string& HttpResponse::body() const { return _body; }
const std::string& HttpResponse::filePath() const { return _filePath; }
size_t HttpResponse::fileOffset() const { return _fileOffset; }
size_t HttpResponse::fileLength() const { return _fileLength; }

void HttpResponse::setHeader(const std::string& name, const std::string& value)
{
    for (size_t i = 0; i < _headers.size(); ++i)
    {
        if (StringUtils::iequals(_headers[i].first, name))
        {
            _headers[i].second = value;
            return;
        }
    }
    _headers.push_back(std::make_pair(name, value));
}

bool HttpResponse::hasHeader(const std::string& name) const
{
    for (size_t i = 0; i < _headers.size(); ++i)
    {
        if (StringUtils::iequals(_headers[i].first, name))
            return true;
    }
    return false;
}

std::string HttpResponse::header(const std::string& name) const
{
    for (size_t i = 0; i < _headers.size(); ++i)
    {
        if (StringUtils::iequals(_headers[i].first, name))
            return _headers[i].second;
    }
    return "";
}

void HttpResponse::setBody(const std::string& body, const std::string& contentType)
{
    _body = body;
    _filePath.clear();
    _fileOffset = 0;
    _fileLength = 0;
    setHeader("Content-Type", contentType);
}

void HttpResponse::setFileBody(const std::string& path, size_t offset, size_t length,
                               const std::string& contentType)
{
    _body.clear();
    _filePath = path;
    _fileOffset = offset;
    _fileLength = length;
    setHeader("Content-Type", contentType);
}

size_t HttpResponse::contentLength() const
{
    return hasFileBody() ? _fileLength : _body.size();
}

std::string HttpResponse::serializeHead(bool keepAlive) const
{
    std::string head = "HTTP/1.1 " + StringUtils::toString(_status) + " "
        + StatusCodes::reasonPhrase(_status) + "\r\n";
    head += std::string("Server: ") + kServerSoftware + "\r\n";
    head += "Date: " + httpDate() + "\r\n";
    for (size_t i = 0; i < _headers.size(); ++i)
    {
        if (!isManagedHeader(_headers[i].first))
            head += _headers[i].first + ": " + _headers[i].second + "\r\n";
    }
    if (statusHasBody(_status))
        head += "Content-Length: " + StringUtils::toString(contentLength()) + "\r\n";
    head += std::string("Connection: ") + (keepAlive ? "keep-alive" : "close") + "\r\n";
    head += "\r\n";
    return head;
}
