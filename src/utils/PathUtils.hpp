#ifndef WEBSERV_UTILS_PATHUTILS_HPP
#define WEBSERV_UTILS_PATHUTILS_HPP

#include <string>

namespace PathUtils
{
    std::string join(const std::string& base, const std::string& tail);
    std::string parentDirectory(const std::string& path);
    std::string baseName(const std::string& path);
    std::string extension(const std::string& path);
    bool normalizeUriPath(const std::string& decodedPath, std::string& out);
    bool makeAbsolute(const std::string& path, std::string& out);
    bool sanitizeFileName(const std::string& rawName, std::string& out);
}

#endif
