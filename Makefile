COMP=gcc
CFLAGS=-Wall -Wextra -Iinclude -fPIC
SCFLAGS=$(CFLAGS) -shared

INC_DIR=include
SRC_DIR=src
TST_DIR=test
GRP_DIR=graphs

INSTALL_DIR=install
LIB_DIR=$(INSTALL_DIR)/lib
BIN_DIR=$(INSTALL_DIR)/bin
TST_FILES=$(patsubst $(TST_DIR)/%.c,%,$(wildcard $(TST_DIR)/*.c))

.PHONY: check valgrind pthreads graphs install

all: install
		
thread: $(SRC_DIR)/thread.c $(INC_DIR)/thread.h
		echo -n "Compiling the thread Library"
		$(COMP) $(CFLAGS) -o $(INSTALL_DIR)/$@.o -c $<
		$(COMP) $(SCFLAGS) $(INSTALL_DIR)/$@.o -o $(LIB_DIR)/lib$@.so
		rm $(INSTALL_DIR)/$@.o
		
install_dir : 
		mkdir -p ${INSTALL_DIR} ${LIB_DIR} ${BIN_DIR}
	
threads: install_dir $(INC_DIR)/thread.h 
		for test in $(TST_FILES); do \
			echo "Compile thread test $$test"; \
			$(COMP) $(CFLAGS) -o $(BIN_DIR)/$$test  $(TST_DIR)/$$test.c -lthread -L$(LIB_DIR); \
		done
	
pthreads: install_dir $(INC_DIR)/thread.h 
		for test in $(TST_FILES); do \
			echo "Compile pthread test $$test"; \
			$(COMP) -DUSE_PTHREAD $(CFLAGS) -o $(BIN_DIR)/pthread_$$test $(TST_DIR)/$$test.c -lpthread; \
		done

test:   $(LIB_DIR)/libthread.so threads pthreads

check:
	for test in $(TST_FILES); do \
		echo "Excute $$test"; \
		./$(BIN_DIR)/$$test 10 5; \
		./$(BIN_DIR)/pthread_$$test 10 5; \
	done

valgrind:

install: install_dir thread test

graphs:
		mkdir -p $(GRP_DIR)/imgs $(GRP_DIR)/data
		python3 $(GRP_DIR)/draw_script.py

clean :
		rm -r $(INSTALL_DIR) $(GRP_DIR)/imgs $(GRP_DIR)/data 