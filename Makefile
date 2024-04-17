NAME = webserv

CXX = c++
RM = rm -f

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(addprefix src/, main.cpp \
												configuration_parser.cpp \
												multiplexer.cpp \
												event_handler.cpp \
												file_reader.cpp \
												configuration.cpp \
												server.cpp \
												client.cpp \
												abnf.cpp \
												uri.cpp \
												transaction.cpp \
												entity.cpp \
												http.cpp \
												cgi.cpp \
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
