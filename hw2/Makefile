.PHONY: all debug clean fclean re
# Name of your program:
NAME=VMSimulator

# List of all .cpp source code files included in your program (separated by spaces):
SRC=VMSimulator.cpp



SRCPATH=./
OBJ=$(addprefix $(SRCPATH), $(SRC:.cpp=.o))

RM=rm -f
INCPATH=includes
CPPFLAGS+= -std=c++14 -Wall -Werror -pedantic -I $(INCPATH)


all: $(OBJ)
	g++ $(OBJ) -o $(NAME) -lpthread

debug: $(OBJ)
	g++ $(OBJ) -g -o $(NAME) -lpthread

clean:
	-$(RM) *~
	-$(RM) *#*
	-$(RM) *swp
	-$(RM) *.core
	-$(RM) *.stackdump
	-$(RM) $(SRCPATH)*.o
	-$(RM) $(SRCPATH)*.obj
	-$(RM) $(SRCPATH)*~
	-$(RM) $(SRCPATH)*#*
	-$(RM) $(SRCPATH)*swp
	-$(RM) $(SRCPATH)*.core
	-$(RM) $(SRCPATH)*.stackdump

fclean: clean
	-$(RM) $(NAME)

re: fclean all
