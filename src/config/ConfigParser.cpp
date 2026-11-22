#include "config/ConfigParser.hpp"

#include "utils/Constants.hpp"
#include "utils/FileUtils.hpp"
#include "utils/PathUtils.hpp"
#include "utils/StringUtils.hpp"

namespace
{
    const char* const kServerDirectives[] = {
        "listen", "server_name", "root", "index", "autoindex", "client_max_body_size",
        "error_page", NULL
    };
    const char* const kLocationDirectives[] = {
        "root", "index", "autoindex", "client_max_body_size", "allowed_methods", "return",
        "upload_store", "cgi", NULL
    };
    const char* const kRepeatableDirectives[] = { "listen", "error_page", "cgi", NULL };
    const char* const kAllowedMethods[] = { "GET", "POST", "DELETE", NULL };
    const int kRedirectCodes[] = { 301, 302, 303, 307, 308 };

    bool inList(const char* const* list, const std::string& word)
    {
        for (size_t i = 0; list[i] != NULL; ++i)
        {
            if (word == list[i])
                return true;
        }
        return false;
    }

    bool isKnownDirective(const std::string& name)
    {
        return name == "server" || name == "location"
            || inList(kServerDirectives, name) || inList(kLocationDirectives, name);
    }

    bool isRedirectCode(int code)
    {
        for (size_t i = 0; i < sizeof(kRedirectCodes) / sizeof(kRedirectCodes[0]); ++i)
        {
            if (kRedirectCodes[i] == code)
                return true;
        }
        return false;
    }

    bool isValidHost(const std::string& host)
    {
        if (host == "localhost")
            return true;
        std::vector<std::string> parts = StringUtils::split(host, '.');
        if (parts.size() != 4)
            return false;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            int value = 0;
            if (parts[i].size() > 3 || !StringUtils::parseInt(parts[i], 0, 255, value)
                || parts[i][0] == '-')
                return false;
        }
        return true;
    }

    std::string quote(const std::string& text)
    {
        return "'" + text + "'";
    }
}

ConfigError::ConfigError(const std::string& message)
    : std::runtime_error("config error: " + message)
{
}

ConfigParser::ConfigParser() : _tokens(), _position(0), _source()
{
}

std::vector<ServerConfig> ConfigParser::parseFile(const std::string& path)
{
    if (!FileUtils::exists(path))
        throw ConfigError(path + ": file not found");
    if (!FileUtils::isRegularFile(path))
        throw ConfigError(path + ": not a regular file");
    if (!FileUtils::isReadable(path))
        throw ConfigError(path + ": permission denied");
    std::string content;
    if (!FileUtils::readFile(path, content, kMaxConfigFileSize))
        throw ConfigError(path + ": cannot read file (larger than "
                          + StringUtils::toString(kMaxConfigFileSize) + " bytes?)");
    return parseString(content, path);
}

std::vector<ServerConfig> ConfigParser::parseString(const std::string& content,
                                                    const std::string& sourceName)
{
    _tokens = ConfigTokenizer::tokenize(content);
    _position = 0;
    _source = sourceName;
    std::vector<ServerConfig> servers;
    while (current().type != ConfigToken::END_OF_FILE)
    {
        if (current().type != ConfigToken::WORD)
            fail(current().line, "unexpected " + quote(current().text));
        Directive directive = readDirective();
        if (directive.name != "server" && isKnownDirective(directive.name))
            fail(directive.line, "directive " + quote(directive.name)
                 + " is not allowed outside a server block");
        if (directive.name != "server")
            fail(directive.line, "unknown directive " + quote(directive.name));
        servers.push_back(parseServer(directive));
    }
    if (servers.empty())
        throw ConfigError(_source + ": no server block found");
    checkConflicts(servers);
    return servers;
}

const ConfigToken& ConfigParser::current() const
{
    return _tokens[_position];
}

void ConfigParser::advance()
{
    if (_position + 1 < _tokens.size())
        ++_position;
}

ConfigParser::Directive ConfigParser::readDirective()
{
    Directive directive;
    directive.name = current().text;
    directive.line = current().line;
    advance();
    while (current().type == ConfigToken::WORD)
    {
        if (current().line != directive.line)
            fail(directive.line, "missing ';' after " + quote(directive.name));
        directive.args.push_back(current().text);
        advance();
    }
    return directive;
}

