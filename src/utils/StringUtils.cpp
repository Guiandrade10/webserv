#include "utils/StringUtils.hpp"

#include <cctype>
#include <limits>

namespace
{
    bool isBlank(char c)
    {
        return c == ' ' || c == '\t';
    }

    int hexDigitValue(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    }

    bool appendDigit(size_t& out, size_t base, size_t digit)
    {
        const size_t maxValue = std::numeric_limits<size_t>::max();
        if (out > (maxValue - digit) / base)
            return false;
        out = out * base + digit;
        return true;
    }
}

namespace StringUtils
{
    std::string toLower(const std::string& text)
    {
        std::string result(text);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
        return result;
    }

    std::string trim(const std::string& text)
    {
        size_t begin = 0;
        while (begin < text.size() && isBlank(text[begin]))
            ++begin;
        size_t end = text.size();
        while (end > begin && isBlank(text[end - 1]))
            --end;
        return text.substr(begin, end - begin);
    }

    std::vector<std::string> split(const std::string& text, char delimiter)
    {
        std::vector<std::string> fields;
        size_t start = 0;
        while (true)
        {
            size_t found = text.find(delimiter, start);
            if (found == std::string::npos)
            {
                fields.push_back(text.substr(start));
                return fields;
            }
            fields.push_back(text.substr(start, found - start));
            start = found + 1;
        }
    }

    std::vector<std::string> splitWhitespace(const std::string& text)
    {
        std::vector<std::string> words;
        std::string current;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (std::isspace(static_cast<unsigned char>(text[i])))
            {
                if (!current.empty())
                    words.push_back(current);
                current.clear();
            }
            else
                current += text[i];
        }
        if (!current.empty())
            words.push_back(current);
        return words;
    }

    bool startsWith(const std::string& text, const std::string& prefix)
    {
        return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
    }

    bool endsWith(const std::string& text, const std::string& suffix)
    {
        return text.size() >= suffix.size()
            && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool iequals(const std::string& a, const std::string& b)
    {
        return a.size() == b.size() && toLower(a) == toLower(b);
    }

    bool parseUnsigned(const std::string& text, size_t& out)
    {
        if (text.empty())
            return false;
        size_t value = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(text[i])))
                return false;
            if (!appendDigit(value, 10, static_cast<size_t>(text[i] - '0')))
                return false;
        }
        out = value;
        return true;
    }

    bool parseInt(const std::string& text, int min, int max, int& out)
    {
        bool negative = !text.empty() && text[0] == '-';
        size_t magnitude = 0;
        if (!parseUnsigned(negative ? text.substr(1) : text, magnitude))
            return false;
        if (magnitude > static_cast<size_t>(std::numeric_limits<int>::max()) + 1)
            return false;
        long value = negative ? -static_cast<long>(magnitude) : static_cast<long>(magnitude);
        if (value < min || value > max)
            return false;
        out = static_cast<int>(value);
        return true;
    }

    bool parseSize(const std::string& text, size_t& out)
    {
        if (text.empty())
            return false;
        size_t multiplier = 1;
        std::string digits = text;
        char last = static_cast<char>(std::tolower(static_cast<unsigned char>(text[text.size() - 1])));
        if (last == 'k' || last == 'm' || last == 'g')
        {
            multiplier = (last == 'k') ? 1024 : (last == 'm') ? 1024 * 1024 : 1024 * 1024 * 1024;
            digits = text.substr(0, text.size() - 1);
        }
        size_t value = 0;
        if (!parseUnsigned(digits, value))
            return false;
        if (value > std::numeric_limits<size_t>::max() / multiplier)
            return false;
        out = value * multiplier;
        return true;
    }

    bool parseHex(const std::string& text, size_t& out)
    {
        if (text.empty())
            return false;
        size_t value = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            int digit = hexDigitValue(text[i]);
            if (digit < 0 || !appendDigit(value, 16, static_cast<size_t>(digit)))
                return false;
        }
        out = value;
        return true;
    }

    bool urlDecode(const std::string& in, std::string& out)
    {
        std::string result;
        for (size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] != '%')
            {
                result += in[i];
                continue;
            }
            if (i + 2 >= in.size())
                return false;
            int high = hexDigitValue(in[i + 1]);
            int low = hexDigitValue(in[i + 2]);
            if (high < 0 || low < 0)
                return false;
            char decoded = static_cast<char>(high * 16 + low);
            if (decoded == '\0')
                return false;
            result += decoded;
            i += 2;
        }
        out = result;
        return true;
    }

    std::string urlEncodePathSegment(const std::string& segment)
    {
        const char* hex = "0123456789ABCDEF";
        std::string result;
        for (size_t i = 0; i < segment.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(segment[i]);
            if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
                result += static_cast<char>(c);
            else
            {
                result += '%';
                result += hex[c >> 4];
                result += hex[c & 0x0F];
            }
        }
        return result;
    }

    std::string htmlEscape(const std::string& text)
    {
        std::string result;
        for (size_t i = 0; i < text.size(); ++i)
        {
            switch (text[i])
            {
                case '&': result += "&amp;"; break;
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += text[i];
            }
        }
        return result;
    }
}
