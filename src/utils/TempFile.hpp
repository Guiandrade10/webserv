#ifndef WEBSERV_UTILS_TEMPFILE_HPP
#define WEBSERV_UTILS_TEMPFILE_HPP

#include <cstddef>
#include <string>

class TempFile
{
public:
    TempFile();
    ~TempFile();
    bool create();
    bool append(const char* data, size_t length);
    void closeWriter();
    void remove();
    bool isCreated() const;
    const std::string& path() const;
    size_t size() const;
private:
    TempFile(const TempFile&);
    TempFile& operator=(const TempFile&);
    static unsigned long _counter;
    std::string _path;
    int         _fd;
    size_t      _size;
};

#endif
