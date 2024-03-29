DIR=.
SRC_DIR=$(DIR)
INCLUDE_DIR=$(DIR)
LIB_DIR=$(DIR)/../lib
LIB_NAME=proc_status
OBJ_DIR=$(DIR)/../obj/$(LIB_NAME)

## compile option
CC=g++
AR=ar
CFLAGS=-std=c++2a
EXTENSION=cpp
INCLUDE_FLAG=-I$(INCLUDE_DIR)

## file path
SRCS=$(wildcard $(SRC_DIR)/*.$(EXTENSION))
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRCS))
LIB_PREFIX=lib
LIB_SUFFIX=.a
LIB_TARGET=$(LIB_DIR)/$(LIB_PREFIX)$(LIB_NAME)$(LIB_SUFFIX)

## compile target
.PHONY: all clean

all:$(LIB_TARGET)

$(LIB_TARGET):$(OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAG) -o $@ -c $<

clean:
	@rm -rf $(OBJ_DIR)
