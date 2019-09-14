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
	rm -f run run_debug a.out
	@echo "Done!"

test:
	@echo "Running tests!"
	@$(CC) $(CFLAGS) $(FILES) -o run
	gcc tests/AllTests.c tests/CuTest.c tests/config_tests.c
	@./a.out
	@echo "Done!"


