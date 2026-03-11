MAIN = ouo
LSP = ouols
# release: ENV=prod
ENV = dev

SRC_DIR = src
vpath %.c $(SRC_DIR)
vpath %.h $(SRC_DIR)

CC = clang
CFLAGS_dev = -g3
CFLAGS_prod = -O3 -Werror
CFLAGS += $(CFLAGS_$(ENV)) -std=c99 \
	-Wall -Wextra -Wconversion -Wmissing-prototypes

MAKEFLAGS += --no-print-directory

all: $(MAIN) $(LSP)

$(MAIN): ouo.c ouo.h
	$(CC) $(CFLAGS) -o $@ $<

$(LSP): ouols.c ouo.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(MAIN) $(LSP)

re:
	$(MAKE) clean
	$(MAKE) all

.PHONY: clean re
