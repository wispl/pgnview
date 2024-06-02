#include "lexer.h"

#include <ctype.h>

static inline bool is_symbol(char c)
{
	return isalnum(c) || c == '_' || c == '+' || c == '#'
			  || c == '=' || c == ':' || c == '-';
}

bool lexer_fopen(struct lexer *lexer, char *filename)
{
	lexer_fclose(lexer);
	lexer->file = fopen(filename, "r");
	lexer->last_char = ' ';
	if (lexer->file == NULL) {
		return false;
	}
	return true;
}

void lexer_fclose(struct lexer *lexer)
{
	if (lexer->file != NULL) {
		fclose(lexer->file);
	}
}

// TODO: small buffer of parsed characters for error messages
void lexer_next_token(struct lexer *lexer)
{
	// ignore whitespace
	while (isspace(lexer->last_char)) {
		lexer->last_char = getc(lexer->file);
	}

	// ignore comments
	if (lexer->last_char == ';') {
		do {
			lexer->last_char = getc(lexer->file);
		}
		while (lexer->last_char != EOF &&
		       lexer->last_char != '\n' &&
		       lexer->last_char != '\r');
	}

	switch (lexer->last_char) {
	case '[':  lexer->curr_token.type = TK_LBRACKET; lexer->last_char = getc(lexer->file); return;
	case ']':  lexer->curr_token.type = TK_RBRACKET; lexer->last_char = getc(lexer->file); return;
	case '(':  lexer->curr_token.type = TK_LPAREN;   lexer->last_char = getc(lexer->file); return;
	case ')':  lexer->curr_token.type = TK_RPAREN;   lexer->last_char = getc(lexer->file); return;
	case '<':  lexer->curr_token.type = TK_LANGLE;   lexer->last_char = getc(lexer->file); return;
	case '>':  lexer->curr_token.type = TK_RANGLE;   lexer->last_char = getc(lexer->file); return;
	case '.':  lexer->curr_token.type = TK_PERIOD;   lexer->last_char = getc(lexer->file); return;
	}

	if (lexer->last_char == '"') {
		lexer->curr_token.type = TK_STRING;
		int len = 0;
		do {
			// skip opening quotes
			lexer->last_char = getc(lexer->file);
			lexer->curr_token.value[len] = lexer->last_char;
			++len;
		} while (lexer->last_char != '"');
		// skip closing quotes
		lexer->last_char = getc(lexer->file);
		--len;

		lexer->curr_token.value[len] = '\0';
		lexer->curr_token.len = len;
		return;
	}

	if (isalnum(lexer->last_char)) {
		lexer->curr_token.type = TK_SYMBOL;
		bool all_ints = true;
		int len = 0;
		do {
			all_ints &= (isdigit(lexer->last_char) != 0);
			lexer->curr_token.value[len] = lexer->last_char;
			lexer->last_char = getc(lexer->file);
			++len;
		} while (is_symbol(lexer->last_char));
		lexer->curr_token.value[len] = '\0';
		lexer->curr_token.len = len;

		// INTEGER tokens are a special case of SYMBOL tokens
		if (all_ints) {
			lexer->curr_token.type = TK_INTEGER;
		}
		return;
	}

	if (lexer->last_char == EOF) {
		lexer->curr_token.type = TK_EOF;
		return;
	}

	lexer->last_char = getc(lexer->file);
	lexer->curr_token.type = TK_UNKNOWN;
	if (lexer->last_char == '\n') {
		lexer->curr_token.value[0] = '\\';
		lexer->curr_token.value[1] = 'n';
		lexer->curr_token.value[2] = '\0';
	} else {
		lexer->curr_token.value[0] = lexer->last_char;
		lexer->curr_token.value[1] = '\0';
	}
	lexer->curr_token.len = 1;
}
