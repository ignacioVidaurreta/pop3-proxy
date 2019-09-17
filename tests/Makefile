CC = gcc
CFLAGS = -pthread
BLUE = \e[34m
NORMAL = \e[0m

all: clean
	@mkdir tmp
	@mv ../test_zip.tar .
	@tar -C tmp -xvf test_zip.tar
	@rm tmp/main.c
	$(CC) $(CFLAGS) AllTests.c CuTest.c config_tests.c parser_tests.c tmp/config.c
	@echo "$(BLUE)"
	@./a.out
	@echo "$(NORMAL)" 
	@$(MAKE) clean

clean:
	rm -rf tmp test_zip.tar a.out
