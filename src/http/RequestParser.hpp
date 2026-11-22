#ifndef WEBSERV_HTTP_REQUESTPARSER_HPP
#define WEBSERV_HTTP_REQUESTPARSER_HPP

#include "http/HttpRequest.hpp"

#include <cstddef>
#include <string>

class RequestParser
{
public:
    enum Status { NEED_MORE, HEADERS_READY, COMPLETE, ERROR };

    RequestParser();
    void reset();
    Status parse(std::string& buffer);
    void setMaxBodySize(size_t maxBytes);
    int errorStatus() const;
    bool inProgress() const;
    HttpRequest& request();
    const HttpRequest& request() const;

private:
    enum State { REQUEST_LINE, HEADERS, WAITING_BODY_LIMIT, BODY_LENGTH, CHUNK_SIZE,
                 CHUNK_DATA, CHUNK_DATA_END, TRAILERS, DONE, FAILED };

    RequestParser(const RequestParser&);
    RequestParser& operator=(const RequestParser&);

    bool stepRequestLine(std::string& buffer);
    bool stepHeaderLine(std::string& buffer);
    bool startBody();
    bool stepBodyData(std::string& buffer, State next);
    bool stepChunkSize(std::string& buffer);
    bool stepChunkDataEnd(std::string& buffer);
    bool stepTrailerLine(std::string& buffer);

    bool handleRequestLine(const std::string& line);
    bool handleTarget(const std::string& rawTarget, std::string& path, std::string& query);
    bool handleHeaderLine(const std::string& line);
    bool finishHeaders();
    bool countHeaderBytes(size_t length);
    static bool takeLine(std::string& buffer, std::string& line);
    bool fail(int status);
    bool reject(int status);

    State       _state;
    HttpRequest _request;
    size_t      _maxBodySize;
    size_t      _headerBytes;
    size_t      _headerCount;
    bool        _chunked;
    size_t      _contentLength;
    size_t      _bodyRemaining;
    size_t      _bodyReceived;
    int         _errorStatus;
};

#endif
