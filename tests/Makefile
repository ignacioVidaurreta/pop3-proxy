CC = gcc
CFLAGS = -pthread
BLUE = \e[34m
NORMAL = \e[0m
GREEN = \e[92m

all: 
	$(CC) $(CFLAGS) AllTests.c CuTest.c config_tests.c parser_tests.c ../config.c
	@echo "$(BLUE)"
	@./a.out
	@echo "$(NORMAL)" 
	@$(MAKE) clean

clean:
	@rm -rf a.out
