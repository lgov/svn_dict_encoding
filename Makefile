# Makefile for the svn dictionary compression test

CC=gcc
CFLAGS=-g

LIB_PATHS=-L/opt/local/lib -L/usr/local/lib
INC_PATHS=-I/opt/local/include/apr-1 -I/opt/local/include -I/usr/local/include/serf-2

LIBS=-lapr-1 -laprutil-1 -lserf-2

OBJS=$(addprefix $(OBJDIR)/, svn_dict)
OBJDIR=build

svn_dict: $(OBJS)
		$(CC) -o svn_dict  $(LIB_PATHS) $(LIBS) $(OBJS)

$(OBJDIR)/%.o : %.c
		$(CC) $(CFLAGS) $(INC_PATHS) -o build/$*.o -c $<

svn_dict.c : svn_dict.h

clean:
		rm -f svn_dict build/svn_dict.o
