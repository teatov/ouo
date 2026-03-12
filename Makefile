MAIN = ouo
LSP = ouols
# release: ENV=RELEASE
ENV = DEBUG

SRC = src
TEST = test

TEST_SRCS=$(wildcard $(TEST)/*.c)
TESTS=$(TEST_SRCS:.c=)

CC = clang
CFLAGS_DEBUG = -g3
CFLAGS_RELEASE = -O3 -Werror
CFLAGS += $(CFLAGS_$(ENV)) -std=c99 \
	-Wall -Wextra -Wconversion -Wmissing-prototypes

MAKEFLAGS += --no-print-directory

all: $(MAIN) $(LSP)

test: $(TESTS)
	for i in $^; do echo; echo $$i; ./$$i || exit 1; done

$(MAIN): $(SRC)/ouo.c $(SRC)/ouo.h
	$(CC) $(CFLAGS) -o $@ $<

$(LSP): $(SRC)/ouols.c $(SRC)/ouo.h
	$(CC) $(CFLAGS) -o $@ $<

$(TEST)/%: $(TEST)/%.c $(TEST)/test.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(MAIN) $(LSP) $(TESTS)

re:
	$(MAKE) clean
	$(MAKE) all

.PHONY: clean re test
