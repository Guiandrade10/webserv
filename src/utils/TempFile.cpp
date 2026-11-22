#include "utils/TempFile.hpp"

#include "utils/Constants.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

unsigned long TempFile::_counter = 0;

TempFile::TempFile() : _path(), _fd(-1), _size(0)
{
}

TempFile::~TempFile()
{
    remove();
}

bool TempFile::create()
{
    remove();
    std::string prefix = PathUtils::join(kTempDirectory, "webserv_")
        + StringUtils::toString(static_cast<unsigned long>(std::time(NULL))) + "_";
    for (int attempt = 0; attempt < kTempFileCreateAttempts; ++attempt)
    {
        std::string candidate = prefix + StringUtils::toString(_counter++) + ".tmp";
        int fd = open(candidate.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd >= 0)
        {
            _path = candidate;
            _fd = fd;
            _size = 0;
            return true;
        }
    }
    return false;
}

bool TempFile::append(const char* data, size_t length)
{
    if (_fd < 0)
        return false;
    size_t written = 0;
    while (written < length)
    {
        ssize_t result = write(_fd, data + written, length - written);
        if (result <= 0)
            return false;
        written += static_cast<size_t>(result);
        _size += static_cast<size_t>(result);
    }
    return true;
}

void TempFile::closeWriter()
{
    if (_fd >= 0)
    {
        close(_fd);
        _fd = -1;
    }
}

void TempFile::remove()
{
    closeWriter();
    if (!_path.empty())
        std::remove(_path.c_str());
    _path.clear();
    _size = 0;
}

bool TempFile::isCreated() const
{
    return !_path.empty();
}

const std::string& TempFile::path() const
{
    return _path;
}

size_t TempFile::size() const
{
    return _size;
}
