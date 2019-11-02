CC = gcc
CFLAGS = -pthread -std=c99 -pedantic -Wall -fsanitize=address -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE
GREEN = \e[92m
NORMAL = \e[0m
FILES=./*.c
EXEC_NAME = run
all: 
	@echo "$(GREEN)Compiling ...$(NORMAL)"
	$(CC) $(CFLAGS) $(FILES) -o $(EXEC_NAME)
	@echo "$(GREEN)Done!$(NORMAL)"

strict:
	@echo "$(GREEN)Compiling in STRICT mode ...$(NORMAL)"
	$(CC) $(CFLAGS) -Werror $(FILES) -o $(EXEC_NAME)
	@echo "$(GREEN)Done!$(NORMAL)"

debug:
	@echo "$(GREEN)Compiling in debug mode ...$(NORMAL)"
	$(CC) -g $(CFLAGS) $(FILES) -o $(EXEC_NAME)_debug
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
	@./$(EXEC_NAME)

gdb: debug
	gdb $(EXEC_NAME)_debug

