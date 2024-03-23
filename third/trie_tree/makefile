DIR=.
SRC_DIR=$(DIR)
INCLUDE_DIR=$(DIR)
OBJ_DIR=$(DIR)
LIB_DIR=$(DIR)/../lib
LIB_NAME=trie_tree

## compile option
CC=g++
AR=ar
CFLAGS=-std=c++2a
EXTENSION=cpp
INCLUDE=-I$(INCLUDE_DIR)

## file path
SRCS=$(wildcard $(SRC_DIR)/*.$(EXTENSION))
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRCS))
LIB_PREFIX=lib
LIB_SUFFIX=.a
LIB_TARGET=$(LIB_DIR)/$(LIB_PREFIX)$(LIB_NAME)$(LIB_SUFFIX)

## compile target
.PHONY: all

all:$(LIB_TARGET)

$(LIB_TARGET):$(OBJS)
	$(AR) rcs $@ $^
	rm -f $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<
