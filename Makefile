COMP=gcc
CFLAGS=-Wall -Wextra -Iinclude -fPIC
SCFLAGS=$(CFLAGS) -shared

INC_DIR=include
SRC_DIR=src
TST_DIR=test

INSTALL_DIR=install
LIB_DIR=$(INSTALL_DIR)/lib
BIN_DIR=$(INSTALL_DIR)/bin
TST_FILES=$(shell ls $(TST_DIR) | sed 's/\.c//g')

.PHONY: thread 

all: install
		
thread: $(SRC_DIR)/thread.c $(INC_DIR)/thread.h
		echo -n "Compiling the thread Library"
		$(COMP) $(CFLAGS) -o $(INSTALL_DIR)/$@.o -c $<
		$(COMP) $(SCFLAGS) $(INSTALL_DIR)/$@.o -o $(LIB_DIR)/lib$@.so
		rm $(INSTALL_DIR)/$@.o

%: test/%.c 
		echo "Compile the test $@";
		$(COMP) $(CFLAGS) -o $(BIN_DIR)/$@  $< -lthread -L$(LIB_DIR) 

install_dir : 
		mkdir -p ${INSTALL_DIR} ${LIB_DIR} ${BIN_DIR}

install: install_dir thread 

test:   $(LIB_DIR)/libthread.so $(TST_FILES)

clean :
		rm -r install