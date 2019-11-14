NORMAL=\e[0m
GREEN=\e[92m
EXEC_NAME=stripmime
all:
	gcc-8 -ggdb3 -Wall -o  $(EXEC_NAME) mime_chars.c mime_msg.c parser_utils.c parser.c pop3_multi.c stripmime.c mime_value.c mime_boundary_key.c mime_boundary_border_end.c mime_type.c mime_body.c
	cp $(EXEC_NAME) ..
	@echo "$(GREEN)Done!$(NORMAL)"

clean:
	$(RM) $(EXEC_NAME)

run:
	./$(EXEC_NAME)
