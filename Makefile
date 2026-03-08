NAME := ouo
# release: ENV=prod
ENV := dev

SRC_DIR := src
BUILD_DIR := build
BUILD_ENV_DIR := $(BUILD_DIR)/$(ENV)
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_ENV_DIR)/%.o)

CC := clang
CFLAGS.dev := -g3 -Og
CFLAGS.prod := -O3 -Werror
CFLAGS := $(CFLAGS.$(ENV)) -std=c99 \
	-Wall -Wextra -Wconversion -Wmissing-prototypes

MAKEFLAGS += --no-print-directory

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME)

$(BUILD_ENV_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(NAME)

re:
	$(MAKE) clean
	$(MAKE) all

.PHONY: clean re
