NAME      := webserv
CXX       := c++
CXXFLAGS  := -Wall -Wextra -Werror -std=c++98
CPPFLAGS  := -I src -MMD -MP
OBJ_DIR   := obj

SRCS      := src/main.cpp \
             src/cgi/CgiEnvironment.cpp \
             src/cgi/CgiOutputParser.cpp \
             src/cgi/CgiProcess.cpp \
             src/config/ConfigParser.cpp \
             src/config/ConfigTokenizer.cpp \
             src/config/LocationConfig.cpp \
             src/config/ServerConfig.cpp \
             src/core/Client.cpp \
             src/core/EventLoop.cpp \
             src/core/Listener.cpp \
             src/core/Signals.cpp \
             src/http/HttpRequest.cpp \
             src/http/HttpResponse.cpp \
             src/http/MimeTypes.cpp \
             src/http/Multipart.cpp \
             src/http/RequestParser.cpp \
             src/http/StatusCodes.cpp \
             src/handlers/AutoIndex.cpp \
             src/handlers/DeleteHandler.cpp \
             src/handlers/ErrorPages.cpp \
             src/handlers/RequestHandler.cpp \
             src/handlers/Router.cpp \
             src/handlers/StaticHandler.cpp \
             src/handlers/UploadHandler.cpp \
             src/utils/FileUtils.cpp \
             src/utils/Logger.cpp \
             src/utils/PathUtils.cpp \
             src/utils/StringUtils.cpp \
             src/utils/TempFile.cpp

OBJS      := $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS      := $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: $(NAME)
	bash tester/run_tester.sh

-include $(DEPS)

.PHONY: all clean fclean re test
