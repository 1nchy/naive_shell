DIR=.
BIN_DIR=$(DIR)/bin
LIB_DIR=$(DIR)/lib
SRC_DIR=$(DIR)/src
INCLUDE_DIR=$(DIR)/include
THIRD_DIR=$(DIR)/third
THIRD_LIB_DIR=$(THIRD_DIR)/lib
MAIN=$(DIR)/main
PROGRAM=$(DIR)/shell

## compile option
CC=g++
CFLAG=
CFLAGS=$(CFLAG)
CFLAGS+=-std=c++2a
EXTENSION=cpp
INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(THIRD_DIR)
LIB_FLAG=-L$(THIRD_LIB_DIR)

## file path
SRCS=$(wildcard $(SRC_DIR)/*.$(EXTENSION))

DEPS_DIR=$(DIR)/deps
DEPS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(DEPS_DIR)/%.d, $(SRCS))

OBJ_DIR=$(DIR)/obj
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRCS))

THIRD_LIB_NAME=proc_status signal_stack trie_tree output file_system
STATIC_LIB_PREFIX=-l
STATIC_LIB_SUFFIX=.a
THIRD_LIB=$(foreach n, $(THIRD_LIB_NAME), $(THIRD_DIR)/$(n))
THIRD_LIB_FLAG=$(foreach n, $(THIRD_LIB_NAME), $(STATIC_LIB_PREFIX)$(n))


## compile target
.PHONY: all $(THIRD_LIB) clean clean_lib

all:$(OBJS) $(THIRD_LIB)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJS) $(LIB_FLAG) $(THIRD_LIB_FLAG)

$(THIRD_LIB):
	@make -C $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAG) -o $@ -c $<

clean:
	@rm -rf $(OBJ_DIR)
	$(foreach n, $(THIRD_LIB), make clean -C $(n);)
