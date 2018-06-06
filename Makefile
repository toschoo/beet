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
libs=-lm -ldl -lpthread -ltsalgo

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

OBJ = $(SRC)/lock.o  \
      $(SRC)/error.o \
      $(SRC)/page.o  \
      $(SRC)/rider.o \
      $(SRC)/ins.o   \
      $(SRC)/node.o  \
      $(SRC)/tree.o  \
      $(SRC)/index.o

DEP = $(HDR)/types.h \
      $(SRC)/lock.h  \
      $(SRC)/page.h  \
      $(SRC)/rider.h \
      $(SRC)/node.h  \
      $(SRC)/ins.h   \
      $(SRC)/tree.h  \
      $(HDR)/index.h

default:	lib 

all:	default tests tools

tools:	

tests:	smoke stress

smoke:	$(SMK)/pagesmoke     \
	$(SMK)/ridersmoke    \
	$(SMK)/nodesmoke     \
	$(SMK)/treesmoke     \
	$(SMK)/embeddedsmoke \
	$(SMK)/contreesmoke  \
	$(SMK)/indexsmoke    \
	$(SMK)/hostsmoke

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

$(SMK)/nodesmoke:	lib $(SMK)/nodesmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/nodesmoke   \
			                    $(SMK)/nodesmoke.o \
			                    $(COM)/math.o      \
			                    $(libs) -lbeet

$(SMK)/treesmoke:	lib $(SMK)/treesmoke.o $(COM)/math.o 
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/treesmoke   \
			                    $(SMK)/treesmoke.o \
			                    $(COM)/math.o      \
			                    $(libs) -lbeet

$(SMK)/embeddedsmoke:	lib $(SMK)/embeddedsmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/embeddedsmoke  \
			                    $(SMK)/embeddedsmoke.o \
			                    $(COM)/math.o      \
			                    $(libs) -lbeet

$(SMK)/contreesmoke:	lib $(SMK)/contreesmoke.o $(COM)/cmd.o \
			                          $(COM)/bench.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/contreesmoke  \
			                    $(SMK)/contreesmoke.o \
			                    $(COM)/cmd.o      \
			                    $(COM)/bench.o    \
			                    $(libs) -lbeet

$(SMK)/indexsmoke:	lib $(SMK)/indexsmoke.o $(COM)/math.o 
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/indexsmoke   \
			                    $(SMK)/indexsmoke.o \
			                    $(COM)/math.o      \
			                    $(libs) -lbeet

$(OUTLIB)/libcmp.so:	$(COM)/cmp.o
			$(LNKMSG)
			$(CC) -shared \
			      -o $(OUTLIB)/libcmp.so \
			         $(COM)/cmp.o

$(SMK)/hostsmoke:	lib $(SMK)/hostsmoke.o $(COM)/math.o \
			$(OUTLIB)/libcmp.so
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/hostsmoke   \
			                    $(SMK)/hostsmoke.o \
			                    $(COM)/math.o      \
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
	rm -f $(RSC)/*.leaf
	rm -f $(RSC)/*.noleaf
	rm -f $(RSC)/roof
	rm -f $(RSC)/config
	rm -rf $(RSC)/idx??
	rm -rf $(RSC)/emb
	rm -f $(SMK)/pagesmoke
	rm -f $(SMK)/ridersmoke
	rm -f $(SMK)/nodesmoke
	rm -f $(SMK)/treesmoke
	rm -f $(SMK)/embeddedsmoke
	rm -f $(SMK)/contreesmoke
	rm -f $(SMK)/indexsmoke
	rm -f $(SMK)/hostsmoke
	rm -f $(STRS)/riderstress
	rm -f $(OUTLIB)/libbeet.so
	rm -f $(OUTLIB)/libcmp.so

