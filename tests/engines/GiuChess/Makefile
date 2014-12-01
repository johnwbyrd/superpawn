PROG = GiuChess-1.0beta2
CSOURCES = check.c exec_mov.c main.c util_functs.c iniz.c legal.c pieces_legmoves.c pieces_ex_legmoves.c wblegmov.c rw_thread.c eval_pos.c list.c
HEADERS = check.h exec_mov.h main.h util_functs.h iniz.h legal.h pieces_legmoves.h pieces_ex_legmoves.h wblegmov.h rw_thread.h eval_pos.h list.h
COMPIL_FLAGS = -Wall -g -O4
LIB = -lm -lpthread
CC = gcc
OBJECTS = $(CSOURCES:.c=.o)

all:	$(PROG)

$(PROG):	$(OBJECTS)
	$(CC) $(COMPIL_FLAGS) $(LIB) -o $(PROG) $(OBJECTS)
	
%.o:	%.c $(HEADERS)
	$(CC) -c $<
	
clean:
	rm -f *.o $(PROG)

nett:
	rm -f *~

