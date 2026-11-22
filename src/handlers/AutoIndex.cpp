#include "handlers/AutoIndex.hpp"

#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

#include <vector>

namespace
{
    std::string encodePath(const std::string& path)
    {
        std::vector<std::string> segments = StringUtils::split(path, '/');
        std::string result;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if (i > 0)
                result += "/";
            result += StringUtils::urlEncodePathSegment(segments[i]);
        }
        return result;
    }

    std::string listItem(const std::string& href, const std::string& text)
    {
        return "<li><a href=\"" + StringUtils::htmlEscape(href) + "\">"
            + StringUtils::htmlEscape(text) + "</a></li>\n";
    }

    std::string parentUri(const std::string& base)
    {
        std::string parent = PathUtils::parentDirectory(base.substr(0, base.size() - 1));
        return (parent == "/") ? parent : parent + "/";
    }
}

namespace AutoIndex
{
    bool render(const std::string& directory, const std::string& uriPath, std::string& html)
    {
        std::vector<std::string> names;
        if (!FileUtils::listDirectory(directory, names))
            return false;
        std::string base = StringUtils::endsWith(uriPath, "/") ? uriPath : uriPath + "/";
        std::string title = "Index of " + StringUtils::htmlEscape(base);
        html = "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>" + title
             + "</title></head>\n<body>\n<h1>" + title + "</h1>\n<hr>\n<ul>\n";
        if (base != "/")
            html += listItem(encodePath(parentUri(base)), "../");
        for (size_t i = 0; i < names.size(); ++i)
        {
            bool isDirectory = FileUtils::isDirectory(PathUtils::join(directory, names[i]));
            std::string suffix = isDirectory ? "/" : "";
            html += listItem(encodePath(base) + StringUtils::urlEncodePathSegment(names[i]) + suffix,
                             names[i] + suffix);
        }
        html += "</ul>\n<hr>\n</body></html>\n";
        return true;
    }
}
