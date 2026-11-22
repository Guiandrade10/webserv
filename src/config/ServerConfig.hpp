#ifndef WEBSERV_CONFIG_SERVERCONFIG_HPP
#define WEBSERV_CONFIG_SERVERCONFIG_HPP

#include "config/LocationConfig.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

struct ListenAddress
{
    std::string host;
    int         port;
    std::string toString() const;
};

class ServerConfig
{
public:
    ServerConfig();

    const std::vector<ListenAddress>& listens() const;
    const std::vector<std::string>& serverNames() const;
    bool hasServerName(const std::string& name) const;
    const std::string& root() const;
    const std::vector<std::string>& indexFiles() const;
    bool autoindex() const;
    size_t clientMaxBodySize() const;
    const std::map<int, std::string>& errorPages() const;
    const std::vector<LocationConfig>& locations() const;
    std::string describe() const;

    bool addListen(const ListenAddress& address);
    void setServerNames(const std::vector<std::string>& names);
    void setRoot(const std::string& root);
    void setIndexFiles(const std::vector<std::string>& files);
    void setAutoindex(bool enabled);
    void setClientMaxBodySize(size_t size);
    bool addErrorPage(int code, const std::string& path);
    void addLocation(const LocationConfig& location);

private:
    std::vector<ListenAddress>  _listens;
    std::vector<std::string>    _serverNames;
    std::string                 _root;
    std::vector<std::string>    _indexFiles;
    bool                        _autoindex;
    size_t                      _clientMaxBodySize;
    std::map<int, std::string>  _errorPages;
    std::vector<LocationConfig> _locations;
};

#endif
