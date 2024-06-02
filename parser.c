#include "parser.h"

#include "lexer.h"
#include "list.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

// copies the value of token to a buffer, the buffer must be freed
static inline void copy_token_value(char **buffer, struct token *token)
{
	*buffer = malloc(token->len);
	memcpy(*buffer, token->value, token->len);
}

static inline void parser_next(struct parser *parser)
{
	lexer_next_token(&parser->lexer);
}

static inline bool parser_check(struct parser *parser, enum token_type type)
{
	return parser->lexer.curr_token.type == type;
}

static bool parser_expect(struct parser *parser, enum token_type type)
{
	// lexer will be one token ahead of parser after calling this
	if (parser_check(parser, type)) {
		parser_next(parser);
		return true;
	}
	fprintf(stderr, "[Syntax Error] expected token '%s' found token '%s' with value '%s'\n",
	 	token_str[type],
	 	token_str[parser->lexer.curr_token.type],
	 	parser->lexer.curr_token.value);

	return false;
}

static void parser_tag(struct parser *parser)
{
	struct tag *tag = malloc(sizeof(struct tag));

	parser_expect(parser, TK_LBRACKET);

	copy_token_value(&tag->type, &parser->lexer.curr_token);
	parser_expect(parser, TK_SYMBOL);
	copy_token_value(&tag->description, &parser->lexer.curr_token);
	parser_expect(parser, TK_STRING);

	parser_expect(parser, TK_RBRACKET);

	list_add(&parser->pgn->tags, &tag->node);
}

static void parser_move(struct parser *parser)
{
	struct move *move = malloc(sizeof(struct move));

	if (parser_check(parser, TK_INTEGER)) {
		parser_expect(parser, TK_INTEGER);
		// weird, but unlimited periods is permitted by the standard
		do {
			parser_expect(parser, TK_PERIOD);
		} while (parser_check(parser, TK_PERIOD));
	}

	copy_token_value(&move->white, &parser->lexer.curr_token);
	parser_expect(parser, TK_SYMBOL);

	copy_token_value(&move->black, &parser->lexer.curr_token);
	parser_expect(parser, TK_SYMBOL);

	list_add(&parser->pgn->moves, &move->node);
}

struct pgn* pgn_read(char* filename)
{
	// initialization
	struct parser *parser = malloc(sizeof(struct parser));
	parser->pgn = malloc(sizeof(struct pgn));
	lexer_fopen(&parser->lexer, filename);
	list_init(&parser->pgn->tags);
	list_init(&parser->pgn->moves);

	// main parser loop
	parser_next(parser);
	while (parser->lexer.curr_token.type != TK_EOF) {
		switch (parser->lexer.curr_token.type) {
		case TK_LBRACKET: parser_tag(parser);   break;
		case TK_INTEGER:  parser_move(parser);  break;
		case TK_SYMBOL:	  parser_move(parser);  break;
		default: parser_next(parser);
		}
	}

	// cleanup
	struct pgn *pgn = parser->pgn;
	lexer_fclose(&parser->lexer);
	free(parser);

	return pgn;
}

void pgn_free(struct pgn *pgn)
{
	free(pgn);
}
