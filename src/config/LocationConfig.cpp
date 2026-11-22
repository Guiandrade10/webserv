#include "config/LocationConfig.hpp"

#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

namespace
{
    template <typename Container>
    std::string joinItems(const Container& items, const std::string& separator)
    {
        std::string result;
        for (typename Container::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (!result.empty())
                result += separator;
            result += *it;
        }
        return result;
    }
}

LocationConfig::LocationConfig()
    : _path("/"), _root(), _indexFiles(1, "index.html"), _autoindex(false),
      _clientMaxBodySize(kDefaultClientMaxBodySize), _allowedMethods(), _redirectCode(0),
      _redirectTarget(), _uploadStore(), _cgiHandlers(), _hasRoot(false),
      _hasIndexFiles(false), _hasAutoindex(false), _hasClientMaxBodySize(false)
{
    _allowedMethods.insert("GET");
}

const std::string& LocationConfig::path() const { return _path; }
const std::string& LocationConfig::root() const { return _root; }
const std::vector<std::string>& LocationConfig::indexFiles() const { return _indexFiles; }
bool LocationConfig::autoindex() const { return _autoindex; }
size_t LocationConfig::clientMaxBodySize() const { return _clientMaxBodySize; }
const std::set<std::string>& LocationConfig::allowedMethods() const { return _allowedMethods; }
bool LocationConfig::hasRedirect() const { return _redirectCode != 0; }
int LocationConfig::redirectCode() const { return _redirectCode; }
const std::string& LocationConfig::redirectTarget() const { return _redirectTarget; }
bool LocationConfig::uploadsEnabled() const { return !_uploadStore.empty(); }
const std::string& LocationConfig::uploadStore() const { return _uploadStore; }
const std::map<std::string, std::string>& LocationConfig::cgiHandlers() const { return _cgiHandlers; }
bool LocationConfig::hasRoot() const { return _hasRoot; }
bool LocationConfig::hasIndexFiles() const { return _hasIndexFiles; }
bool LocationConfig::hasAutoindex() const { return _hasAutoindex; }
bool LocationConfig::hasClientMaxBodySize() const { return _hasClientMaxBodySize; }

bool LocationConfig::allowsMethod(const std::string& method) const
{
    return _allowedMethods.find(method) != _allowedMethods.end();
}

const std::string* LocationConfig::findCgiInterpreter(const std::string& extension) const
{
    std::map<std::string, std::string>::const_iterator it = _cgiHandlers.find(extension);
    if (it == _cgiHandlers.end())
        return NULL;
    return &it->second;
}

std::string LocationConfig::describe() const
{
    std::string text = "location " + _path + " root=" + _root
        + " methods=" + joinItems(_allowedMethods, ",")
        + " index=" + joinItems(_indexFiles, ",")
        + " autoindex=" + (_autoindex ? "on" : "off")
        + " max_body=" + StringUtils::toString(_clientMaxBodySize);
    if (uploadsEnabled())
        text += " upload_store=" + _uploadStore;
    for (std::map<std::string, std::string>::const_iterator it = _cgiHandlers.begin();
         it != _cgiHandlers.end(); ++it)
        text += " cgi=" + it->first + ":" + it->second;
    if (hasRedirect())
        text += " return=" + StringUtils::toString(_redirectCode) + " " + _redirectTarget;
    return text;
}

void LocationConfig::setPath(const std::string& path) { _path = path; }

void LocationConfig::setRoot(const std::string& root)
{
    _root = root;
    _hasRoot = true;
}

void LocationConfig::setIndexFiles(const std::vector<std::string>& files)
{
    _indexFiles = files;
    _hasIndexFiles = true;
}

void LocationConfig::setAutoindex(bool enabled)
{
    _autoindex = enabled;
    _hasAutoindex = true;
}

void LocationConfig::setClientMaxBodySize(size_t size)
{
    _clientMaxBodySize = size;
    _hasClientMaxBodySize = true;
}

void LocationConfig::setAllowedMethods(const std::set<std::string>& methods)
{
    _allowedMethods = methods;
}

void LocationConfig::setRedirect(int code, const std::string& target)
{
    _redirectCode = code;
    _redirectTarget = target;
}

void LocationConfig::setUploadStore(const std::string& directory) { _uploadStore = directory; }

bool LocationConfig::addCgiHandler(const std::string& extension, const std::string& interpreter)
{
    if (_cgiHandlers.find(extension) != _cgiHandlers.end())
        return false;
    _cgiHandlers[extension] = interpreter;
    return true;
}
