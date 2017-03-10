bt_lexer: bt_parser;
	re2c -o bt_lexer.cpp bt_lexer.re
	mv bt_lexer.cpp ..
	cp bt_lexer.h ..

bt_parser:
	lemon bt_parser.y -s | true
	mv bt_parser.c ../bt_parser.cpp
	mv bt_parser.h ..
