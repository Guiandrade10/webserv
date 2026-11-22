#ifndef WEBSERV_CORE_LISTENER_HPP
#define WEBSERV_CORE_LISTENER_HPP

#include "config/ServerConfig.hpp"

#include <string>
#include <vector>

class Listener
{
public:
    explicit Listener(const ListenAddress& address);
    ~Listener();
    void open();
    int fd() const;
    const ListenAddress& address() const;
    void addServer(const ServerConfig* server);
    const std::vector<const ServerConfig*>& servers() const;
    const ServerConfig* defaultServer() const;
    int acceptClient(std::string& remoteIp, int& remotePort);
private:
    Listener(const Listener&);
    Listener& operator=(const Listener&);
    void bindAndListen(const struct addrinfo* info);

    ListenAddress                    _address;
    int                              _fd;
    std::vector<const ServerConfig*> _servers;
};

#endif
