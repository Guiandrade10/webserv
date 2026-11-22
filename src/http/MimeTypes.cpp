#include "http/MimeTypes.hpp"

#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

namespace
{
    struct MimeEntry
    {
        const char* extension;
        const char* type;
    };

    const MimeEntry kMimeTable[] = {
        { ".html",  "text/html" },
        { ".htm",   "text/html" },
        { ".css",   "text/css" },
        { ".js",    "text/javascript" },
        { ".json",  "application/json" },
        { ".txt",   "text/plain" },
        { ".xml",   "application/xml" },
        { ".png",   "image/png" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".gif",   "image/gif" },
        { ".svg",   "image/svg+xml" },
        { ".ico",   "image/x-icon" },
        { ".webp",  "image/webp" },
        { ".pdf",   "application/pdf" },
        { ".zip",   "application/zip" },
        { ".mp3",   "audio/mpeg" },
        { ".mp4",   "video/mp4" },
        { ".woff",  "font/woff" },
        { ".woff2", "font/woff2" },
        { ".ttf",   "font/ttf" }
    };

    const char* const kDefaultMimeType = "application/octet-stream";
}

namespace MimeTypes
{
    std::string forPath(const std::string& path)
    {
        std::string extension = StringUtils::toLower(PathUtils::extension(path));
        const size_t count = sizeof(kMimeTable) / sizeof(kMimeTable[0]);
        for (size_t i = 0; i < count; ++i)
        {
            if (extension == kMimeTable[i].extension)
                return kMimeTable[i].type;
        }
        return kDefaultMimeType;
    }
}
