all:
	gcc-8 -ggdb3 -Wall -o mime_filter mime_chars.c mime_msg.c parser_utils.c parser.c pop3_multi.c stripmime.c mime_value.c mime_boundary_key.c mime_boundary_border_end.c mime_type.c mime_body.c

clean:
	$(RM) mime_filter

run:
	./mime_filter