void ConfigParser::expectSemicolon(const Directive& directive)
{
    if (current().type == ConfigToken::SEMICOLON)
    {
        advance();
        return;
    }
    if (current().type == ConfigToken::OPEN_BRACE)
        fail(directive.line, "unexpected '{' after " + quote(directive.name));
    fail(directive.line, "missing ';' after " + quote(directive.name));
}

void ConfigParser::expectOpenBrace(const Directive& directive)
{
    if (current().type != ConfigToken::OPEN_BRACE)
        fail(directive.line, "expected '{' after " + quote(directive.name));
    advance();
}

void ConfigParser::fail(int line, const std::string& message) const
{
    throw ConfigError(_source + ":" + StringUtils::toString(line) + ": " + message);
}

ServerConfig ConfigParser::parseServer(const Directive& keyword)
{
    if (!keyword.args.empty())
        fail(keyword.line, "'server' takes no arguments");
    expectOpenBrace(keyword);
    ServerDraft draft;
    draft.hasRoot = false;
    draft.line = keyword.line;
    while (current().type != ConfigToken::CLOSE_BRACE)
    {
        if (current().type == ConfigToken::END_OF_FILE)
            fail(keyword.line, "'server' block is never closed (missing '}')");
        if (current().type != ConfigToken::WORD)
            fail(current().line, "unexpected " + quote(current().text));
        Directive directive = readDirective();
        if (directive.name == "location")
            parseLocation(directive, draft);
        else
            applyServerDirective(draft, directive);
    }
    advance();
    return finalizeServer(draft);
}

void ConfigParser::parseLocation(const Directive& keyword, ServerDraft& draft)
{
    if (keyword.args.size() != 1)
        fail(keyword.line, "'location' needs exactly one path");
    const std::string& path = keyword.args[0];
    if (path[0] != '/')
        fail(keyword.line, "location path must start with '/': " + quote(path));
    for (size_t i = 0; i < draft.locations.size(); ++i)
    {
        if (draft.locations[i].path() == path)
            fail(keyword.line, "duplicate location " + quote(path));
    }
    expectOpenBrace(keyword);
    LocationConfig location;
    location.setPath(path);
    std::set<std::string> seen;
    while (current().type != ConfigToken::CLOSE_BRACE)
    {
        if (current().type == ConfigToken::END_OF_FILE)
            fail(keyword.line, "'location " + path + "' block is never closed (missing '}')");
        if (current().type != ConfigToken::WORD)
            fail(current().line, "unexpected " + quote(current().text));
        applyLocationDirective(location, seen, readDirective());
    }
    advance();
    draft.locations.push_back(location);
}

void ConfigParser::applyServerDirective(ServerDraft& draft, const Directive& directive)
{
    checkContext(directive, kServerDirectives, "server");
    expectSemicolon(directive);
    checkNotRepeated(draft.seen, directive);
    const std::string& name = directive.name;
    if (name == "listen")
    {
        checkArgumentCount(directive, 1, 1);
        if (!draft.config.addListen(parseListen(directive)))
            fail(directive.line, "duplicate listen " + quote(directive.args[0]));
    }
    else if (name == "server_name")
    {
        checkArgumentCount(directive, 1, 0);
        draft.config.setServerNames(directive.args);
    }
    else if (name == "root")
    {
        draft.config.setRoot(parseDirectory(directive, false));
        draft.hasRoot = true;
    }
    else if (name == "index")
    {
        checkArgumentCount(directive, 1, 0);
        draft.config.setIndexFiles(directive.args);
    }
    else if (name == "autoindex")
        draft.config.setAutoindex(parseAutoindex(directive));
    else if (name == "client_max_body_size")
        draft.config.setClientMaxBodySize(parseBodySize(directive));
    else
        parseErrorPage(draft, directive);
}

void ConfigParser::applyLocationDirective(LocationConfig& location, std::set<std::string>& seen,
                                          const Directive& directive)
{
    if (directive.name == "location")
        fail(directive.line, "nested location blocks are not allowed");
    checkContext(directive, kLocationDirectives, "location");
    expectSemicolon(directive);
    checkNotRepeated(seen, directive);
    const std::string& name = directive.name;
    if (name == "root")
        location.setRoot(parseDirectory(directive, false));
    else if (name == "index")
    {
        checkArgumentCount(directive, 1, 0);
        location.setIndexFiles(directive.args);
    }
    else if (name == "autoindex")
        location.setAutoindex(parseAutoindex(directive));
    else if (name == "client_max_body_size")
        location.setClientMaxBodySize(parseBodySize(directive));
    else if (name == "allowed_methods")
        location.setAllowedMethods(parseMethods(directive));
    else if (name == "return")
        parseReturn(location, directive);
    else if (name == "upload_store")
        location.setUploadStore(parseDirectory(directive, true));
    else
        parseCgi(location, directive);
}

