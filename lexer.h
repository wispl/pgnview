#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdio.h>

enum token_type {
	TK_LBRACKET = 1,  // [
	TK_RBRACKET,      // ]
	TK_LPAREN,        // (
	TK_RPAREN,        // )
	TK_LANGLE,        // <
	TK_RANGLE,        // >
	TK_PERIOD,	  // .
	TK_ASTERISK,      // *
	TK_STRING,        // quote delimeted characters
	TK_SYMBOL,        // letter or digited followed by any of these [A-Za-z0-9_+#=:-]
	TK_INTEGER,       // sequence of decimal digits, special case of SYMBOL
	TK_NAG,           // $ followed by digits
	TK_UNKNOWN,
	TK_EOF,
	TK_MAX
};

struct token {
	enum token_type type;
	// max length of symbols & strings is 255 in the standard
	char value[256];
	int len;
};

struct lexer {
	// struct token prev_token;
	struct token curr_token;
	char last_char;
	FILE *file;
};

bool lexer_fopen(struct lexer *lexer, char *filename);
void lexer_fclose(struct lexer *lexer);
void lexer_next_token(struct lexer *lexer);

#endif
