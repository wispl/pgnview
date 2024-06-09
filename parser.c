#include "parser.h"

#include "array.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pgn standard:
// https://ia802908.us.archive.org/26/items/pgn-standard-1994-03-12/PGN_standard_1994-03-12.txt

enum token_type {
	TK_LBRACKET,	// [
	TK_RBRACKET, 	// ]
	TK_LPAREN,	// (
	TK_RPAREN,	// )
	TK_LANGLE,	// <
	TK_RANGLE,	// >
	TK_PERIOD,	// .
	TK_ASTERISK,	// *
	TK_STRING,	// quote delimeted characters
	TK_SYMBOL,	// letter or digited followed by any of these [A-Za-z0-9_+#=:-]
	TK_INTEGER,	// sequence of decimal digits, special case of SYMBOL
	TK_NAG,		// $ followed by digits
	TK_UNKNOWN,     // unparsable tokens
	TK_EOF,         // end of file
	TK_MAX
};

static const char* token_str[TK_MAX] = {
	[TK_LBRACKET] = "[",
	[TK_RBRACKET] = "]",
	[TK_LPAREN]   = "(",
	[TK_RPAREN]   = ")",
	[TK_LANGLE]   = "<",
	[TK_RANGLE]   = ">",
	[TK_PERIOD]   =	".",
	[TK_ASTERISK] = "*",
	[TK_STRING]   = "string",
	[TK_SYMBOL]   = "symbol",
	[TK_INTEGER]  = "integer",
	[TK_NAG]      = "nag",
	[TK_UNKNOWN]  = "unknown",
	[TK_EOF]      = "eof"
};

struct token {
	enum token_type type;   // type of the token
	char value[256]; 	// symbols and strings have max length of 255
	int len;                // length of value, including the null terminator
};

// internal structure for sharing data between parsing functions
struct parser {
	// lexer
	FILE *file;          // file being lexed and parsed
	struct token token;  // current token
	char last_char;

	// parser
	bool unhandled_error;

	// data
	struct pgn *pgn;    // pgn data to fill during parsing
};

//
// Lexer
//

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

// TODO: small buffer of parsed characters for error messages
// TODO: nag tokens
void next_token(struct parser *parser)
{
	// EOF token
	if (parser->last_char == EOF) {
		parser->token.type = TK_EOF;
		return;
	}

	// ignore whitespace
	while (isspace(parser->last_char)) {
		parser->last_char = getc(parser->file);
	}

	// ignore comments
	if (parser->last_char == ';') {
		do {
			parser->last_char = getc(parser->file);
		} while (parser->last_char != EOF &&
			 parser->last_char != '\n' &&
			 parser->last_char != '\r');
	}

	// terminal tokens
	switch (parser->last_char) {
	case '[':  parser->token.type = TK_LBRACKET; break;
	case ']':  parser->token.type = TK_RBRACKET; break;
	case '(':  parser->token.type = TK_LPAREN;   break;
	case ')':  parser->token.type = TK_RPAREN;   break;
	case '<':  parser->token.type = TK_LANGLE;   break;
	case '>':  parser->token.type = TK_RANGLE;   break;
	case '.':  parser->token.type = TK_PERIOD;   break;
	case '*':  parser->token.type = TK_ASTERISK; break;
	default:   parser->token.type = TK_UNKNOWN;
	}
	if (parser->token.type != TK_UNKNOWN) {
		parser->token.value[0] = parser->last_char;
		parser->token.value[1] = '\0';
		parser->token.len = 2;
		parser->last_char = getc(parser->file);
		return;
	}

	// string token
	if (parser->last_char == '"') {
		parser->token.type = TK_STRING;
		int len = 0;
		while ((parser->last_char = getc(parser->file)) != '"') {
			parser->token.value[len] = parser->last_char;
			++len;
		}
		parser->token.value[len] = '\0';
		parser->token.len = len + 1;

		// skip closing quotes
		parser->last_char = getc(parser->file);
		return;
	}

	// symbol token and integer token (special case of symbol token)
	if (isalnum(parser->last_char)) {
		bool all_ints = true;
		int len = 0;
		do {
			all_ints &= (isdigit(parser->last_char) != 0);
			parser->token.value[len] = parser->last_char;
			parser->last_char = getc(parser->file);
			++len;
		} while (is_symbol(parser->last_char));

		parser->token.type = all_ints ? TK_INTEGER : TK_SYMBOL;
		parser->token.value[len] = '\0';
		parser->token.len = len + 1;
		return;
	}

	// unknown tokens
	parser->token.type = TK_UNKNOWN;
	if (isprint(parser->last_char)) {
		parser->token.value[0] = parser->last_char;
		parser->token.value[1] = '\0';
		parser->token.len = 2;
	} else {
		// store control sequences literally
		parser->token.value[0] = '\\';
		parser->token.value[1] = cntrlchr(parser->last_char);
		parser->token.value[2] = '\0';
		parser->token.len = 3;
	}
	parser->last_char = getc(parser->file);
}

