NAME = webserv

CXX = c++ -std=c++98
RM = rm -f

CXXFLAGS = -Wall -Wextra -Werror

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(addprefix src/, main.cpp \
													multiplexer.cpp \
													configuration_parser.cpp \
													event_handler.cpp \
													file_reader.cpp \
													configuration.cpp \
													transaction.cpp \
													server.cpp \
													client.cpp \
													socket.cpp \
													entity.cpp \
													uri.cpp \
													cgi.cpp \
													http.cpp \
													abnf.cpp \
													utils.cpp)

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(OBJ_DIR) $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $(NAME)

$(OBJ_DIR):
	mkdir obj

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: clean fclean re all
