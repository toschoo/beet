# =========================================================================
# Makefile for BEET
# (c) Tobias Schoofs, 2018
# =========================================================================
CC = @gcc
CXX = @g++
AR = @ar

LNKMSG = @printf "linking   $@\n"
CMPMSG = @printf "compiling $@\n"

FLGMSG = @printf "CFLAGS: $(CFLAGS)\nLDFLAGS: $(LDFLAGS)\n"

INSMSG = @printf ". setenv.sh"

CFLAGS = -g -O3 -Wall -std=c99 -fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS = -L./lib

INC = -I./include -I./test -I./src -I./
LIB = lib
libs=-lm -lpthread -ltsalgo

SRC = src/beet
HDR = include/beet
TST = test
COM = common
SMK = test/smoke
STRS = test/stress
TOOLS = tools
LOG = log
RSC = rsc
OUTLIB = lib

OBJ = $(SRC)/lock.o \
      $(SRC)/error.o \
      $(SRC)/page.o \
      $(SRC)/rider.o

DEP = $(SRC)/lock.h \
      $(SRC)/page.h \
      $(SRC)/rider.h

default:	lib 

all:	default tests tools

tools:	

tests:	smoke stress

smoke:	$(SMK)/pagesmoke \
	$(SMK)/ridersmoke

stress: $(STRS)/riderstress

debug:	CFLAGS += -g
debug:	default
debug:	tools

install:	$(OUTLIB)/libbeet.so
		cp $(OUTLIB)/libbeet.so /usr/local/lib/
		cp -r include/beet /usr/local/include/

run:	

flags:
	$(FLGMSG)

.SUFFIXES: .c .o

.c.o:
	$(CMPMSG)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

.cpp.o:
	$(CMPMSG)
	$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

# Tests and demos

# Tools

# Library
lib:	lib/libbeet.so

lib/libbeet.so:	$(OBJ) $(DEP)
		$(LNKMSG)
		$(CC) -shared \
		-o $(OUTLIB)/libbeet.so \
		$(OBJ) \
		$(libs)
			
# Tests and demos
$(TST)/tstprogress:	$(TST)/tstprogress.o $(TST)/progress.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(TST)/tstprogress \
			                    $(TST)/progress.o  \
			                    $(TST)/tstprogress.o

$(SMK)/pagesmoke:	lib $(SMK)/pagesmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/pagesmoke   \
			                    $(SMK)/pagesmoke.o \
			                    $(COM)/math.o      \
			                    $(libs) -lbeet

$(SMK)/ridersmoke:	lib $(SMK)/ridersmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/ridersmoke   \
			                    $(SMK)/ridersmoke.o \
			                    $(COM)/math.o       \
			                    $(libs) -lbeet

$(STRS)/riderstress:	lib $(STRS)/riderstress.o \
			$(COM)/math.o $(COM)/bench.o $(COM)/cmd.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(STRS)/riderstress   \
			                    $(STRS)/riderstress.o \
			                    $(COM)/math.o        \
			                    $(COM)/bench.o       \
			                    $(COM)/cmd.o         \
			                    $(libs) -lbeet
# Tools
$(TOOLS)/readkeys:	$(DEP) lib $(TOOLS)/readkeys.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(TOOLS)/readkeys $(TOOLS)/readkeys.o

# Clean up
clean:
	rm -f $(SRC)/*.o
	rm -f $(SMK)/*.o
	rm -f $(STRS)/*.o
	rm -f $(COM)/*.o
	rm -f $(TOOLS)/*.o
	rm -f $(LOG)/*.log
	rm -f $(TOOLS)/readkeys
	rm -f $(RSC)/*.bin
	rm -f $(SMK)/pagesmoke
	rm -f $(SMK)/ridersmoke
	rm -f $(STRS)/riderstress
	rm -f $(OUTLIB)/libbeet.so

