COMP=gcc
CFLAGS=-Wall -Wextra -Iinclude -fPIC
SCFLAGS=$(CFLAGS) -shared

INC_FOLDER=include
SRC_FOLDER=src
TST_FOLDER=test

INSTALL_FOLDER=install
LIB_FOLDER=$(INSTALL_FOLDER)/lib
BIN_FOLDER=$(INSTALL_FOLDER)/bin

all: install

install/thread.o: src/thread.c
		$(COMP) $(CFLAGS) -o $@ -c $^

libthread.so: $(INSTALL_FOLDER)/thread.o
		$(COMP) $(SCFLAGS) $^ -o $(LIB_FOLDER)/$@

install/bin/%: test/%.c libthread.so
		$(COMP) $(CFLAGS) -o $@  $^ -lthread -L$(LIB_FOLDER) && ./$(BIN_FOLDER)/$@

test: install $(BIN_FOLDER)/%

install : 
		mkdir ${INSTALL_FOLDER} ${LIB_FOLDER} ${BIN_FOLDER}
		


clean :
		rm -r install