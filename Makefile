CC = gcc
CFLAGS = -pthread --std=c99 -pedantic -Wall -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE -ggdb3 
GREEN = \e[92m
NORMAL = \e[0m
FILES=./*.c
EXEC_NAME = pop3filter 
DEBUG_NAME = pop3filter_debug
all: 
	@echo "$(GREEN)Compiling ...$(NORMAL)"
	$(CC) $(CFLAGS) $(FILES) -o $(EXEC_NAME) -lsctp
	@echo "$(GREEN)Done!$(NORMAL)"

strict:
	@echo "$(GREEN)Compiling in STRICT mode ...$(NORMAL)"
	$(CC) $(CFLAGS) -Werror $(FILES) -o $(EXEC_NAME)
	@echo "$(GREEN)Done!$(NORMAL)"

sanitized:
	$(CC) $(CFLAGS) -fsanitize=address -static-libasan $(FILES) -o $(EXEC_NAME) -lsctp

debug:
	@echo "$(GREEN)Compiling in debug mode ...$(NORMAL)"
	$(CC) -g $(CFLAGS) $(FILES) -o $(DEBUG_NAME) -lsctp
	@echo "$(GREEN)Done!$(NORMAL)"


clean:
	@echo "$(GREEN)Cleaning up ...$(NORMAL)"
	rm -f $(EXEC_NAME) $(EXEC_NAME)_debug tests/a.out
	@echo "$(GREEN)Done!$(NORMAL)"

test: all
	@echo "$(GREEN)TESTING IN PROCESS - PLEASE HOLD!$(NORMAL)"
	@echo "$(GREEN)Running tests/$(NORMAL)"
	$(MAKE) -C tests/
	@echo "$(GREEN)Running admin/tests/$(NORMAL)"
	$(MAKE) -C admin/tests 
	@echo "$(GREEN)Done!$(NORMAL)"

run: all
	@./$(EXEC_NAME) localhost

gdb: debug
	gdb --args $(DEBUG_NAME) localhost

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(EXEC_NAME)

