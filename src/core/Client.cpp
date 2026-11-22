#include "core/Client.hpp"

#include "cgi/CgiOutputParser.hpp"
#include "handlers/ErrorPages.hpp"
#include "handlers/RequestHandler.hpp"
#include "utils/Constants.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

#include <exception>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

Client::Client(int fd, const Listener& listener, const std::string& remoteIp, int remotePort,
               time_t now)
    : _fd(fd), _listener(listener), _remoteIp(remoteIp), _remotePort(remotePort),
      _state(READING_REQUEST), _parser(), _route(), _inBuffer(), _outBuffer(), _outOffset(0),
      _file(), _fileRemaining(0), _cgiJob(), _cgiPending(false), _cgi(NULL), _retiredCgi(NULL),
      _responseStatus(0), _responseBytes(0), _keepAlive(true),
      _lastActivity(now), _stateSince(now)
{
}

Client::~Client()
{
    CgiProcess* processes[2] = { _cgi, _retiredCgi };
    for (int i = 0; i < 2; ++i)
    {
        if (processes[i] != NULL)
        {
            processes[i]->terminate();
            processes[i]->waitBlocking();
            delete processes[i];
        }
    }
    close(_fd);
}

int Client::fd() const { return _fd; }
Client::State Client::state() const { return _state; }
bool Client::isFinished() const { return _state == TO_CLOSE; }
void Client::forceClose() { finish(); }

short Client::socketEvents() const
{
    switch (_state)
    {
        case READING_REQUEST:
        case LINGERING:
            return POLLIN;
        case SENDING_RESPONSE:
            return POLLOUT;
        case WAITING_FOR_CGI:
            return (_inBuffer.size() < kCgiWaitInputLimit) ? POLLIN : 0;
        case TO_CLOSE:
            return 0;
    }
    return 0;
}

void Client::onSocketEvent(short revents, time_t now)
{
    const short failure = POLLHUP | POLLERR | POLLNVAL;
    switch (_state)
    {
        case READING_REQUEST:
        case LINGERING:
        case WAITING_FOR_CGI:
            if (revents & (POLLIN | failure))
                readOnce(now);
            break;
        case SENDING_RESPONSE:
            if (revents & POLLOUT)
                writeOnce(now);
            else if (revents & failure)
                finish();
            break;
        case TO_CLOSE:
            break;
    }
}

void Client::readOnce(time_t now)
{
    char buffer[kIoChunkSize];
    ssize_t received = recv(_fd, buffer, sizeof(buffer), 0);
    if (received < 0)
    {
        finish();
        return;
    }
    if (received == 0)
    {
        finish();
        return;
    }
    _lastActivity = now;
    if (_state == LINGERING)
        return;
    _inBuffer.append(buffer, static_cast<size_t>(received));
    processInput(now);
}

void Client::writeOnce(time_t now)
{
    if (_outOffset == _outBuffer.size() && !refillFromFile())
    {
        finish();
        return;
    }
    size_t remaining = _outBuffer.size() - _outOffset;
    ssize_t sent = send(_fd, _outBuffer.c_str() + _outOffset, remaining, 0);
    if (sent <= 0)
    {
        finish();
        return;
    }
    _lastActivity = now;
    _outOffset += static_cast<size_t>(sent);
    if (_outOffset == _outBuffer.size() && _fileRemaining == 0)
        onResponseSent(now);
}

void Client::processInput(time_t now)
{
    if (_state != READING_REQUEST)
        return;
    try
    {
        while (true)
        {
            RequestParser::Status status = _parser.parse(_inBuffer);
            if (status == RequestParser::NEED_MORE)
                return;
            if (status == RequestParser::HEADERS_READY)
            {
                _route = Router::resolve(_listener.servers(), _parser.request());
                _parser.setMaxBodySize(_route.location->clientMaxBodySize());
                continue;
            }
            if (status == RequestParser::ERROR)
                queueError(_parser.errorStatus(), now);
            else
                handleCompleteRequest(now);
            return;
        }
    }
    catch (const std::exception& error)
    {
        Logger::error(std::string("request failed: ") + error.what());
        queueError(500, now);
    }
}

void Client::handleCompleteRequest(time_t now)
{
    const HttpRequest& request = _parser.request();
    HandlerResult result = RequestHandler::handle(request, _route, connectionInfo());
    if (result.isCgi())
    {
        _cgiJob = result.cgiJob();
        _cgiPending = true;
        setState(WAITING_FOR_CGI, now);
        return;
    }
    startResponse(result.response(), request.keepAlive(), now);
}

int Client::highestFd() const
{
    int highest = _fd;
    if (_cgi != NULL && _cgi->outputFd() > highest)
        highest = _cgi->outputFd();
    if (_retiredCgi != NULL && _retiredCgi->outputFd() > highest)
        highest = _retiredCgi->outputFd();
    return highest;
}

bool Client::needsCgiStart() const
{
    return _cgiPending && _state == WAITING_FOR_CGI;
}

void Client::startCgi(int highestFd, time_t now)
{
    _cgiPending = false;
    CgiProcess* process = new CgiProcess();
    bool started = false;
    try
    {
        started = process->start(_cgiJob, highestFd, now);
    }
    catch (...)
    {
        delete process;
        throw;
    }
    if (!started)
    {
        delete process;
        queueError(500, now);
        return;
    }
    _cgi = process;
}

int Client::cgiOutputFd() const
{
    if (_state == WAITING_FOR_CGI && _cgi != NULL && _cgi->isCollectingOutput())
        return _cgi->outputFd();
    return -1;
}