void ConfigParser::checkContext(const Directive& directive, const char* const* allowed,
                                const std::string& blockName) const
{
    if (inList(allowed, directive.name))
        return;
    if (directive.name == "server")
        fail(directive.line, "'server' blocks cannot be nested");
    if (isKnownDirective(directive.name))
        fail(directive.line, "directive " + quote(directive.name)
             + " is not allowed in a " + blockName + " block");
    fail(directive.line, "unknown directive " + quote(directive.name));
}

void ConfigParser::checkArgumentCount(const Directive& directive, size_t min, size_t max) const
{
    size_t count = directive.args.size();
    if (count < min || (max != 0 && count > max))
        fail(directive.line, "wrong number of arguments for " + quote(directive.name));
}

void ConfigParser::checkNotRepeated(std::set<std::string>& seen, const Directive& directive) const
{
    if (inList(kRepeatableDirectives, directive.name))
        return;
    if (!seen.insert(directive.name).second)
        fail(directive.line, "directive " + quote(directive.name) + " is repeated in this block");
}

ListenAddress ConfigParser::parseListen(const Directive& directive) const
{
    const std::string& value = directive.args[0];
    ListenAddress address;
    address.host = "0.0.0.0";
    std::string portText = value;
    size_t colon = value.rfind(':');
    if (colon != std::string::npos)
    {
        address.host = value.substr(0, colon);
        portText = value.substr(colon + 1);
        if (!isValidHost(address.host))
            fail(directive.line, "invalid host " + quote(address.host)
                 + " (expected localhost or an IPv4 address)");
    }
    if (!StringUtils::parseInt(portText, 1, 65535, address.port))
        fail(directive.line, "invalid port " + quote(portText) + " (expected 1-65535)");
    return address;
}

std::string ConfigParser::resolvePath(const Directive& directive, const std::string& path) const
{
    std::string absolute;
    if (!PathUtils::makeAbsolute(path, absolute))
        fail(directive.line, "cannot make " + quote(path)
             + " absolute (the PWD environment variable is missing)");
    return absolute;
}

std::string ConfigParser::parseDirectory(const Directive& directive, bool mustBeWritable) const
{
    checkArgumentCount(directive, 1, 1);
    std::string directory = resolvePath(directive, directive.args[0]);
    if (!FileUtils::isDirectory(directory))
        fail(directive.line, quote(directive.name) + " directory not found: "
             + quote(directive.args[0]) + " (resolved to " + directory + ")");
    if (mustBeWritable && !FileUtils::isWritable(directory))
        fail(directive.line, quote(directive.name) + " directory is not writable: "
             + quote(directory));
    return directory;
}

bool ConfigParser::parseAutoindex(const Directive& directive) const
{
    checkArgumentCount(directive, 1, 1);
    if (directive.args[0] == "on")
        return true;
    if (directive.args[0] == "off")
        return false;
    fail(directive.line, "autoindex must be 'on' or 'off', not " + quote(directive.args[0]));
    return false;
}

size_t ConfigParser::parseBodySize(const Directive& directive) const
{
    checkArgumentCount(directive, 1, 1);
    size_t size = 0;
    if (!StringUtils::parseSize(directive.args[0], size))
        fail(directive.line, "invalid size " + quote(directive.args[0])
             + " (examples: 1024, 8K, 10M, 1G)");
    return size;
}

std::set<std::string> ConfigParser::parseMethods(const Directive& directive) const
{
    checkArgumentCount(directive, 1, 0);
    std::set<std::string> methods;
    for (size_t i = 0; i < directive.args.size(); ++i)
    {
        if (!inList(kAllowedMethods, directive.args[i]))
            fail(directive.line, "invalid method " + quote(directive.args[i])
                 + " (allowed: GET POST DELETE)");
        methods.insert(directive.args[i]);
    }
    return methods;
}

void ConfigParser::parseReturn(LocationConfig& location, const Directive& directive) const
{
    checkArgumentCount(directive, 2, 2);
    int code = 0;
    if (!StringUtils::parseInt(directive.args[0], 0, 999, code) || !isRedirectCode(code))
        fail(directive.line, "invalid return code " + quote(directive.args[0])
             + " (allowed: 301 302 303 307 308)");
    location.setRedirect(code, directive.args[1]);
}

