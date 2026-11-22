#include "http/RequestParser.hpp"

#include "utils/Constants.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <cctype>
#include <cstring>
#include <limits>
#include <vector>

namespace
{
    const size_t kMaxMethodLength = 16;

    bool isTokenChar(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c))
            || (c != '\0' && std::strchr("!#$%&'*+-.^_`|~", c) != NULL);
    }

    bool isToken(const std::string& text)
    {
        if (text.empty())
            return false;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (!isTokenChar(text[i]))
                return false;
        }
        return true;
    }

    bool hasControlOrSpace(const std::string& text)
    {
        for (size_t i = 0; i < text.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c <= 0x20 || c == 0x7F)
                return true;
        }
        return false;
    }

    int checkVersion(const std::string& version)
    {
        bool wellFormed = version.size() == 8 && StringUtils::startsWith(version, "HTTP/")
            && std::isdigit(static_cast<unsigned char>(version[5])) && version[6] == '.'
            && std::isdigit(static_cast<unsigned char>(version[7]));
        if (!wellFormed)
            return 400;
        if (version != "HTTP/1.1" && version != "HTTP/1.0")
            return 505;
        return 0;
    }
}

RequestParser::RequestParser()
    : _state(REQUEST_LINE), _request(), _maxBodySize(0), _headerBytes(0), _headerCount(0),
      _chunked(false), _contentLength(0), _bodyRemaining(0), _bodyReceived(0), _errorStatus(0)
{
}

void RequestParser::reset()
{
    _state = REQUEST_LINE;
    _request.reset();
    _maxBodySize = 0;
    _headerBytes = 0;
    _headerCount = 0;
    _chunked = false;
    _contentLength = 0;
    _bodyRemaining = 0;
    _bodyReceived = 0;
    _errorStatus = 0;
}

void RequestParser::setMaxBodySize(size_t maxBytes) { _maxBodySize = maxBytes; }
int RequestParser::errorStatus() const { return _errorStatus; }
HttpRequest& RequestParser::request() { return _request; }
const HttpRequest& RequestParser::request() const { return _request; }

bool RequestParser::inProgress() const
{
    return _state != REQUEST_LINE && _state != DONE && _state != FAILED;
}

RequestParser::Status RequestParser::parse(std::string& buffer)
{
    while (true)
    {
        bool progressed = true;
        switch (_state)
        {
            case DONE:               return COMPLETE;
            case FAILED:             return ERROR;
            case REQUEST_LINE:       progressed = stepRequestLine(buffer); break;
            case HEADERS:
                progressed = stepHeaderLine(buffer);
                if (_state == WAITING_BODY_LIMIT)
                    return HEADERS_READY;
                break;
            case WAITING_BODY_LIMIT: progressed = startBody(); break;
            case BODY_LENGTH:        progressed = stepBodyData(buffer, DONE); break;
            case CHUNK_SIZE:         progressed = stepChunkSize(buffer); break;
            case CHUNK_DATA:         progressed = stepBodyData(buffer, CHUNK_DATA_END); break;
            case CHUNK_DATA_END:     progressed = stepChunkDataEnd(buffer); break;
            case TRAILERS:           progressed = stepTrailerLine(buffer); break;
        }
        if (!progressed)
            return NEED_MORE;
    }
}

bool RequestParser::takeLine(std::string& buffer, std::string& line)
{
    size_t newline = buffer.find('\n');
    if (newline == std::string::npos)
        return false;
    size_t end = (newline > 0 && buffer[newline - 1] == '\r') ? newline - 1 : newline;
    line = buffer.substr(0, end);
    buffer.erase(0, newline + 1);
    return true;
}

bool RequestParser::fail(int status)
{
    _errorStatus = status;
    _state = FAILED;
    return true;
}

bool RequestParser::reject(int status)
{
    fail(status);
    return false;
}

bool RequestParser::stepRequestLine(std::string& buffer)
{
    std::string line;
    if (!takeLine(buffer, line))
    {
        if (buffer.size() > kMaxRequestLineSize)
            return fail(414);
        return false;
    }
    if (line.size() > kMaxRequestLineSize)
        return fail(414);
    if (line.empty())
        return true;
    if (handleRequestLine(line))
        _state = HEADERS;
    return true;
}

bool RequestParser::handleRequestLine(const std::string& line)
{
    std::vector<std::string> parts = StringUtils::split(line, ' ');
    if (parts.size() != 3 || parts[0].empty() || parts[1].empty() || parts[2].empty())
        return reject(400);
    if (parts[0].size() > kMaxMethodLength || !isToken(parts[0]))
        return reject(400);
    std::string path;
    std::string query;
    if (!handleTarget(parts[1], path, query))
        return false;
    int versionError = checkVersion(parts[2]);
    if (versionError != 0)
        return reject(versionError);
    _request.setRequestLine(parts[0], parts[1], path, query, parts[2]);
    return true;
}

