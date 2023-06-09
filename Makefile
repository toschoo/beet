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
LIB = lib/libbeet.so
libs=-lm -ldl -lpthread -ltsalgo

SRC = src/beet
HDR = include/beet
TST = test
COM = common
SMK = test/smoke
STRS = test/stress
TOOLS = tools
BENCH = bench
BIN = bin
LOG = log
RSC = rsc
OUTLIB = lib

OBJ = $(SRC)/lock.o   \
      $(SRC)/error.o  \
      $(SRC)/page.o   \
      $(SRC)/rider.o  \
      $(SRC)/ins.o    \
      $(SRC)/node.o   \
      $(SRC)/tree.o   \
      $(SRC)/config.o \
      $(SRC)/iter.o   \
      $(SRC)/index.o  \
      $(SRC)/version.o

DEP = $(HDR)/types.h   \
      $(SRC)/lock.h    \
      $(SRC)/page.h    \
      $(SRC)/rider.h   \
      $(SRC)/node.h    \
      $(SRC)/ins.h     \
      $(SRC)/tree.h    \
      $(SRC)/iterimp.h \
      $(HDR)/config.h  \
      $(HDR)/iter.h    \
      $(HDR)/index.h

default:	lib 

all:	default tests bench tools

tools:	bin/beet

tests:	smoke stress

bench: $(BIN)/writebench \
       $(BIN)/readbench

smoke:	$(SMK)/pagesmoke     \
	$(SMK)/ridersmoke    \
	$(SMK)/nodesmoke     \
	$(SMK)/treesmoke     \
	$(SMK)/embeddedsmoke \
	$(SMK)/contreesmoke  \
	$(SMK)/indexsmoke    \
	$(SMK)/hostsmoke     \
	$(SMK)/rscsmoke      \
	$(SMK)/rangesmoke    \
	$(SMK)/range2smoke

stress: $(STRS)/riderstress

debug:	CFLAGS += -g
debug:	default
debug:	tools

install:	$(OUTLIB)/libbeet.so $(BIN)/beet
		cp $(OUTLIB)/libbeet.so /usr/local/lib/
		cp -r include/beet /usr/local/include/
		cp bin/beet /usr/local/bin

run:	

flags:
	$(FLGMSG)

.SUFFIXES: .c .o

.c.o:	$(DEP)
	$(CMPMSG)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

.cpp.o:	$(DEP)
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
			                    $(LIB)              \
			                    $(libs)

$(SMK)/ridersmoke:	lib $(SMK)/ridersmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/ridersmoke   \
			                    $(SMK)/ridersmoke.o \
			                    $(COM)/math.o       \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/nodesmoke:	lib $(SMK)/nodesmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/nodesmoke   \
			                    $(SMK)/nodesmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/treesmoke:	lib $(SMK)/treesmoke.o $(COM)/math.o 
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/treesmoke   \
			                    $(SMK)/treesmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/embeddedsmoke:	lib $(SMK)/embeddedsmoke.o $(COM)/math.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/embeddedsmoke  \
			                    $(SMK)/embeddedsmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/contreesmoke:	lib $(SMK)/contreesmoke.o $(COM)/cmd.o \
			                          $(COM)/bench.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/contreesmoke  \
			                    $(SMK)/contreesmoke.o \
			                    $(COM)/cmd.o      \
			                    $(COM)/bench.o    \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/indexsmoke:	lib $(SMK)/indexsmoke.o $(COM)/math.o 
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/indexsmoke   \
			                    $(SMK)/indexsmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

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
			                    $(LIB)              \
			                    $(libs)

$(SMK)/rscsmoke:	lib $(SMK)/rscsmoke.o $(COM)/math.o \
			$(OUTLIB)/libcmp.so
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/rscsmoke   \
			                    $(SMK)/rscsmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/rangesmoke:	lib $(SMK)/rangesmoke.o $(COM)/math.o \
			$(OUTLIB)/libcmp.so
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/rangesmoke   \
			                    $(SMK)/rangesmoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(SMK)/range2smoke:	lib $(SMK)/range2smoke.o $(COM)/math.o \
			$(OUTLIB)/libcmp.so
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(SMK)/range2smoke   \
			                    $(SMK)/range2smoke.o \
			                    $(COM)/math.o      \
			                    $(LIB)              \
			                    $(libs)

$(STRS)/riderstress:	lib $(STRS)/riderstress.o \
			$(COM)/math.o $(COM)/bench.o $(COM)/cmd.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(STRS)/riderstress   \
			                    $(STRS)/riderstress.o \
			                    $(COM)/math.o        \
			                    $(COM)/bench.o       \
			                    $(COM)/cmd.o         \
			                    $(LIB)              \
			                    $(libs)

# Bench
$(BIN)/writebench:	lib $(BENCH)/writebench.o \
			$(OUTLIB)/libcmp.so $(COM)/bench.o $(COM)/cmd.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(BIN)/writebench   \
			$(BENCH)/writebench.o \
			$(COM)/bench.o       \
			$(COM)/cmd.o         \
			$(LIB)              \
			$(libs)

$(BIN)/readbench:	lib $(BENCH)/readbench.o \
			$(OUTLIB)/libcmp.so $(COM)/bench.o $(COM)/cmd.o
			$(LNKMSG)
			$(CC) $(LDFLAGS) -o $(BIN)/readbench \
			$(BENCH)/readbench.o \
			$(COM)/bench.o       \
			$(COM)/cmd.o         \
			$(LIB)              \
			$(libs)

# Tools
$(BIN)/beet:	lib $(TOOLS)/beet.o \
		$(COM)/cmd.o
		$(LNKMSG)
		$(CC) $(LDFLAGS) -o $(BIN)/beet \
		$(TOOLS)/beet.o \
		$(COM)/cmd.o    \
		$(LIB)          \
		$(libs)

# Clean up
clean:
	rm -f $(SRC)/*.o
	rm -f $(SMK)/*.o
	rm -f $(STRS)/*.o
	rm -f $(COM)/*.o
	rm -f $(BENCH)/*.o
	rm -f $(TOOLS)/*.o
	rm -f $(LOG)/*.log
	rm -f $(RSC)/*.bin
	rm -f $(RSC)/*.leaf
	rm -f $(RSC)/*.noleaf
	rm -f $(RSC)/roof
	rm -f $(RSC)/config
	rm -rf $(RSC)/idx??
	rm -rf $(RSC)/idx???
	rm -rf $(RSC)/emb
	rm -f $(SMK)/pagesmoke
	rm -f $(SMK)/ridersmoke
	rm -f $(SMK)/nodesmoke
	rm -f $(SMK)/treesmoke
	rm -f $(SMK)/embeddedsmoke
	rm -f $(SMK)/contreesmoke
	rm -f $(SMK)/indexsmoke
	rm -f $(SMK)/hostsmoke
	rm -f $(SMK)/rscsmoke
	rm -f $(SMK)/rangesmoke
	rm -f $(SMK)/range2smoke
	rm -f $(STRS)/riderstress
	rm -f $(BIN)/writebench
	rm -f $(BIN)/readbench
	rm -f $(BIN)/beet
	rm -f $(OUTLIB)/libbeet.so
	rm -f $(OUTLIB)/libcmp.so

