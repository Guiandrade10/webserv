#ifndef WEBSERV_CONFIG_LOCATIONCONFIG_HPP
#define WEBSERV_CONFIG_LOCATIONCONFIG_HPP

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

class LocationConfig
{
public:
    LocationConfig();

    const std::string& path() const;
    const std::string& root() const;
    const std::vector<std::string>& indexFiles() const;
    bool autoindex() const;
    size_t clientMaxBodySize() const;
    const std::set<std::string>& allowedMethods() const;
    bool allowsMethod(const std::string& method) const;
    bool hasRedirect() const;
    int redirectCode() const;
    const std::string& redirectTarget() const;
    bool uploadsEnabled() const;
    const std::string& uploadStore() const;
    const std::string* findCgiInterpreter(const std::string& extension) const;
    const std::map<std::string, std::string>& cgiHandlers() const;
    std::string describe() const;

    void setPath(const std::string& path);
    void setRoot(const std::string& root);
    void setIndexFiles(const std::vector<std::string>& files);
    void setAutoindex(bool enabled);
    void setClientMaxBodySize(size_t size);
    void setAllowedMethods(const std::set<std::string>& methods);
    void setRedirect(int code, const std::string& target);
    void setUploadStore(const std::string& directory);
    bool addCgiHandler(const std::string& extension, const std::string& interpreter);
    bool hasRoot() const;
    bool hasIndexFiles() const;
    bool hasAutoindex() const;
    bool hasClientMaxBodySize() const;

private:
    std::string                        _path;
    std::string                        _root;
    std::vector<std::string>           _indexFiles;
    bool                               _autoindex;
    size_t                             _clientMaxBodySize;
    std::set<std::string>              _allowedMethods;
    int                                _redirectCode;
    std::string                        _redirectTarget;
    std::string                        _uploadStore;
    std::map<std::string, std::string> _cgiHandlers;
    bool                               _hasRoot;
    bool                               _hasIndexFiles;
    bool                               _hasAutoindex;
    bool                               _hasClientMaxBodySize;
};

#endif
