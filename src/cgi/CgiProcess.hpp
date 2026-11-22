#ifndef WEBSERV_CGI_CGIPROCESS_HPP
#define WEBSERV_CGI_CGIPROCESS_HPP

#include "utils/TempFile.hpp"

#include <ctime>
#include <string>
#include <sys/types.h>
#include <vector>

struct CgiJob
{
    std::string              interpreter;
    std::string              scriptPath;
    std::string              workingDirectory;
    std::string              bodyPath;
    std::vector<std::string> environment;
};

class CgiProcess
{
public:
    enum ReadStatus { READ_MORE, READ_EOF, READ_ERROR };

    CgiProcess();
    ~CgiProcess();
    bool start(const CgiJob& job, int highestFd, time_t now);
    int  outputFd() const;
    bool isCollectingOutput() const;
    ReadStatus readOutput(time_t now);
    bool hasTimedOut(time_t now) const;
    void terminate();
    bool tryReap();
    void waitBlocking();
    const TempFile& output() const;

private:
    CgiProcess(const CgiProcess&);
    CgiProcess& operator=(const CgiProcess&);
    bool openPipeAndOutput(int pipeFds[2]);
    bool finishParentSide(const int pipeFds[2], time_t now);
    void runChild(const CgiJob& job, const int pipeFds[2], int highestFd,
                  char* const argv[], char* const envp[]);

    pid_t    _pid;
    int      _outFd;
    bool     _collecting;
    bool     _reaped;
    time_t   _startTime;
    time_t   _lastOutput;
    TempFile _output;
};

#endif
