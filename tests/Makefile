CC = gcc
CFLAGS = -pthread
all: 
	@mkdir tmp
	@tar -C tmp -xvf test_zip.tar
	@rm tmp/main.c
	$(CC) $(CFLAGS) AllTests.c CuTest.c config_tests.c parser_tests.c tmp/*.c
	@./a.out
	@$(MAKE) clean
	@echo "Done!"

clean:
	rm -rf tmp test_zip.tar a.out