//
// Parser
//

static inline bool check(struct parser *parser, enum token_type type)
{
	return parser->token.type == type;
}

static bool expect(struct parser *parser, enum token_type type)
{
	// lexer will be one token ahead of parser after calling this
	if (check(parser, type)) {
		next_token(parser);
		return true;
	}

	parser->unhandled_error = true;
	fprintf(stderr, "[Syntax Error] expected token '%s' found token '%s' with value '%s'\n",
	 	token_str[type],
	 	token_str[parser->token.type],
	 	parser->token.value);
	return false;
}

// copies the value of token to a buffer, the buffer must be freed
static inline void copy_token_value(char **buffer, struct token *token)
{
	*buffer = malloc(token->len);
	memcpy(*buffer, token->value, token->len);
}

// tag is made of the following tokens: "[SYMBOL STRING]"
static void tag(struct parser *parser)
{
	struct pgn_tag tag;

	expect(parser, TK_LBRACKET);

	copy_token_value(&tag.name, &parser->token);
	expect(parser, TK_SYMBOL);

	copy_token_value(&tag.desc, &parser->token);
	expect(parser, TK_STRING);

	expect(parser, TK_RBRACKET);

	if (parser->unhandled_error) {
		fprintf(stderr, "[Parser Error] Unable to parse 'tag' due to errors\n");
		parser->unhandled_error = false;
	} else {
		array_push(&parser->pgn->tags, tag);
	}
}

// TODO: handle comments and NAG tokens
// move is made of the following tokens: "(INTEGER PERIOD+)? SYMBOL SYMBOL"
static void movetext(struct parser *parser)
{
	struct pgn_move white, black;

	// move-indicator, the 1. in (1. e4 e5), is optional
	if (check(parser, TK_INTEGER)) {
		expect(parser, TK_INTEGER);
		// weird, but unlimited periods is permitted by the standard
		do {
			expect(parser, TK_PERIOD);
		} while (check(parser, TK_PERIOD));
	}

	memcpy(&white.text, &parser->token.value, parser->token.len);
	expect(parser, TK_SYMBOL);

	memcpy(&black.text, &parser->token.value, parser->token.len);
	expect(parser, TK_SYMBOL);

	if (parser->unhandled_error) {
		fprintf(stderr, "[Parser Error] Unable to parse 'movetext' due to errors\n");
		parser->unhandled_error = false;
	} else {
		array_push(&parser->pgn->moves, white);
		array_push(&parser->pgn->moves, black);
	}
}

// TODO: error codes
void pgn_read(struct pgn* pgn, char* filename)
{
	// initialization
	struct parser parser = {
		.file  = fopen(filename, "r"),
		.last_char = ' ',
		.pgn = pgn
	};
	array_init(&pgn->tags, sizeof(struct pgn_tag));
	array_init(&pgn->moves, sizeof(struct pgn_move));

	if (parser.file == NULL) {
		return;
	}

	// parsing
	next_token(&parser);
	while (parser.token.type != TK_EOF) {
		switch (parser.token.type) {
		case TK_LBRACKET: tag(&parser);  break;
		case TK_INTEGER:  movetext(&parser); break;
		case TK_SYMBOL:	  movetext(&parser); break;
		default: 	  next_token(&parser);
		}
	}

	// finalization
	// Delete last move as it provides the result of the game
	char *result = array_get(&pgn->moves, struct pgn_move, pgn->moves.len - 1).text;
	memcpy(pgn->result, result, sizeof(char) * 8);
	array_pop(&pgn->moves);

	// cleanup
	fclose(parser.file);
}

void pgn_free(struct pgn *pgn)
{
	if (pgn == NULL) {
		return;
	}
	for (int i = 0; i < pgn->tags.len; ++i) {
		free(array_get(&pgn->tags, struct pgn_tag, i).name);
		free(array_get(&pgn->tags, struct pgn_tag, i).desc);
	}
	array_free(&pgn->tags);
	array_free(&pgn->moves);
}
