#ifndef WEBSERV_CORE_CLIENT_HPP
#define WEBSERV_CORE_CLIENT_HPP

#include "cgi/CgiProcess.hpp"
#include "core/Listener.hpp"
#include "handlers/RequestHandler.hpp"
#include "handlers/Router.hpp"
#include "http/HttpResponse.hpp"
#include "http/RequestParser.hpp"

#include <ctime>
#include <fstream>
#include <string>

class Client
{
public:
    enum State { READING_REQUEST, WAITING_FOR_CGI, SENDING_RESPONSE, LINGERING, TO_CLOSE };

    Client(int fd, const Listener& listener, const std::string& remoteIp, int remotePort, time_t now);
    ~Client();
    int   fd() const;
    State state() const;
    short socketEvents() const;
    void  onSocketEvent(short revents, time_t now);
    void  checkTimeout(time_t now);
    bool  isFinished() const;
    void  forceClose();
    int   highestFd() const;

    bool  needsCgiStart() const;
    void  startCgi(int highestFd, time_t now);
    int   cgiOutputFd() const;
    void  onCgiEvent(short revents, time_t now);
    CgiProcess* releaseCgi();

private:
    Client(const Client&);
    Client& operator=(const Client&);

    void readOnce(time_t now);
    void writeOnce(time_t now);
    void processInput(time_t now);
    void handleCompleteRequest(time_t now);
    void finishCgi(CgiProcess::ReadStatus status, time_t now);
    void retireCgi();
    ConnectionInfo connectionInfo() const;
    void queueError(int status, time_t now);
    void startResponse(HttpResponse response, bool keepAlive, time_t now);
    bool openFileBody(const HttpResponse& response);
    bool refillFromFile();
    void closeFileBody();
    void onResponseSent(time_t now);
    void logAccess() const;
    void startLingering(time_t now);
    void setState(State state, time_t now);
    void finish();

    int             _fd;
    const Listener& _listener;
    std::string     _remoteIp;
    int             _remotePort;
    State           _state;
    RequestParser   _parser;
    Route           _route;
    std::string     _inBuffer;
    std::string     _outBuffer;
    size_t          _outOffset;
    std::ifstream   _file;
    size_t          _fileRemaining;
    CgiJob          _cgiJob;
    bool            _cgiPending;
    CgiProcess*     _cgi;
    CgiProcess*     _retiredCgi;
    int             _responseStatus;
    size_t          _responseBytes;
    bool            _keepAlive;
    time_t          _lastActivity;
    time_t          _stateSince;
};

#endif
