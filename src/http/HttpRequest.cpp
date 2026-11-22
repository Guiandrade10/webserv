#include "http/HttpRequest.hpp"

#include "utils/StringUtils.hpp"

#include <vector>

HttpRequest::HttpRequest()
    : _method(), _target(), _path(), _query(), _version(), _headers(), _body()
{
}

void HttpRequest::reset()
{
    _method.clear();
    _target.clear();
    _path.clear();
    _query.clear();
    _version.clear();
    _headers.clear();
    _body.remove();
}

const std::string& HttpRequest::method() const { return _method; }
const std::string& HttpRequest::target() const { return _target; }
const std::string& HttpRequest::path() const { return _path; }
const std::string& HttpRequest::query() const { return _query; }
const std::string& HttpRequest::version() const { return _version; }
const std::map<std::string, std::string>& HttpRequest::headers() const { return _headers; }
size_t HttpRequest::bodySize() const { return _body.size(); }
const std::string& HttpRequest::bodyPath() const { return _body.path(); }

bool HttpRequest::hasHeader(const std::string& lowerName) const
{
    return _headers.find(lowerName) != _headers.end();
}

std::string HttpRequest::header(const std::string& lowerName) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowerName);
    return (it == _headers.end()) ? std::string() : it->second;
}

bool HttpRequest::connectionHas(const std::string& token) const
{
    std::vector<std::string> tokens = StringUtils::split(header("connection"), ',');
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (StringUtils::iequals(StringUtils::trim(tokens[i]), token))
            return true;
    }
    return false;
}

bool HttpRequest::keepAlive() const
{
    if (_version == "HTTP/1.0")
        return connectionHas("keep-alive");
    return !connectionHas("close");
}

void HttpRequest::setRequestLine(const std::string& method, const std::string& target,
                                 const std::string& path, const std::string& query,
                                 const std::string& version)
{
    _method = method;
    _target = target;
    _path = path;
    _query = query;
    _version = version;
}

bool HttpRequest::addHeader(const std::string& lowerName, const std::string& value)
{
    std::map<std::string, std::string>::iterator it = _headers.find(lowerName);
    if (it == _headers.end())
    {
        _headers[lowerName] = value;
        return true;
    }
    if (lowerName == "host")
        return false;
    if (lowerName == "content-length")
        return it->second == value;
    it->second += ", " + value;
    return true;
}

bool HttpRequest::appendBody(const char* data, size_t length)
{
    if (length == 0)
        return true;
    if (!_body.isCreated() && !_body.create())
        return false;
    return _body.append(data, length);
}

void HttpRequest::finishBody()
{
    _body.closeWriter();
}
