#include "http/StatusCodes.hpp"

#include <cstddef>

namespace
{
    struct StatusEntry
    {
        int         code;
        const char* phrase;
    };

    const StatusEntry kStatusTable[] = {
        { 200, "OK" },
        { 201, "Created" },
        { 204, "No Content" },
        { 301, "Moved Permanently" },
        { 302, "Found" },
        { 303, "See Other" },
        { 307, "Temporary Redirect" },
        { 308, "Permanent Redirect" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" },
        { 408, "Request Timeout" },
        { 411, "Length Required" },
        { 413, "Content Too Large" },
        { 414, "URI Too Long" },
        { 415, "Unsupported Media Type" },
        { 431, "Request Header Fields Too Large" },
        { 500, "Internal Server Error" },
        { 501, "Not Implemented" },
        { 502, "Bad Gateway" },
        { 503, "Service Unavailable" },
        { 504, "Gateway Timeout" },
        { 505, "HTTP Version Not Supported" }
    };
}

namespace StatusCodes
{
    const char* reasonPhrase(int code)
    {
        const size_t count = sizeof(kStatusTable) / sizeof(kStatusTable[0]);
        for (size_t i = 0; i < count; ++i)
        {
            if (kStatusTable[i].code == code)
                return kStatusTable[i].phrase;
        }
        return "Unknown";
    }
}
