#include "config/ServerConfig.hpp"

#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

std::string ListenAddress::toString() const
{
    return host + ":" + StringUtils::toString(port);
}

ServerConfig::ServerConfig()
    : _listens(), _serverNames(), _root(), _indexFiles(1, "index.html"), _autoindex(false),
      _clientMaxBodySize(kDefaultClientMaxBodySize), _errorPages(), _locations()
{
}

const std::vector<ListenAddress>& ServerConfig::listens() const { return _listens; }
const std::vector<std::string>& ServerConfig::serverNames() const { return _serverNames; }
const std::string& ServerConfig::root() const { return _root; }
const std::vector<std::string>& ServerConfig::indexFiles() const { return _indexFiles; }
bool ServerConfig::autoindex() const { return _autoindex; }
size_t ServerConfig::clientMaxBodySize() const { return _clientMaxBodySize; }
const std::map<int, std::string>& ServerConfig::errorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::locations() const { return _locations; }

bool ServerConfig::hasServerName(const std::string& name) const
{
    for (size_t i = 0; i < _serverNames.size(); ++i)
    {
        if (StringUtils::iequals(_serverNames[i], name))
            return true;
    }
    return false;
}

std::string ServerConfig::describe() const
{
    std::string text = "server";
    for (size_t i = 0; i < _listens.size(); ++i)
        text += " " + _listens[i].toString();
    text += " names=[";
    for (size_t i = 0; i < _serverNames.size(); ++i)
        text += (i > 0 ? " " : "") + _serverNames[i];
    text += "] root=" + _root + " error_pages=" + StringUtils::toString(_errorPages.size())
        + " locations=" + StringUtils::toString(_locations.size());
    return text;
}

bool ServerConfig::addListen(const ListenAddress& address)
{
    for (size_t i = 0; i < _listens.size(); ++i)
    {
        if (_listens[i].toString() == address.toString())
            return false;
    }
    _listens.push_back(address);
    return true;
}

void ServerConfig::setServerNames(const std::vector<std::string>& names) { _serverNames = names; }
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setIndexFiles(const std::vector<std::string>& files) { _indexFiles = files; }
void ServerConfig::setAutoindex(bool enabled) { _autoindex = enabled; }
void ServerConfig::setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
void ServerConfig::addLocation(const LocationConfig& location) { _locations.push_back(location); }

bool ServerConfig::addErrorPage(int code, const std::string& path)
{
    if (_errorPages.find(code) != _errorPages.end())
        return false;
    _errorPages[code] = path;
    return true;
}
