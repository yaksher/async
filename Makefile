CC = clang
CLEAN_COMMAND = rm -rf threadpool threadpool.o bin/* out/*

ifdef DEBUG
  CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -O3 -pthread -g -fsanitize=thread
  ifeq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell touch .debug)
  endif
else
  CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -O3 -pthread 
  ifneq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND) .debug)
  endif
endif

all: run

run: bin/test
	$^

bin/test: out/test.o out/async.o out/threadpool.o out/queue.o
	$(CC) $(CFLAGS) $^ -o $@

out/%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	$(CLEAN_COMMAND)