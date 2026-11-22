#ifndef WEBSERV_UTILS_FILEUTILS_HPP
#define WEBSERV_UTILS_FILEUTILS_HPP

#include <string>
#include <vector>

namespace FileUtils
{
    bool exists(const std::string& path);
    bool isDirectory(const std::string& path);
    bool isRegularFile(const std::string& path);
    bool isReadable(const std::string& path);
    bool isWritable(const std::string& path);
    bool isExecutable(const std::string& path);
    bool fileSize(const std::string& path, size_t& out);
    bool readFile(const std::string& path, std::string& out, size_t maxBytes);
    bool writeFile(const std::string& path, const std::string& data);
    bool copyFile(const std::string& from, const std::string& to);
    bool listDirectory(const std::string& path, std::vector<std::string>& names);
}

#endif
