CC = clang
CLEAN_COMMAND = rm -rf threadpool threadpool.o bin/* out/*

CFLAGS_BASE = -Wall -Wextra -Wno-deprecated-declarations -pthread
RELEASE_FLAGS = -O3
DEBUG_FLAGS = -O0 -g -fsanitize=thread

ifeq ($(shell uname -s),Darwin)
  PRELOAD = DYLD_INSERT_LIBRARIES
else
  PRELOAD = LD_PRELOAD
endif

ifdef DEBUG
  CFLAGS = $(CFLAGS_BASE) $(DEBUG_FLAGS)
  ifeq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell touch .debug)
  endif
else
  CFLAGS = $(CFLAGS_BASE) $(RELEASE_FLAGS)
  ifneq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND) .debug)
  endif
endif

all: run

run: bin/wrap_malloc.so bin/test
	$(PRELOAD)=$^

bin/test: out/test.o out/async.o out/threadpool.o out/queue.o
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

bin/wrap_malloc.so: wrappings.c
	$(CC) -fpic -shared wrappings.c -o bin/wrap_malloc.so

out/%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

threadpool.c: wrappings.h threadpool.h

async.c: async.h

test.c: async.h

queue.c: queue.h

clean:
	$(CLEAN_COMMAND)