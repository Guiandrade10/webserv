#include "cgi/CgiOutputParser.hpp"

#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

#include <fstream>
#include <vector>

namespace
{
    bool readStart(const std::string& path, std::string& start)
    {
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (!file)
            return false;
        std::vector<char> buffer(kIoChunkSize);
        file.read(&buffer[0], static_cast<std::streamsize>(buffer.size()));
        start.assign(&buffer[0], static_cast<size_t>(file.gcount()));
        return true;
    }

    bool findHeaderEnd(const std::string& text, size_t& headerEnd, size_t& bodyStart)
    {
        size_t crlf = text.find("\r\n\r\n");
        size_t lf = text.find("\n\n");
        if (crlf == std::string::npos && lf == std::string::npos)
            return false;
        if (lf == std::string::npos || (crlf != std::string::npos && crlf < lf))
        {
            headerEnd = crlf;
            bodyStart = crlf + 4;
        }
        else
        {
            headerEnd = lf;
            bodyStart = lf + 2;
        }
        return true;
    }

    bool parseStatus(const std::string& value, int& status)
    {
        std::vector<std::string> words = StringUtils::splitWhitespace(value);
        return !words.empty() && StringUtils::parseInt(words[0], 100, 599, status);
    }

    bool isDropped(const std::string& lowerName)
    {
        return lowerName == "content-length" || lowerName == "transfer-encoding"
            || lowerName == "connection";
    }

    bool applyHeaders(const std::string& headers, HttpResponse& response, std::string& contentType)
    {
        std::vector<std::string> lines = StringUtils::split(headers, '\n');
        bool hasStatus = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string line = lines[i];
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            size_t colon = line.find(':');
            if (colon == std::string::npos || colon == 0)
                return false;
            std::string name = StringUtils::trim(line.substr(0, colon));
            std::string value = StringUtils::trim(line.substr(colon + 1));
            std::string lower = StringUtils::toLower(name);
            if (name.empty() || name.find(' ') != std::string::npos)
                return false;
            if (lower == "status")
            {
                int status = 0;
                if (!parseStatus(value, status))
                    return false;
                response.setStatus(status);
                hasStatus = true;
            }
            else if (lower == "content-type")
                contentType = value;
            else if (!isDropped(lower))
                response.setHeader(name, value);
        }
        if (!hasStatus && response.hasHeader("Location"))
            response.setStatus(302);
        return true;
    }
}

namespace CgiOutputParser
{
    bool toResponse(const TempFile& output, HttpResponse& response)
    {
        std::string start;
        if (output.size() == 0 || !readStart(output.path(), start))
            return false;
        size_t headerEnd = 0;
        size_t bodyStart = 0;
        if (!findHeaderEnd(start, headerEnd, bodyStart) || headerEnd == 0)
            return false;
        HttpResponse result;
        std::string contentType = "text/html";
        if (!applyHeaders(start.substr(0, headerEnd), result, contentType))
            return false;
        result.setFileBody(output.path(), bodyStart, output.size() - bodyStart, contentType);
        response = result;
        return true;
    }
}
