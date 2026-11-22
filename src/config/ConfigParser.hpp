#ifndef WEBSERV_CONFIG_CONFIGPARSER_HPP
#define WEBSERV_CONFIG_CONFIGPARSER_HPP

#include "config/ConfigTokenizer.hpp"
#include "config/ServerConfig.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

class ConfigError : public std::runtime_error
{
public:
    explicit ConfigError(const std::string& message);
};

class ConfigParser
{
public:
    ConfigParser();
    std::vector<ServerConfig> parseFile(const std::string& path);
    std::vector<ServerConfig> parseString(const std::string& content,
                                          const std::string& sourceName);

private:
    struct Directive
    {
        std::string              name;
        std::vector<std::string> args;
        int                      line;
    };

    struct PendingErrorPage
    {
        int         code;
        std::string path;
        int         line;
    };

    struct ServerDraft
    {
        ServerConfig                  config;
        std::vector<LocationConfig>   locations;
        std::vector<PendingErrorPage> errorPages;
        std::set<int>                 errorCodes;
        std::set<std::string>         seen;
        bool                          hasRoot;
        int                           line;
    };

    const ConfigToken& current() const;
    void advance();
    Directive readDirective();
    void expectSemicolon(const Directive& directive);
    void expectOpenBrace(const Directive& directive);
    void fail(int line, const std::string& message) const;

    ServerConfig parseServer(const Directive& keyword);
    void parseLocation(const Directive& keyword, ServerDraft& draft);
    void applyServerDirective(ServerDraft& draft, const Directive& directive);
    void applyLocationDirective(LocationConfig& location, std::set<std::string>& seen,
                                const Directive& directive);
    void checkContext(const Directive& directive, const char* const* allowed,
                      const std::string& blockName) const;
    void checkArgumentCount(const Directive& directive, size_t min, size_t max) const;
    void checkNotRepeated(std::set<std::string>& seen, const Directive& directive) const;

    ListenAddress parseListen(const Directive& directive) const;
    std::string parseDirectory(const Directive& directive, bool mustBeWritable) const;
    std::string resolvePath(const Directive& directive, const std::string& path) const;
    bool parseAutoindex(const Directive& directive) const;
    size_t parseBodySize(const Directive& directive) const;
    std::set<std::string> parseMethods(const Directive& directive) const;
    void parseReturn(LocationConfig& location, const Directive& directive) const;
    void parseCgi(LocationConfig& location, const Directive& directive) const;
    void parseErrorPage(ServerDraft& draft, const Directive& directive) const;

    ServerConfig finalizeServer(ServerDraft& draft) const;
    void inheritFromServer(LocationConfig& location, const ServerConfig& server) const;
    void checkConflicts(const std::vector<ServerConfig>& servers) const;
    void checkSharedAddress(const ServerConfig& first, const ServerConfig& second,
                            const std::string& address) const;

    std::vector<ConfigToken> _tokens;
    size_t                   _position;
    std::string              _source;
};

#endif
