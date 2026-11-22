#ifndef WEBSERV_CORE_EVENTLOOP_HPP
#define WEBSERV_CORE_EVENTLOOP_HPP

#include "cgi/CgiProcess.hpp"
#include "config/ServerConfig.hpp"
#include "core/Client.hpp"
#include "core/Listener.hpp"

#include <ctime>
#include <map>
#include <poll.h>
#include <vector>

class EventLoop
{
public:
    explicit EventLoop(const std::vector<ServerConfig>& servers);
    ~EventLoop();
    void run();

private:
    EventLoop(const EventLoop&);
    EventLoop& operator=(const EventLoop&);

    enum FdRole { ROLE_LISTENER, ROLE_CLIENT, ROLE_CGI_OUTPUT };
    struct FdOwner
    {
        FdRole    role;
        Listener* listener;
        Client*   client;
    };

    void openListeners();
    void deleteAll();
    void runOnce();
    void buildPollSet();
    void addPollEntry(int fd, short events, const FdOwner& owner);
    void dispatch(const struct pollfd& entry, const FdOwner& owner, time_t now);
    void acceptClient(Listener& listener, time_t now);
    void startPendingCgis(time_t now);
    int  highestFd() const;
    void checkTimeouts(time_t now);
    void collectRetiredCgis();
    void collectFrom(Client& client);
    void reapRetiredCgis();
    void closeFinishedClients();

    std::vector<ServerConfig>   _servers;
    std::vector<Listener*>      _listeners;
    std::map<int, Client*>      _clients;
    std::vector<CgiProcess*>    _retiredCgis;
    std::vector<struct pollfd>  _pollFds;
    std::vector<FdOwner>        _owners;
};

#endif
