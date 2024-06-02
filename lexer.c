#include "lexer.h"

#include <ctype.h>

static inline bool is_symbol(char c)
{
	return isalnum(c) || c == '_' || c == '+' || c == '#'
			  || c == '=' || c == ':' || c == '-';
}

static char cntrlchr(char c)
{
	switch (c) {
	case '\a': return 'a';
	case '\b': return 'b';
	case '\e': return 'e';
	case '\n': return 'n';
	case '\r': return 'r';
	case '\t': return 't';
	}
	return '?';
}

bool lexer_fopen(struct lexer *lexer, char *filename)
{
	// close previously opened file
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
// TODO: nag tokens
void lexer_next_token(struct lexer *lexer)
{
	// EOF token
	if (lexer->last_char == EOF) {
		lexer->curr_token.type = TK_EOF;
		return;
	}

	// ignore whitespace
	while (isspace(lexer->last_char)) {
		lexer->last_char = getc(lexer->file);
	}

	// ignore comments
	if (lexer->last_char == ';') {
		do {
			lexer->last_char = getc(lexer->file);
		} while (lexer->last_char != EOF &&
			 lexer->last_char != '\n' &&
			 lexer->last_char != '\r');
	}

	// terminal tokens
	switch (lexer->last_char) {
	case '[':  lexer->curr_token.type = TK_LBRACKET; break;
	case ']':  lexer->curr_token.type = TK_RBRACKET; break;
	case '(':  lexer->curr_token.type = TK_LPAREN;   break;
	case ')':  lexer->curr_token.type = TK_RPAREN;   break;
	case '<':  lexer->curr_token.type = TK_LANGLE;   break;
	case '>':  lexer->curr_token.type = TK_RANGLE;   break;
	case '.':  lexer->curr_token.type = TK_PERIOD;   break;
	case '*':  lexer->curr_token.type = TK_ASTERISK; break;
	default:   lexer->curr_token.type = TK_UNKNOWN;
	}
	if (lexer->curr_token.type != TK_UNKNOWN) {
		lexer->curr_token.value[0] = lexer->last_char;
		lexer->curr_token.value[1] = '\0';
		lexer->curr_token.len = 2;
		lexer->last_char = getc(lexer->file);
		return;
	}

	// string token
	if (lexer->last_char == '"') {
		lexer->curr_token.type = TK_STRING;
		int len = 0;
		while ((lexer->last_char = getc(lexer->file)) != '"') {
			lexer->curr_token.value[len] = lexer->last_char;
			++len;
		}
		lexer->curr_token.value[len] = '\0';
		lexer->curr_token.len = len;

		// skip closing quotes
		lexer->last_char = getc(lexer->file);
		return;
	}

	// symbol token and integer token (special case of symbol token)
	if (isalnum(lexer->last_char)) {
		bool all_ints = true;
		int len = 0;
		do {
			all_ints &= (isdigit(lexer->last_char) != 0);
			lexer->curr_token.value[len] = lexer->last_char;
			lexer->last_char = getc(lexer->file);
			++len;
		} while (is_symbol(lexer->last_char));

		lexer->curr_token.type = all_ints ? TK_INTEGER : TK_SYMBOL;
		lexer->curr_token.value[len] = '\0';
		lexer->curr_token.len = len;
		return;
	}

	// all other tokens
	lexer->curr_token.type = TK_UNKNOWN;
	if (isprint(lexer->last_char)) {
		lexer->curr_token.value[0] = lexer->last_char;
		lexer->curr_token.value[1] = '\0';
		lexer->curr_token.len = 2;
	} else {
		lexer->curr_token.value[0] = '\\';
		lexer->curr_token.value[1] = cntrlchr(lexer->last_char);
		lexer->curr_token.value[2] = '\0';
		lexer->curr_token.len = 3;
	}
	lexer->last_char = getc(lexer->file);
}