bool RequestParser::handleTarget(const std::string& rawTarget, std::string& path,
                                 std::string& query)
{
    if (hasControlOrSpace(rawTarget))
        return reject(400);
    std::string target = rawTarget;
    std::string lower = StringUtils::toLower(target);
    if (StringUtils::startsWith(lower, "http://") || StringUtils::startsWith(lower, "https://"))
    {
        size_t slash = target.find('/', target.find("://") + 3);
        target = (slash == std::string::npos) ? "/" : target.substr(slash);
    }
    if (target[0] != '/')
        return reject(400);
    size_t hash = target.find('#');
    if (hash != std::string::npos)
        target.erase(hash);
    size_t question = target.find('?');
    query = (question == std::string::npos) ? "" : target.substr(question + 1);
    std::string decoded;
    if (!StringUtils::urlDecode(target.substr(0, question), decoded)
        || !PathUtils::normalizeUriPath(decoded, path))
        return reject(400);
    return true;
}

bool RequestParser::countHeaderBytes(size_t length)
{
    _headerBytes += length;
    if (_headerBytes > kMaxHeaderSectionSize)
        return reject(431);
    return true;
}

bool RequestParser::stepHeaderLine(std::string& buffer)
{
    std::string line;
    if (!takeLine(buffer, line))
    {
        if (_headerBytes + buffer.size() > kMaxHeaderSectionSize)
            return fail(431);
        return false;
    }
    if (!countHeaderBytes(line.size() + 2))
        return true;
    if (line.empty())
    {
        if (finishHeaders())
            _state = WAITING_BODY_LIMIT;
        return true;
    }
    if (++_headerCount > kMaxHeaderCount)
        return fail(431);
    handleHeaderLine(line);
    return true;
}

bool RequestParser::handleHeaderLine(const std::string& line)
{
    if (line[0] == ' ' || line[0] == '\t')
        return reject(400);
    size_t colon = line.find(':');
    if (colon == std::string::npos || !isToken(line.substr(0, colon)))
        return reject(400);
    std::string name = StringUtils::toLower(line.substr(0, colon));
    std::string value = StringUtils::trim(line.substr(colon + 1));
    if (!_request.addHeader(name, value))
        return reject(400);
    return true;
}

bool RequestParser::finishHeaders()
{
    if (_request.version() == "HTTP/1.1" && !_request.hasHeader("host"))
        return reject(400);
    bool hasEncoding = _request.hasHeader("transfer-encoding");
    bool hasLength = _request.hasHeader("content-length");
    if (hasEncoding && hasLength)
        return reject(400);
    if (hasEncoding)
    {
        if (!StringUtils::iequals(_request.header("transfer-encoding"), "chunked"))
            return reject(501);
        _chunked = true;
    }
    if (hasLength && !StringUtils::parseUnsigned(_request.header("content-length"), _contentLength))
        return reject(400);
    return true;
}

bool RequestParser::startBody()
{
    if (_chunked)
        _state = CHUNK_SIZE;
    else if (_contentLength == 0)
        _state = DONE;
    else if (_maxBodySize != 0 && _contentLength > _maxBodySize)
        return fail(413);
    else
    {
        _bodyRemaining = _contentLength;
        _state = BODY_LENGTH;
    }
    return true;
}

bool RequestParser::stepBodyData(std::string& buffer, State next)
{
    if (buffer.empty())
        return false;
    size_t count = (buffer.size() < _bodyRemaining) ? buffer.size() : _bodyRemaining;
    if (!_request.appendBody(buffer.c_str(), count))
        return fail(500);
    buffer.erase(0, count);
    _bodyRemaining -= count;
    _bodyReceived += count;
    if (_bodyRemaining == 0)
    {
        _state = next;
        if (next == DONE)
            _request.finishBody();
    }
    return true;
}

bool RequestParser::stepChunkSize(std::string& buffer)
{
    std::string line;
    if (!takeLine(buffer, line))
    {
        if (buffer.size() > kMaxChunkSizeLine)
            return fail(400);
        return false;
    }
    size_t semicolon = line.find(';');
    if (semicolon != std::string::npos)
        line.erase(semicolon);
    size_t size = 0;
    if (!StringUtils::parseHex(StringUtils::trim(line), size))
        return fail(400);
    if (size == 0)
    {
        _state = TRAILERS;
        return true;
    }
    if (size > std::numeric_limits<size_t>::max() - _bodyReceived
        || (_maxBodySize != 0 && _bodyReceived + size > _maxBodySize))
        return fail(413);
    _bodyRemaining = size;
    _state = CHUNK_DATA;
    return true;
}

bool RequestParser::stepChunkDataEnd(std::string& buffer)
{
    if (buffer.empty())
        return false;
    if (buffer[0] == '\n')
        buffer.erase(0, 1);
    else if (buffer[0] != '\r')
        return fail(400);
    else if (buffer.size() < 2)
        return false;
    else if (buffer[1] != '\n')
        return fail(400);
    else
        buffer.erase(0, 2);
    _state = CHUNK_SIZE;
    return true;
}

bool RequestParser::stepTrailerLine(std::string& buffer)
{
    std::string line;
    if (!takeLine(buffer, line))
    {
        if (_headerBytes + buffer.size() > kMaxHeaderSectionSize)
            return fail(431);
        return false;
    }
    if (!countHeaderBytes(line.size() + 2))
        return true;
    if (line.empty())
    {
        _request.finishBody();
        _state = DONE;
    }
    return true;
}
