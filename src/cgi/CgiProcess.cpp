#include "cgi/CgiProcess.hpp"

#include "utils/Constants.hpp"
#include "utils/Logger.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    std::vector<char*> toCStrings(const std::vector<std::string>& strings)
    {
        std::vector<char*> pointers;
        for (size_t i = 0; i < strings.size(); ++i)
            pointers.push_back(const_cast<char*>(strings[i].c_str()));
        pointers.push_back(NULL);
        return pointers;
    }

    int maxOf(int a, int b)
    {
        return (a > b) ? a : b;
    }
}

CgiProcess::CgiProcess()
    : _pid(-1), _outFd(-1), _collecting(false), _reaped(false), _startTime(0), _lastOutput(0),
      _output()
{
}

CgiProcess::~CgiProcess()
{
    if (_outFd >= 0)
        close(_outFd);
}

int CgiProcess::outputFd() const { return _outFd; }
bool CgiProcess::isCollectingOutput() const { return _collecting; }
const TempFile& CgiProcess::output() const { return _output; }

bool CgiProcess::openPipeAndOutput(int pipeFds[2])
{
    if (pipe(pipeFds) < 0)
    {
        Logger::error(std::string("cgi pipe: ") + std::strerror(errno));
        return false;
    }
    if (!_output.create())
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }
    return true;
}

bool CgiProcess::start(const CgiJob& job, int highestFd, time_t now)
{
    std::vector<std::string> arguments;
    arguments.push_back(job.interpreter);
    arguments.push_back(job.scriptPath);
    std::vector<char*> argv = toCStrings(arguments);
    std::vector<char*> envp = toCStrings(job.environment);
    int pipeFds[2];
    if (!openPipeAndOutput(pipeFds))
        return false;
    std::cout.flush();
    std::cerr.flush();
    _pid = fork();
    if (_pid < 0)
    {
        Logger::error(std::string("cgi fork: ") + std::strerror(errno));
        close(pipeFds[0]);
        close(pipeFds[1]);
        _output.remove();
        return false;
    }
    if (_pid == 0)
        runChild(job, pipeFds, highestFd, &argv[0], &envp[0]);
    return finishParentSide(pipeFds, now);
}

bool CgiProcess::finishParentSide(const int pipeFds[2], time_t now)
{
    close(pipeFds[1]);
    _outFd = pipeFds[0];
    if (fcntl(_outFd, F_SETFL, O_NONBLOCK) < 0)
    {
        terminate();
        waitBlocking();
        return false;
    }
    _collecting = true;
    _startTime = now;
    _lastOutput = now;
    return true;
}

void CgiProcess::runChild(const CgiJob& job, const int pipeFds[2], int highestFd,
                          char* const argv[], char* const envp[])
{
    const char* inputPath = job.bodyPath.empty() ? "/dev/null" : job.bodyPath.c_str();
    int input = open(inputPath, O_RDONLY);
    if (input < 0 || dup2(input, STDIN_FILENO) < 0)
        std::exit(1);
    if (dup2(pipeFds[1], STDOUT_FILENO) < 0)
        std::exit(1);
    int last = maxOf(maxOf(highestFd, kChildFdCloseLimit), maxOf(maxOf(pipeFds[0], pipeFds[1]), input));
    for (int fd = STDERR_FILENO + 1; fd <= last; ++fd)
        close(fd);
    signal(SIGPIPE, SIG_DFL);
    if (chdir(job.workingDirectory.c_str()) < 0)
        std::exit(1);
    execve(argv[0], argv, envp);
    std::exit(1);
}

CgiProcess::ReadStatus CgiProcess::readOutput(time_t now)
{
    char buffer[kIoChunkSize];
    ssize_t count = read(_outFd, buffer, sizeof(buffer));
    if (count < 0)
    {
        _collecting = false;
        return READ_ERROR;
    }
    if (count == 0)
    {
        _collecting = false;
        _output.closeWriter();
        return READ_EOF;
    }
    if (!_output.append(buffer, static_cast<size_t>(count)))
    {
        _collecting = false;
        return READ_ERROR;
    }
    _lastOutput = now;
    return READ_MORE;
}

bool CgiProcess::hasTimedOut(time_t now) const
{
    return _collecting && (now - _lastOutput >= kCgiIdleTimeoutSec
                           || now - _startTime >= kCgiMaxRuntimeSec);
}

void CgiProcess::terminate()
{
    if (_pid > 0 && !_reaped)
        kill(_pid, SIGKILL);
    _collecting = false;
}

bool CgiProcess::tryReap()
{
    if (_pid <= 0 || _reaped)
        return true;
    int status = 0;
    pid_t result = waitpid(_pid, &status, WNOHANG);
    if (result == 0)
        return false;
    _reaped = true;
    return true;
}

void CgiProcess::waitBlocking()
{
    if (_pid > 0 && !_reaped)
        waitpid(_pid, NULL, 0);
    _reaped = true;
}
