#include "utils/PathUtils.hpp"

#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

#include <cstdlib>
#include <vector>

namespace
{
    std::string joinSegments(const std::vector<std::string>& segments)
    {
        std::string result;
        for (size_t i = 0; i < segments.size(); ++i)
            result += "/" + segments[i];
        return result.empty() ? "/" : result;
    }
}

namespace PathUtils
{
    std::string join(const std::string& base, const std::string& tail)
    {
        if (base.empty())
            return tail;
        if (tail.empty())
            return base;
        size_t baseEnd = base.size();
        while (baseEnd > 0 && base[baseEnd - 1] == '/')
            --baseEnd;
        size_t tailStart = 0;
        while (tailStart < tail.size() && tail[tailStart] == '/')
            ++tailStart;
        return base.substr(0, baseEnd) + "/" + tail.substr(tailStart);
    }

    std::string parentDirectory(const std::string& path)
    {
        size_t slash = path.rfind('/');
        if (slash == std::string::npos)
            return ".";
        if (slash == 0)
            return "/";
        return path.substr(0, slash);
    }

    std::string baseName(const std::string& path)
    {
        size_t slash = path.rfind('/');
        if (slash == std::string::npos)
            return path;
        return path.substr(slash + 1);
    }

    std::string extension(const std::string& path)
    {
        std::string name = baseName(path);
        size_t dot = name.rfind('.');
        if (dot == std::string::npos || dot == 0)
            return "";
        return name.substr(dot);
    }

    bool normalizeUriPath(const std::string& decodedPath, std::string& out)
    {
        if (decodedPath.empty() || decodedPath[0] != '/')
            return false;
        std::vector<std::string> parts = StringUtils::split(decodedPath, '/');
        std::vector<std::string> kept;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (parts[i].empty() || parts[i] == ".")
                continue;
            if (parts[i] == "..")
            {
                if (kept.empty())
                    return false;
                kept.pop_back();
                continue;
            }
            kept.push_back(parts[i]);
        }
        std::string result = joinSegments(kept);
        bool trailingSlash = decodedPath[decodedPath.size() - 1] == '/';
        if (trailingSlash && result != "/")
            result += "/";
        out = result;
        return true;
    }

    bool makeAbsolute(const std::string& path, std::string& out)
    {
        std::string full = path;
        if (path.empty() || path[0] != '/')
        {
            const char* pwd = std::getenv("PWD");
            if (pwd == NULL || pwd[0] != '/')
                return false;
            full = join(pwd, path);
        }
        std::vector<std::string> parts = StringUtils::split(full, '/');
        std::vector<std::string> kept;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (!parts[i].empty() && parts[i] != ".")
                kept.push_back(parts[i]);
        }
        out = joinSegments(kept);
        return true;
    }

    bool sanitizeFileName(const std::string& rawName, std::string& out)
    {
        size_t cut = rawName.find_last_of("/\\");
        std::string name = (cut == std::string::npos) ? rawName : rawName.substr(cut + 1);
        if (name.empty() || name == "." || name == ".." || name.size() > kMaxFileNameLength)
            return false;
        for (size_t i = 0; i < name.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(name[i]);
            if (c < 0x20 || c == 0x7F)
                return false;
        }
        out = name;
        return true;
    }
}