void Client::onCgiEvent(short revents, time_t now)
{
    if (cgiOutputFd() < 0 || !(revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)))
        return;
    CgiProcess::ReadStatus status = _cgi->readOutput(now);
    if (status != CgiProcess::READ_MORE)
        finishCgi(status, now);
}

void Client::finishCgi(CgiProcess::ReadStatus status, time_t now)
{
    HttpResponse response;
    if (status == CgiProcess::READ_EOF && CgiOutputParser::toResponse(_cgi->output(), response))
    {
        startResponse(response, _parser.request().keepAlive(), now);
        return;
    }
    Logger::warn("cgi " + _cgiJob.scriptPath + ": invalid or missing output");
    queueError(502, now);
}

void Client::retireCgi()
{
    if (_cgi == NULL)
        return;
    if (_retiredCgi != NULL)
    {
        _retiredCgi->terminate();
        _retiredCgi->waitBlocking();
        delete _retiredCgi;
    }
    _retiredCgi = _cgi;
    _cgi = NULL;
}

CgiProcess* Client::releaseCgi()
{
    CgiProcess* released = NULL;
    if (_retiredCgi != NULL)
    {
        released = _retiredCgi;
        _retiredCgi = NULL;
    }
    else if (_state == TO_CLOSE && _cgi != NULL)
    {
        released = _cgi;
        _cgi = NULL;
    }
    return released;
}

ConnectionInfo Client::connectionInfo() const
{
    ConnectionInfo info;
    info.remoteAddress = _remoteIp;
    info.remotePort = _remotePort;
    info.serverAddress = _listener.address().host;
    info.serverPort = _listener.address().port;
    return info;
}

void Client::queueError(int status, time_t now)
{
    const ServerConfig* server = (_route.server != NULL) ? _route.server : _listener.defaultServer();
    startResponse(ErrorPages::build(status, server), false, now);
}

void Client::startResponse(HttpResponse response, bool keepAlive, time_t now)
{
    closeFileBody();
    if (response.hasFileBody() && !openFileBody(response))
    {
        closeFileBody();
        const ServerConfig* server = (_route.server != NULL) ? _route.server : _listener.defaultServer();
        response = ErrorPages::build(500, server);
    }
    bool headOnly = _parser.request().method() == "HEAD";
    if (headOnly)
        closeFileBody();
    _outBuffer = response.serializeHead(keepAlive);
    if (!headOnly && !response.hasFileBody())
        _outBuffer += response.body();
    _outOffset = 0;
    _keepAlive = keepAlive;
    _responseStatus = response.status();
    _responseBytes = response.contentLength();
    setState(SENDING_RESPONSE, now);
}

bool Client::openFileBody(const HttpResponse& response)
{
    _file.open(response.filePath().c_str(), std::ios::in | std::ios::binary);
    if (!_file)
        return false;
    _file.seekg(static_cast<std::streamoff>(response.fileOffset()));
    if (!_file)
        return false;
    _fileRemaining = response.fileLength();
    return true;
}

bool Client::refillFromFile()
{
    if (_fileRemaining == 0)
        return false;
    char buffer[kIoChunkSize];
    size_t wanted = (_fileRemaining < sizeof(buffer)) ? _fileRemaining : sizeof(buffer);
    _file.read(buffer, static_cast<std::streamsize>(wanted));
    std::streamsize got = _file.gcount();
    if (got <= 0)
        return false;
    _outBuffer.assign(buffer, static_cast<size_t>(got));
    _outOffset = 0;
    _fileRemaining -= static_cast<size_t>(got);
    return true;
}

void Client::closeFileBody()
{
    if (_file.is_open())
        _file.close();
    _file.clear();
    _fileRemaining = 0;
}

void Client::onResponseSent(time_t now)
{
    logAccess();
    closeFileBody();
    retireCgi();
    _outBuffer.clear();
    _outOffset = 0;
    _parser.reset();
    _route = Route();
    if (!_keepAlive)
    {
        startLingering(now);
        return;
    }
    setState(READING_REQUEST, now);
    if (!_inBuffer.empty())
        processInput(now);
}

void Client::logAccess() const
{
    const HttpRequest& request = _parser.request();
    std::string requestLine = request.method().empty() ? "-"
        : request.method() + " " + request.target() + " " + request.version();
    Logger::info(_remoteIp + ":" + StringUtils::toString(_remotePort) + " \"" + requestLine + "\" "
                 + StringUtils::toString(_responseStatus) + " " + StringUtils::toString(_responseBytes));
}

void Client::startLingering(time_t now)
{
    if (shutdown(_fd, SHUT_WR) < 0)
    {
        finish();
        return;
    }
    _inBuffer.clear();
    setState(LINGERING, now);
}

void Client::checkTimeout(time_t now)
{
    switch (_state)
    {
        case READING_REQUEST:
        {
            bool partial = !_inBuffer.empty() || _parser.inProgress();
            if (!partial && now - _lastActivity >= kKeepAliveTimeoutSec)
                finish();
            else if (partial && now - _lastActivity >= kRequestTimeoutSec)
                queueError(408, now);
            break;
        }
        case SENDING_RESPONSE:
            if (now - _lastActivity >= kSendTimeoutSec)
                finish();
            break;
        case LINGERING:
            if (now - _stateSince >= kLingerTimeoutSec)
                finish();
            break;
        case WAITING_FOR_CGI:
            if (_cgi != NULL && _cgi->hasTimedOut(now))
            {
                Logger::warn("cgi " + _cgiJob.scriptPath + ": timeout, killed");
                _cgi->terminate();
                queueError(504, now);
            }
            break;
        case TO_CLOSE:
            break;
    }
}

void Client::setState(State state, time_t now)
{
    _state = state;
    _stateSince = now;
}

void Client::finish()
{
    _state = TO_CLOSE;
}
