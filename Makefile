CC = gcc
CFLAGS = -pthread -std=c99 -pedantic -Wall
FILES = main.c pop3.c parser.c server.c client.c config.c logger.c
all:
	@echo "Compiling ..."
	$(CC) $(CFLAGS) $(FILES) -o run
	@echo "Done!"

debug:
	@echo "Compiling in debug mode ..."
	$(CC) -g $(CFLAGS) $(FILES) -o run_debug
	@echo "Done!"


clean:
	@echo "Cleaning up ..."
	rm -f run run_debug tests/a.out
	@echo "Done!"

test: all
	@echo "Running tests!"
	@tar -zcvf test_zip.tar *.c include/*
	mv test_zip.tar tests/
	$(MAKE) -C tests/
	@echo "Done!"

run: all
	@./run

gdb: debug
	gdb run_debug

