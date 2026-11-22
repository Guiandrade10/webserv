#ifndef WEBSERV_UTILS_CONSTANTS_HPP
#define WEBSERV_UTILS_CONSTANTS_HPP

#include <cstddef>

const char* const kDefaultConfigPath     = "config/default.conf";
const char* const kServerSoftware        = "webserv/1.0";
const size_t      kMaxConfigFileSize      = 1024 * 1024;
const size_t      kMaxErrorPageSize       = 1024 * 1024;
const char* const kTempDirectory         = "/tmp";
const int         kTempFileCreateAttempts = 100;

const size_t kIoChunkSize                = 64 * 1024;
const size_t kMaxRequestLineSize         = 8 * 1024;
const size_t kMaxHeaderSectionSize       = 16 * 1024;
const size_t kMaxHeaderCount             = 100;
const size_t kCgiWaitInputLimit          = 16 * 1024;
const size_t kMaxChunkSizeLine           = 1024;
const size_t kDefaultClientMaxBodySize   = 1024 * 1024;
const size_t kMaxInMemoryMultipart       = 64 * 1024 * 1024;
const size_t kMaxClients                 = 256;
const int    kListenBacklog              = 512;
const int    kMaxGeneratedNameAttempts   = 1000;
const size_t kMaxFileNameLength          = 255;

const int    kPollTimeoutMs              = 1000;
const int    kRequestTimeoutSec          = 30;
const int    kKeepAliveTimeoutSec        = 30;
const int    kSendTimeoutSec             = 30;
const int    kCgiIdleTimeoutSec          = 10;
const int    kCgiMaxRuntimeSec           = 60;
const int    kLingerTimeoutSec           = 2;
const int    kChildFdCloseLimit          = 1024;

#endif
