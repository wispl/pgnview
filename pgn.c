#include "pgn.h"

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
	TK_SYMBOL,	// letter or digits followed by any of [A-Za-z0-9_+#=:-]
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

static const char *syntax_err =
	"Error(Syntax) |%d, col %d|: expected token '%s' "
	"but found token '%s' with value '%s'\n";

static const char *parser_err =
	"Error(Parser) |%d, col %d|: error occured trying to parse '%s'\n";

struct token {
	enum token_type type; // type of the token
	char value[256];      // symbols and strings have max length of 255
	int len;              // length of value, including the null terminator
};

// internal structure for sharing data between parsing functions
struct parser {
	// lexer
	FILE *file;
	struct token token;
	char last_char;

	// parser
	int error_buf_idx;
	bool unhandled_error;
	int y, x;

	// data
	struct pgn *pgn;
};

//
// Lexer
//

static inline bool is_symbol(char c)
{
	return isalnum(c) || c == '_' || c == '+' || c == '#'
			  || c == '=' || c == ':' || c == '-';
}

static char next_char(struct parser *parser)
{
	if (parser->last_char == '\n') {
		++parser->y;
		parser->x = 0;
	}
	++parser->x;
	parser->last_char = getc(parser->file);
	return parser->last_char;
}

// TODO: small buffer of parsed characters for error messages
// TODO: nag tokens
static void next_token(struct parser *parser)
{
	// EOF token
	if (parser->last_char == EOF) {
		parser->token.type = TK_EOF;
		return;
	}

	// ignore whitespace
	while (isspace(parser->last_char)) {
		next_char(parser);
	}

	// ignore comments
	if (parser->last_char == ';') {
		do {
			next_char(parser);
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
		next_char(parser);
		return;
	}

	// string token
	if (parser->last_char == '"') {
		parser->token.type = TK_STRING;
		int len = 0;
		while ((next_char(parser)) != '"') {
			parser->token.value[len] = parser->last_char;
			++len;
		}
		parser->token.value[len] = '\0';
		parser->token.len = len + 1;

		// skip closing quotes
		next_char(parser);
		return;
	}

	// symbol token and integer token (special case of symbol token)
	if (isalnum(parser->last_char)) {
		bool all_ints = true;
		int len = 0;
		do {
			all_ints &= (isdigit(parser->last_char) != 0);
			parser->token.value[len] = parser->last_char;
			next_char(parser);
			++len;
		} while (is_symbol(parser->last_char));

		parser->token.type = all_ints ? TK_INTEGER : TK_SYMBOL;
		parser->token.value[len] = '\0';
		parser->token.len = len + 1;
		return;
	}

	// unknown tokens
	parser->token.type = TK_UNKNOWN;
	parser->token.value[0] = parser->last_char;
	parser->token.value[1] = '\0';
	parser->token.len = 2;

	next_char(parser);
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
	if (check(parser, type)) {
		next_token(parser);
		return true;
	}

	parser->unhandled_error = true;

	fprintf(stderr, syntax_err,
		parser->y,
		parser->x,
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
	int x = parser->x;
	int y = parser->y;

	struct pgn_tag tag;

	expect(parser, TK_LBRACKET);

	copy_token_value(&tag.name, &parser->token);
	expect(parser, TK_SYMBOL);

	copy_token_value(&tag.desc, &parser->token);
	expect(parser, TK_STRING);

	expect(parser, TK_RBRACKET);

	if (parser->unhandled_error) {
		fprintf(stderr, parser_err, y, x, "tag");
		parser->unhandled_error = false;
	} else {
		array_push(&parser->pgn->tags, tag);
	}
}

// TODO: handle comments and NAG tokens
// Move is made of the following tokens: "(INTEGER PERIOD+)? SYMBOL"
// The "(Integer PERIOD+)?" portion is known as the move indicator
// and is optional for imports.
static void movetext(struct parser *parser)
{
	int x = parser->x;
	int y = parser->y;

	struct pgn_move move;

	if (check(parser, TK_INTEGER)) {
		expect(parser, TK_INTEGER);
		// unlimited periods is permitted by the standard: "1..."
		do {
			expect(parser, TK_PERIOD);
		} while (check(parser, TK_PERIOD));
	}

	memcpy(&move.text, &parser->token.value, parser->token.len);
	expect(parser, TK_SYMBOL);

	if (parser->unhandled_error) {
		fprintf(stderr, parser_err, y, x, "move");
		parser->unhandled_error = false;
	} else {
		array_push(&parser->pgn->moves, move);
	}
}

// TODO: error codes
void pgn_read(struct pgn* pgn, char* filename)
{
	// initialization
	struct parser parser = {
		.file  = fopen(filename, "r"),
		.last_char = ' ',
		.error_buf_idx = 0,
		.y = 1,
		.x = 0,
		.pgn = pgn
	};
	array_init(&pgn->tags);
	array_init(&pgn->moves);

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
	char *result = array_last(&pgn->moves).text;
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
		free(array_get(&pgn->tags, i).name);
		free(array_get(&pgn->tags, i).desc);
	}
	array_free(&pgn->tags);
	array_free(&pgn->moves);
}
