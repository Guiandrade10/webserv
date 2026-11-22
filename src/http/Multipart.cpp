#include "http/Multipart.hpp"

#include "utils/StringUtils.hpp"

#include <map>

namespace
{
    typedef std::map<std::string, std::string> Parameters;

    std::string parseParameters(const std::string& value, Parameters& parameters)
    {
        std::vector<std::string> items;
        std::string current;
        bool quoted = false;
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] == '"')
                quoted = !quoted;
            if (value[i] == ';' && !quoted)
            {
                items.push_back(current);
                current.clear();
            }
            else
                current += value[i];
        }
        items.push_back(current);
        for (size_t i = 1; i < items.size(); ++i)
        {
            size_t equals = items[i].find('=');
            if (equals == std::string::npos)
                continue;
            std::string key = StringUtils::toLower(StringUtils::trim(items[i].substr(0, equals)));
            std::string data = StringUtils::trim(items[i].substr(equals + 1));
            if (data.size() >= 2 && data[0] == '"' && data[data.size() - 1] == '"')
                data = data.substr(1, data.size() - 2);
            parameters[key] = data;
        }
        return StringUtils::trim(items[0]);
    }

    bool parsePartHeaders(const std::string& headers, MultipartPart& part)
    {
        std::vector<std::string> lines = StringUtils::split(headers, '\n');
        bool hasDisposition = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string line = lines[i];
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            size_t colon = line.find(':');
            if (colon == std::string::npos)
                return false;
            std::string name = StringUtils::toLower(StringUtils::trim(line.substr(0, colon)));
            std::string value = StringUtils::trim(line.substr(colon + 1));
            if (name == "content-type")
                part.contentType = value;
            else if (name == "content-disposition")
            {
                Parameters parameters;
                if (!StringUtils::iequals(parseParameters(value, parameters), "form-data"))
                    return false;
                part.name = parameters["name"];
                part.fileName = parameters["filename"];
                hasDisposition = true;
            }
        }
        return hasDisposition;
    }
}

namespace Multipart
{
    bool extractBoundary(const std::string& contentTypeHeader, std::string& boundary)
    {
        Parameters parameters;
        std::string type = parseParameters(contentTypeHeader, parameters);
        if (!StringUtils::iequals(type, "multipart/form-data"))
            return false;
        Parameters::const_iterator it = parameters.find("boundary");
        if (it == parameters.end() || it->second.empty())
            return false;
        boundary = it->second;
        return true;
    }

    bool parse(const std::string& body, const std::string& boundary,
               std::vector<MultipartPart>& parts)
    {
        const std::string delimiter = "--" + boundary;
        size_t position = body.find(delimiter);
        if (position == std::string::npos)
            return false;
        position += delimiter.size();
        std::vector<MultipartPart> found;
        while (true)
        {
            if (body.compare(position, 2, "--") == 0)
            {
                parts = found;
                return true;
            }
            if (body.compare(position, 2, "\r\n") != 0)
                return false;
            size_t headersStart = position + 2;
            size_t headersEnd = body.find("\r\n\r\n", headersStart);
            if (headersEnd == std::string::npos || headersEnd == headersStart)
                return false;
            MultipartPart part;
            if (!parsePartHeaders(body.substr(headersStart, headersEnd - headersStart), part))
                return false;
            size_t contentStart = headersEnd + 4;
            size_t next = body.find("\r\n" + delimiter, contentStart);
            if (next == std::string::npos)
                return false;
            part.content = body.substr(contentStart, next - contentStart);
            found.push_back(part);
            position = next + 2 + delimiter.size();
        }
    }
}
