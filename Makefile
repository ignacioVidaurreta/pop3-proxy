CC = gcc
CFLAGS = -pthread -std=c99 -pedantic -Wall
FILES = main.c pop3.c parser.c server.c client.c config.c logger.c buffer.c
GREEN = \e[92m
NORMAL = \e[0m
all:
	@echo "$(GREEN)Compiling ...$(NORMAL)"
	$(CC) $(CFLAGS) $(FILES) -o run
	@echo "$(GREEN)Done!$(NORMAL)"

debug:
	@echo "$(GREEN)Compiling in debug mode ...$(NORMAL)"
	$(CC) -g $(CFLAGS) $(FILES) -o run_debug
	@echo "$(GREEN)Done!$(NORMAL)"


clean:
	@echo "$(GREEN)Cleaning up ...$(NORMAL)"
	rm -f run run_debug tests/a.out
	@echo "$(GREEN)Done!$(NORMAL)"

test: all
	@echo "$(GREEN)Running tests!$(NORMAL)"
	@tar -zcvf test_zip.tar *.c include/*
	mv test_zip.tar tests/
	$(MAKE) -C tests/
	@echo "$(GREEN)Done!$(NORMAL)"

run: all
	@./run

gdb: debug
	gdb run_debug

