#include "core/EventLoop.hpp"

#include "core/Signals.hpp"
#include "utils/Constants.hpp"
#include "utils/Logger.hpp"

#include <algorithm>
#include <exception>
#include <unistd.h>

EventLoop::EventLoop(const std::vector<ServerConfig>& servers)
    : _servers(servers), _listeners(), _clients(), _retiredCgis(), _pollFds(), _owners()
{
    try
    {
        openListeners();
    }
    catch (...)
    {
        deleteAll();
        throw;
    }
}

EventLoop::~EventLoop()
{
    deleteAll();
}

void EventLoop::openListeners()
{
    std::map<std::string, Listener*> byAddress;
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const std::vector<ListenAddress>& listens = _servers[i].listens();
        for (size_t j = 0; j < listens.size(); ++j)
        {
            std::string key = listens[j].toString();
            if (byAddress.find(key) == byAddress.end())
            {
                _listeners.push_back(NULL);
                _listeners.back() = new Listener(listens[j]);
                Listener* listener = _listeners.back();
                listener->open();
                byAddress[key] = listener;
                Logger::info("listening on " + key);
            }
            byAddress[key]->addServer(&_servers[i]);
        }
    }
}

void EventLoop::deleteAll()
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        it->second->forceClose();
        collectFrom(*it->second);
        delete it->second;
    }
    _clients.clear();
    for (size_t i = 0; i < _retiredCgis.size(); ++i)
    {
        _retiredCgis[i]->terminate();
        _retiredCgis[i]->waitBlocking();
        delete _retiredCgis[i];
    }
    _retiredCgis.clear();
    for (size_t i = 0; i < _listeners.size(); ++i)
        delete _listeners[i];
    _listeners.clear();
}

void EventLoop::run()
{
    Logger::info("webserv started");
    while (!Signals::stopRequested())
    {
        try
        {
            runOnce();
        }
        catch (const std::exception& error)
        {
            Logger::error(std::string("event loop: ") + error.what());
        }
    }
    Logger::info("shutting down");
}

void EventLoop::runOnce()
{
    buildPollSet();
    int ready = poll(&_pollFds[0], _pollFds.size(), kPollTimeoutMs);
    time_t now = std::time(NULL);
    if (ready > 0)
    {
        for (size_t i = 0; i < _pollFds.size(); ++i)
        {
            if (_pollFds[i].revents != 0)
                dispatch(_pollFds[i], _owners[i], now);
        }
    }
    startPendingCgis(now);
    checkTimeouts(now);
    collectRetiredCgis();
    reapRetiredCgis();
    closeFinishedClients();
}

void EventLoop::buildPollSet()
{
    _pollFds.clear();
    _owners.clear();
    short listenEvents = (_clients.size() < kMaxClients) ? POLLIN : 0;
    for (size_t i = 0; i < _listeners.size(); ++i)
    {
        FdOwner owner = { ROLE_LISTENER, _listeners[i], NULL };
        addPollEntry(_listeners[i]->fd(), listenEvents, owner);
    }
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->isFinished())
            continue;
        FdOwner owner = { ROLE_CLIENT, NULL, it->second };
        addPollEntry(it->first, it->second->socketEvents(), owner);
        int cgiFd = it->second->cgiOutputFd();
        if (cgiFd >= 0)
        {
            FdOwner cgiOwner = { ROLE_CGI_OUTPUT, NULL, it->second };
            addPollEntry(cgiFd, POLLIN, cgiOwner);
        }
    }
}

void EventLoop::addPollEntry(int fd, short events, const FdOwner& owner)
{
    struct pollfd entry;
    entry.fd = fd;
    entry.events = events;
    entry.revents = 0;
    _pollFds.push_back(entry);
    _owners.push_back(owner);
}

void EventLoop::dispatch(const struct pollfd& entry, const FdOwner& owner, time_t now)
{
    try
    {
        if (owner.role == ROLE_LISTENER)
        {
            if (entry.revents & POLLIN)
                acceptClient(*owner.listener, now);
        }
        else if (owner.role == ROLE_CLIENT)
            owner.client->onSocketEvent(entry.revents, now);
        else
            owner.client->onCgiEvent(entry.revents, now);
    }
    catch (const std::exception& error)
    {
        Logger::error(std::string("client error: ") + error.what());
        if (owner.client != NULL)
            owner.client->forceClose();
    }
}

void EventLoop::acceptClient(Listener& listener, time_t now)
{
    std::string remoteIp;
    int remotePort = 0;
    int fd = listener.acceptClient(remoteIp, remotePort);
    if (fd < 0)
        return;
    Client* client = NULL;
    try
    {
        client = new Client(fd, listener, remoteIp, remotePort, now);
        _clients[fd] = client;
    }
    catch (...)
    {
        if (client != NULL)
            delete client;
        else
            close(fd);
        throw;
    }
}

void EventLoop::startPendingCgis(time_t now)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (!it->second->needsCgiStart())
            continue;
        try
        {
            it->second->startCgi(highestFd(), now);
        }
        catch (const std::exception& error)
        {
            Logger::error(std::string("cgi start: ") + error.what());
            it->second->forceClose();
        }
    }
}

int EventLoop::highestFd() const
{
    int highest = 2;
    for (size_t i = 0; i < _listeners.size(); ++i)
        highest = std::max(highest, _listeners[i]->fd());
    for (std::map<int, Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
        highest = std::max(highest, it->second->highestFd());
    for (size_t i = 0; i < _retiredCgis.size(); ++i)
        highest = std::max(highest, _retiredCgis[i]->outputFd());
    return highest;
}

void EventLoop::collectRetiredCgis()
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        collectFrom(*it->second);
}

void EventLoop::collectFrom(Client& client)
{
    while (true)
    {
        _retiredCgis.push_back(NULL);
        CgiProcess* process = client.releaseCgi();
        if (process == NULL)
        {
            _retiredCgis.pop_back();
            return;
        }
        process->terminate();
        _retiredCgis.back() = process;
    }
}

void EventLoop::reapRetiredCgis()
{
    size_t i = 0;
    while (i < _retiredCgis.size())
    {
        if (_retiredCgis[i]->tryReap())
        {
            delete _retiredCgis[i];
            _retiredCgis.erase(_retiredCgis.begin() + i);
        }
        else
            ++i;
    }
}

void EventLoop::checkTimeouts(time_t now)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        it->second->checkTimeout(now);
}

void EventLoop::closeFinishedClients()
{
    std::map<int, Client*>::iterator it = _clients.begin();
    while (it != _clients.end())
    {
        if (it->second->isFinished())
        {
            delete it->second;
            _clients.erase(it++);
        }
        else
            ++it;
    }
}
