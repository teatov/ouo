NAME := ouo

SRC_DIR := src
OBJ_DIR := obj
SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC := clang
CFLAGS := -std=c99 -g3 -Wall -Wextra -Werror
MAKEFLAGS += --no-print-directory

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(NAME)

re:
	$(MAKE) clean
	$(MAKE) all

.PHONY: clean re
