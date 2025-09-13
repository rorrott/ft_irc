NAME = ircserv

DEF_COLOR =		\033[0;39m
RED =			\033[0;91m
GREEN =			\033[0;92m
BLUE =			\033[0;94m
MAGENTA =		\033[0;95m
CYAN =			\033[0;96m
BROWN =			\033[38;2;184;143;29m

SRCS = src/main.cpp src/Channel.cpp src/Client.cpp src/Server.cpp

INCLUDE = Channel.hpp Client.hpp Server.hpp
CXX = c++
RM = rm -f
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g
OBJS = ${SRCS:.cpp=.o}

%.o: %.cpp
	@echo "${BLUE} ◎ $(BROWN)Compiling   ${MAGENTA}→   $(CYAN)$< $(DEF_COLOR)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

all:	${NAME}

${NAME}: ${OBJS}
		@${CXX} ${CXXFLAGS} ${OBJS} -o ${NAME}
		@echo "\n$(GREEN) Created $(NAME) ✓ $(DEF_COLOR)\n"



clean:
		@${RM} ${OBJS}
		@echo "\n${BLUE} ◎ $(RED)All objects cleaned successfully ${BLUE}◎$(DEF_COLOR)\n"
fclean:
		@${RM} ${OBJS} ${NAME}
		@echo "\n${BLUE} ◎ $(RED)All objects and executable cleaned successfully${BLUE} ◎$(DEF_COLOR)\n"

re: fclean all

.PHONY: all clean fclean re