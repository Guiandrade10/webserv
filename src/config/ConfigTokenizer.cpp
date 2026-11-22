#include "config/ConfigTokenizer.hpp"

#include <cctype>

namespace
{
    ConfigToken makeToken(ConfigToken::Type type, const std::string& text, int line)
    {
        ConfigToken token;
        token.type = type;
        token.text = text;
        token.line = line;
        return token;
    }

    bool isSpecial(char c)
    {
        return c == '{' || c == '}' || c == ';' || c == '#';
    }

    bool isSpace(char c)
    {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    }
}

std::vector<ConfigToken> ConfigTokenizer::tokenize(const std::string& content)
{
    std::vector<ConfigToken> tokens;
    int line = 1;
    size_t i = 0;
    while (i < content.size())
    {
        char c = content[i];
        if (c == '\n')
        {
            ++line;
            ++i;
        }
        else if (isSpace(c))
            ++i;
        else if (c == '#')
        {
            while (i < content.size() && content[i] != '\n')
                ++i;
        }
        else if (c == '{' || c == '}' || c == ';')
        {
            ConfigToken::Type type = (c == '{') ? ConfigToken::OPEN_BRACE
                                   : (c == '}') ? ConfigToken::CLOSE_BRACE
                                   : ConfigToken::SEMICOLON;
            tokens.push_back(makeToken(type, std::string(1, c), line));
            ++i;
        }
        else
        {
            size_t start = i;
            while (i < content.size() && !isSpace(content[i]) && !isSpecial(content[i]))
                ++i;
            tokens.push_back(makeToken(ConfigToken::WORD, content.substr(start, i - start), line));
        }
    }
    tokens.push_back(makeToken(ConfigToken::END_OF_FILE, "end of file", line));
    return tokens;
}
