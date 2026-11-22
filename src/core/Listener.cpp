#include "core/Listener.hpp"

#include "utils/Constants.hpp"
#include "utils/StringUtils.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
    std::string ipToString(unsigned long networkOrder)
    {
        unsigned long ip = ntohl(networkOrder);
        std::ostringstream text;
        text << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
             << ((ip >> 8) & 0xFF) << "." << (ip & 0xFF);
        return text.str();
    }
}

Listener::Listener(const ListenAddress& address) : _address(address), _fd(-1), _servers()
{
}

Listener::~Listener()
{
    if (_fd >= 0)
        close(_fd);
}

void Listener::open()
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* info = NULL;
    std::string port = StringUtils::toString(_address.port);
    int status = getaddrinfo(_address.host.c_str(), port.c_str(), &hints, &info);
    if (status != 0)
        throw std::runtime_error("getaddrinfo " + _address.toString() + ": " + gai_strerror(status));
    try
    {
        bindAndListen(info);
    }
    catch (...)
    {
        freeaddrinfo(info);
        throw;
    }
    freeaddrinfo(info);
}

void Listener::bindAndListen(const struct addrinfo* info)
{
    const std::string where = _address.toString();
    _fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (_fd < 0)
        throw std::runtime_error("socket " + where + ": " + std::strerror(errno));
    int enable = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        throw std::runtime_error("setsockopt " + where + ": " + std::strerror(errno));
    if (bind(_fd, info->ai_addr, info->ai_addrlen) < 0)
        throw std::runtime_error("bind " + where + ": " + std::strerror(errno));
    if (listen(_fd, kListenBacklog) < 0)
        throw std::runtime_error("listen " + where + ": " + std::strerror(errno));
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl " + where + ": " + std::strerror(errno));
}

int Listener::fd() const { return _fd; }
const ListenAddress& Listener::address() const { return _address; }
void Listener::addServer(const ServerConfig* server) { _servers.push_back(server); }
const std::vector<const ServerConfig*>& Listener::servers() const { return _servers; }

const ServerConfig* Listener::defaultServer() const
{
    return _servers.empty() ? NULL : _servers[0];
}

int Listener::acceptClient(std::string& remoteIp, int& remotePort)
{
    struct sockaddr_in peer;
    socklen_t length = sizeof(peer);
    std::memset(&peer, 0, sizeof(peer));
    int clientFd = accept(_fd, reinterpret_cast<struct sockaddr*>(&peer), &length);
    if (clientFd < 0)
        return -1;
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(clientFd);
        return -1;
    }
    remoteIp = ipToString(peer.sin_addr.s_addr);
    remotePort = ntohs(peer.sin_port);
    return clientFd;
}
