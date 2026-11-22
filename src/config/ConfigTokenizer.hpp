#ifndef WEBSERV_CONFIG_CONFIGTOKENIZER_HPP
#define WEBSERV_CONFIG_CONFIGTOKENIZER_HPP

#include <string>
#include <vector>

struct ConfigToken
{
    enum Type { WORD, OPEN_BRACE, CLOSE_BRACE, SEMICOLON, END_OF_FILE };
    Type        type;
    std::string text;
    int         line;
};

class ConfigTokenizer
{
public:
    static std::vector<ConfigToken> tokenize(const std::string& content);
};

#endif
