#ifndef WEBSERV_HTTP_HTTPRESPONSE_HPP
#define WEBSERV_HTTP_HTTPRESPONSE_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class HttpResponse
{
public:
    HttpResponse();
    void setStatus(int code);
    int  status() const;
    void setHeader(const std::string& name, const std::string& value);
    bool hasHeader(const std::string& name) const;
    std::string header(const std::string& name) const;
    void setBody(const std::string& body, const std::string& contentType);
    void setFileBody(const std::string& path, size_t offset, size_t length,
                     const std::string& contentType);
    bool hasFileBody() const;
    const std::string& body() const;
    const std::string& filePath() const;
    size_t fileOffset() const;
    size_t fileLength() const;
    size_t contentLength() const;
    std::string serializeHead(bool keepAlive) const;

private:
    typedef std::vector<std::pair<std::string, std::string> > HeaderList;

    int         _status;
    HeaderList  _headers;
    std::string _body;
    std::string _filePath;
    size_t      _fileOffset;
    size_t      _fileLength;
};

#endif