void ConfigParser::parseCgi(LocationConfig& location, const Directive& directive) const
{
    checkArgumentCount(directive, 2, 2);
    const std::string& extension = directive.args[0];
    if (extension.size() < 2 || extension[0] != '.' || extension.find('/') != std::string::npos)
        fail(directive.line, "invalid cgi extension " + quote(extension)
             + " (must start with '.', like .py)");
    std::string interpreter = resolvePath(directive, directive.args[1]);
    if (!FileUtils::isRegularFile(interpreter) || !FileUtils::isExecutable(interpreter))
        fail(directive.line, "cgi interpreter is not an executable file: "
             + quote(directive.args[1]) + " (resolved to " + interpreter + ")");
    if (!location.addCgiHandler(extension, interpreter))
        fail(directive.line, "cgi extension " + quote(extension) + " is repeated");
}

void ConfigParser::parseErrorPage(ServerDraft& draft, const Directive& directive) const
{
    checkArgumentCount(directive, 2, 0);
    for (size_t i = 0; i + 1 < directive.args.size(); ++i)
    {
        PendingErrorPage page;
        if (!StringUtils::parseInt(directive.args[i], 400, 599, page.code))
            fail(directive.line, "invalid error_page code " + quote(directive.args[i])
                 + " (expected 400-599)");
        if (!draft.errorCodes.insert(page.code).second)
            fail(directive.line, "error_page for " + directive.args[i] + " is repeated");
        page.path = directive.args[directive.args.size() - 1];
        page.line = directive.line;
        draft.errorPages.push_back(page);
    }
}

ServerConfig ConfigParser::finalizeServer(ServerDraft& draft) const
{
    if (draft.config.listens().empty())
        fail(draft.line, "server block has no 'listen' directive");
    if (!draft.hasRoot)
        fail(draft.line, "server block has no 'root' directive");
    ServerConfig server = draft.config;
    for (size_t i = 0; i < draft.errorPages.size(); ++i)
    {
        const PendingErrorPage& page = draft.errorPages[i];
        std::string file = PathUtils::join(server.root(), page.path);
        if (!FileUtils::isRegularFile(file))
            fail(page.line, "error_page file not found: " + quote(page.path)
                 + " (resolved to " + file + ")");
        server.addErrorPage(page.code, file);
    }
    bool hasRootLocation = false;
    for (size_t i = 0; i < draft.locations.size(); ++i)
    {
        if (draft.locations[i].path() == "/")
            hasRootLocation = true;
    }
    if (!hasRootLocation)
        draft.locations.insert(draft.locations.begin(), LocationConfig());
    for (size_t i = 0; i < draft.locations.size(); ++i)
    {
        inheritFromServer(draft.locations[i], server);
        server.addLocation(draft.locations[i]);
    }
    return server;
}

void ConfigParser::inheritFromServer(LocationConfig& location, const ServerConfig& server) const
{
    if (!location.hasRoot())
    {
        if (location.path() == "/")
            location.setRoot(server.root());
        else
            location.setRoot(PathUtils::join(server.root(), location.path()));
    }
    if (!location.hasIndexFiles())
        location.setIndexFiles(server.indexFiles());
    if (!location.hasAutoindex())
        location.setAutoindex(server.autoindex());
    if (!location.hasClientMaxBodySize())
        location.setClientMaxBodySize(server.clientMaxBodySize());
}

void ConfigParser::checkConflicts(const std::vector<ServerConfig>& servers) const
{
    for (size_t a = 0; a < servers.size(); ++a)
    {
        for (size_t b = a + 1; b < servers.size(); ++b)
        {
            for (size_t i = 0; i < servers[a].listens().size(); ++i)
            {
                for (size_t j = 0; j < servers[b].listens().size(); ++j)
                {
                    std::string address = servers[a].listens()[i].toString();
                    if (address == servers[b].listens()[j].toString())
                        checkSharedAddress(servers[a], servers[b], address);
                }
            }
        }
    }
}

void ConfigParser::checkSharedAddress(const ServerConfig& first, const ServerConfig& second,
                                      const std::string& address) const
{
    const std::string where = _source + ": conflicting servers on " + address + ": ";
    if (first.serverNames().empty() && second.serverNames().empty())
        throw ConfigError(where + "two servers without server_name");
    for (size_t i = 0; i < second.serverNames().size(); ++i)
    {
        if (first.hasServerName(second.serverNames()[i]))
            throw ConfigError(where + "server_name " + quote(second.serverNames()[i])
                              + " is used twice");
    }
}
