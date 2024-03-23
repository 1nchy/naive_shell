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
CFLAGS=-std=c++2a
DEBUG_FLAG=-g
EXTENSION=cpp
INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(THIRD_DIR)
LIB_FLAG=-L$(THIRD_LIB_DIR)

## file path
SRCS=$(wildcard $(SRC_DIR)/*.$(EXTENSION))

DEPS_DIR=$(DIR)/deps
DEPS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(DEPS_DIR)/%.d, $(SRCS))

OBJ_DIR=$(DIR)/obj
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRCS))
DEBUG_OBJ_DIR=$(DIR)/debug_obj
DEBUG_OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(DEBUG_OBJ_DIR)/%.o, $(SRCS))

THIRD_LIB_NAME=proc_status signal_stack trie_tree
STATIC_LIB_PREFIX=-l
STATIC_LIB_SUFFIX=.a
THIRD_LIB=$(foreach n, $(THIRD_LIB_NAME), $(THIRD_DIR)/$(n))
# THIRD_LIB_FLAG=$(foreach n, $(THIRD_LIB_NAME), $(STATIC_LIB_PREFIX)$(n)$(STATIC_LIB_SUFFIX))
THIRD_LIB_FLAG=$(foreach n, $(THIRD_LIB_NAME), $(STATIC_LIB_PREFIX)$(n))


## compile target
.PHONY: all $(THIRD_LIB)

all:$(OBJS) $(THIRD_LIB)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJS) $(LIB_FLAG) $(THIRD_LIB_FLAG)

debug:$(DEBUG_OBJS) $(THIRD_LIB)
	$(CC) $(DEBUG_FLAG) $(CFLAGS) -o $(PROGRAM) $(DEBUG_OBJS) $(LIB_FLAG) $(THIRD_LIB_FLAG)

$(THIRD_LIB):
	@make -C $@

$(DEPS_DIR)/%.d: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(DEPS_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAG) -MM $^ | sed 1's,^,$@ $(OBJ_DIR)/,' > $@

# include $(DEPS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAG) -o $@ -c $<

$(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(DEBUG_FLAG) $(CFLAGS) $(INCLUDE_FLAG) -o $@ -c $<

clean:
	@rm -rf $(OBJ_DIR)
	@rm -rf $(DEBUG_OBJ_DIR)
	@rm -rf $(DEPS_DIR)