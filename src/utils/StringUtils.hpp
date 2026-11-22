#ifndef WEBSERV_UTILS_STRINGUTILS_HPP
#define WEBSERV_UTILS_STRINGUTILS_HPP

#include <sstream>
#include <string>
#include <vector>

namespace StringUtils
{
    std::string toLower(const std::string& text);
    std::string trim(const std::string& text);
    std::vector<std::string> split(const std::string& text, char delimiter);
    std::vector<std::string> splitWhitespace(const std::string& text);
    bool startsWith(const std::string& text, const std::string& prefix);
    bool endsWith(const std::string& text, const std::string& suffix);
    bool iequals(const std::string& a, const std::string& b);

    template <typename T>
    std::string toString(const T& value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    bool parseUnsigned(const std::string& text, size_t& out);
    bool parseInt(const std::string& text, int min, int max, int& out);
    bool parseSize(const std::string& text, size_t& out);
    bool parseHex(const std::string& text, size_t& out);
    bool urlDecode(const std::string& in, std::string& out);
    std::string urlEncodePathSegment(const std::string& segment);
    std::string htmlEscape(const std::string& text);
}

#endif
