CC = gcc
CFLAGS = -pthread
BLUE = \e[34m
NORMAL = \e[0m

all: 
	$(CC) $(CFLAGS) AllTests.c CuTest.c linked_list_test.c ../linked_list.c
	@echo "$(BLUE)"
	@./a.out
	@echo "$(NORMAL)" 

