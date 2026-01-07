#include "pgn.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The '/' is technically not specified to be part of a SYMBOL in the standard,
// but since 1/2-1/2 is a possibility in the movetext, I think it should count
// as being part of a symbol
enum token_type {
	TK_LBRACKET,	// [
	TK_RBRACKET, 	// ]
	TK_LPAREN,	// (
	TK_RPAREN,	// )
	TK_LANGLE,	// <
	TK_RANGLE,	// >
	TK_PERIOD,	// .
	TK_COMMENT,	// ; along with any text until \n or anything inside { }
	TK_TERMINATION,	// *, 1-0, 0-1, 1/2-1/2 
	TK_STRING,	// quote delimited characters
	TK_SYMBOL,	// letter or digits followed by any of [A-Za-z0-9_+#=:-/]
	TK_INTEGER,	// sequence of decimal digits
	TK_NAG,		// $ followed by digits
	TK_UNKNOWN,     // unparsable tokens
	TK_EOF,         // end of file
	TK_MAX
};

static const char* token_str[TK_MAX] = {
	[TK_LBRACKET]     = "LBRACKET",
	[TK_RBRACKET]     = "RBRACKET",
	[TK_LPAREN]       = "LPAREN",
	[TK_RPAREN]       = "RPAREN",
	[TK_LANGLE]       = "LANGE",
	[TK_RANGLE]       = "RANGLE",
	[TK_PERIOD]       = "PERIOD",
	[TK_COMMENT]      = "COMMENT",
	[TK_TERMINATION]  = "TERMINATION",
	[TK_STRING]       = "STRING",
	[TK_SYMBOL]       = "SYMBOL",
	[TK_INTEGER]      = "INTEGER",
	[TK_NAG]          = "NAG",
	[TK_UNKNOWN]      = "UNKNOWN",
	[TK_EOF]          = "EOF"
};

static const char *syntax_err =
	"Error(Syntax) |%d, col %d|: expected token '%s' "
	"but found token '%s' with value '%.*s'. Previous token "
	"was '%s' with value '%.*s'.\n";

static const char *parser_err =
	"Error(Parser) |%d, col %d|: error occurred trying to parse '%s'\n";

/// Note the value stored in the token does not include the null terminator
struct token {
	enum token_type type; // type of the token
	char value[256];      // symbols and strings have max length of 255
	int len;              // length of value
};

/// Internal structure for sharing data between parsing functions
/// Since the implementation of the reader is an interleaved  
/// lexer + parser, we need this sharing of data 
struct parser {
	struct pgn *pgn;         // current pgn game being parsed
	enum pgn_result result;  // result of pgn read operation

	// lexer
	FILE *file;              // pgn file being read from
	int y, x;                // location of lexer cursor (syntax errors)

	// parser
	bool unhandled_error;    // whether there is an unhandled error
	struct token token;      // current token being parsed
	struct token prev_token; // previous token
};

//
// Lexer
//

static inline bool is_symbol(char c)
{
	return isalnum(c) || c == '_' || c == '+' || c == '#' || c == '/'
			  || c == '=' || c == ':' || c == '-';
}

static inline char advance(struct parser *parser)
{
	++parser->x;
	return getc(parser->file);
}

static inline char peek(struct parser *parser)
{
	char c = getc(parser->file);
	ungetc(c, parser->file);
	return c;
}

static inline void add_to_token(struct parser *parser, char c)
{
	parser->token.value[parser->token.len] = c;
	++parser->token.len;
}

static void terminal(struct parser *parser, char c, enum token_type type)
{
	parser->token.value[0] = c;
	parser->token.type = type;
	parser->token.len = 1;
}

// Collapses periods or treat the period as ellipses depending on the number
// of periods, see `void move_indicator()` for more information on this
static void period(struct parser *parser)
{
	while (peek(parser) == '.')
		add_to_token(parser, advance(parser));

	// If there are three periods (ellipse), then we treat it as a symbol
	// because it indicates a white move
	parser->token.type = (parser->token.len == 3) ? TK_SYMBOL : TK_PERIOD;
}

/// String tokens, these start with a " and end with a " 
static void string(struct parser *parser)
{
	parser->token.type = TK_STRING;
	// TODO: handle escaped strings, \"
	char c;
	while ((c = advance(parser)) != '"')
		add_to_token(parser, c);
}

static void symbol(struct parser *parser)
{
	// Assume all digits
	bool integer = true;
	// We added a char earlier, it could be a letter
	// which would would mean our assumption above is false
	if (isdigit(parser->token.value[0]) == 0)
		integer = false;

	while (is_symbol(peek(parser))) {
		char c = advance(parser);
		add_to_token(parser, c);
		// Found a non-digit, so this is a symbol
		// NOTE: isdigit returns zero if it is not a digit
		integer &= (isdigit(c) != 0);
	}

	parser->token.type = integer ? TK_INTEGER : TK_SYMBOL;
	// But the symbol might be a termination marker
	if (parser->token.type == TK_SYMBOL &&
	    (strncmp(parser->token.value, "1/2-1/2", 7) == 0 ||
	     strncmp(parser->token.value, "1-0", 3) == 0 ||
	     strncmp(parser->token.value, "0-1", 3) == 0)) {
		parser->token.type = TK_TERMINATION;
	}
}

/// Nag tokens, these are digits following a $
static void nag(struct parser *parser)
{
	parser->token.type = TK_NAG;
	while (isdigit(peek(parser)))
		add_to_token(parser, advance(parser));
}

/// Comment tokens are not in the standard but I included it to make parsing
/// easy. Line comment continues until the end of a line.
static void line_comment(struct parser *parser)
{
	parser->token.type = TK_COMMENT;
	char c;
	while ((c = advance(parser)) != '\n')
		add_to_token(parser, c);
}

/// A brace comment is whatever is inside { }. These cannot be nested. 
/// TODO: handle error on nesting?
static void brace_comment(struct parser *parser)
{
	parser->token.type = TK_COMMENT;
	char c;
	while ((c = advance(parser)) != '}')
		add_to_token(parser, c);
}

static void nag_shorthand(struct parser *parser)
{
	parser->token.type = TK_NAG;
	if (peek(parser) == '!' || peek(parser) == '?')
		add_to_token(parser, advance(parser));
}

// This just allows me to use fake ranges for the switch statement
static char transform_char(char c) {
	if (isalnum(c))
		return 'a';
	else
		return c;
}

static void next_token(struct parser *parser)
{
	// TODO: add verbose variable and use this for debugging
	/* printf("parsed %s with value %.*s\n", token_str[parser->token.type], parser->token.len, parser->token.value); */

	parser->prev_token = parser->token;
	// Clear out the current token value, no need to clear out .value itself
	// since we will use memcpy to pull the value out of the token, that is,
	// the token is an implementation detail which is not exposed publicly
	parser->token.len = 0;

	while (true) {
		char c = advance(parser);
		switch (transform_char(c)) {
		// Terminal tokens
		case '[': return terminal(parser, c, TK_LBRACKET);
		case ']': return terminal(parser, c, TK_RBRACKET);
		case '(': return terminal(parser, c, TK_LPAREN);
		case ')': return terminal(parser, c, TK_RPAREN);
		case '<': return terminal(parser, c, TK_LANGLE);
		case '>': return terminal(parser, c, TK_RANGLE);
		case '*': return terminal(parser, c, TK_TERMINATION);
		case EOF: return terminal(parser, c, TK_EOF);
		// NAG shorthands
		case '!':
		case '?':
			add_to_token(parser, c);
			return nag_shorthand(parser);
		// Complex tokens
		case '"': return string(parser);
		case '$': return nag(parser);
		case ';': return line_comment(parser);
		case '{': return brace_comment(parser);
		case '.':
			add_to_token(parser, c);
			return period(parser);
		case 'a':
			add_to_token(parser, c);
			return symbol(parser);
		// Ignore whitespace
		case '\t':
		case '\r':
		case ' ': break;
		case '\n': ++parser->y; parser->x = 0; break;
		default: return terminal(parser, c, TK_UNKNOWN);
		}
	}
}

//
// Parser
//

static inline bool accept(struct parser *parser, enum token_type type)
{
	return parser->token.type == type;
}

static bool expect(struct parser *parser, enum token_type type)
{
	if (accept(parser, type)) {
		next_token(parser);
		return true;
	}

	parser->unhandled_error = true;
	fprintf(stderr, syntax_err,
		parser->y,
		parser->x,
	 	token_str[type],
	 	token_str[parser->token.type],
		parser->token.len,
	 	parser->token.value,
	 	token_str[parser->prev_token.type],
		parser->prev_token.len,
	 	parser->prev_token.value);
	return false;
}

/// Copies the value of token to a buffer, the buffer must be freed
static inline void copy_token_value(char **buffer, struct token *token)
{
	char *tmp = malloc(sizeof(char) * (token->len + 1));
	if (tmp == NULL)
		abort();
	*buffer = tmp;
	memcpy(*buffer, token->value, token->len);
	tmp[token->len] = '\0';
}

/// Tag is made of the following tokens: "BRACKET SYMBOL STRING BRACKET"
static void tag(struct parser *parser)
{
	struct pgn_tag tag = {0};

	expect(parser, TK_LBRACKET);
	if (expect(parser, TK_SYMBOL)) { 
		copy_token_value(&tag.name, &parser->prev_token);
	}
	if (expect(parser, TK_STRING)) {
		copy_token_value(&tag.desc, &parser->prev_token);
	}
	expect(parser, TK_RBRACKET);

	// TODO: decide on an error handling strategy
	if (parser->unhandled_error) {
		free(tag.name);
		free(tag.desc);
		fprintf(stderr, parser_err, parser->y, parser->x, "tag");
		parser->unhandled_error = false;
		parser->result = PGN_TAG_PARSE_ERROR;
	} else {
		struct pgn_game *game = &vec_last(parser->pgn->games);
		vec_push(game->tags, tag);
	}
}

/// A move indicator shows the order the move is in,
///
///  1. d4
///  1  c4
///     c4
///  1........... Nf3
///
/// Are all valid. The move indicator is optional for imports. Also unlimited
/// periods and no periods is permitted by the standard. Consider,
///
///  1 ... d6
///
/// the ... actually denotes a move by white so that has to be handled
/// specifically Therefore the ... in "1............ d6" does not count but
/// "1... d6" does but the following "1......... ... d6" is fine.
/// TODO: handle ellipses
static void move_indicator(struct parser *parser)
{
	expect(parser, TK_INTEGER);
	// TODO: remove? I don't think this error can ever happen since we check
	// for TK_INTEGER beforehand
	if (parser->unhandled_error) {
		fprintf(stderr, parser_err, parser->y, parser->x, "move indicator");
		parser->unhandled_error = false;
		parser->result = PGN_MOVE_PARSE_ERROR;
	}
	// Since we collapse all the periods into a single token, we do not need
	// a while loop for this (single token instead of multiple)
	if (accept(parser, TK_PERIOD))
		expect(parser, TK_PERIOD);
}

/// Insane recursive nonsense incoming 
static void movetext(struct parser *parser, struct pgn_variation *variation);

/// RAV (recursive annotation variation) specifies a variation, for example
///
///   1. d4 d5 (1 ... d6 (1 ... e6) (1 ... f5) 2. e4 e5 3. d5 f5) (1 ... Nf5)
///
/// is allowed, which is actually insane. And NAGs and comments can still
/// show up in this by the way.
static void rav(struct parser *parser, struct pgn_variation *variation)
{
	expect(parser, TK_LPAREN);
	movetext(parser, variation);
	expect(parser, TK_RPAREN);
}

/// A ply is of a symbol, optionally along with a comment, NAG and RAV
/// (Recursive Annotation Variations), all of which can appear at once
/// and in any order.
static void ply(struct parser *parser, struct pgn_variation *variation)
{
	struct pgn_ply ply = {0};

	if (expect(parser, TK_SYMBOL))
		memcpy(&ply.text, &parser->prev_token.value, parser->prev_token.len);

	// TODO: as of right now, this actually overrides the previous
	// nag or comment, I think this is probably correct, but not sure
	//
	// I could not find anything on the standard forbidding multiple
	// NAGs or comments for a move, so assume it is legal
	// There is not really a reason to have multiple NAGS or comments 
	// but there is also not a reason why you couldn't have them either.
	while (accept(parser, TK_NAG) || accept(parser, TK_COMMENT) ||
	       accept(parser, TK_LPAREN)) {
		if (accept(parser, TK_NAG)) {
			expect(parser, TK_NAG);
			char *str = parser->prev_token.value;
			char *end;
			ply.nag = strtol(str, &end, 10);
			// Probably not a number, might be the shorthands
			if (*end) {
				if (strncmp(str, "!", 1) == 0)
					ply.nag = 1;
				else if (strncmp(str, "?", 1) == 0)
					ply.nag = 2;
				else if (strncmp(str, "!!", 2) == 0)
					ply.nag = 3;
				else if (strncmp(str, "??", 2) == 0)
					ply.nag = 4;
				else if (strncmp(str, "!?", 2) == 0)
					ply.nag = 5;
				else if (strncmp(str, "?!", 2) == 0)
					ply.nag = 6;
				else
					ply.nag = -1;
				fprintf(stderr, parser_err, parser->y, parser->x, "unknown NAG");
			}
		}

		if (accept(parser, TK_COMMENT)) {
			expect(parser, TK_COMMENT);
			copy_token_value(&ply.comment, &parser->prev_token);
		}

		if (accept(parser, TK_LPAREN)) {
			struct pgn_variation variation = {0};
			vec_push(ply.variations, variation);
			rav(parser, &variation);
		}
	}

	if (parser->unhandled_error) {
		fprintf(stderr, parser_err, parser->y, parser->x, "ply");
		parser->unhandled_error = false;
		parser->result = PGN_MOVE_PARSE_ERROR;
		free(ply.comment);
	} else {
		if (variation == NULL) {
			struct pgn_game *game = &vec_last(parser->pgn->games);
			vec_push(game->plies, ply);
		} else {
			vec_push(variation->plies, ply);
		}
	}
}

/// A movetext consists of a series of plies, the plies themselves may
/// optionally be prepended with a move order. The move from black
/// can be considered optional, consider the following
///
///  |snip| 21. Ng3 Ka1 22. a4 *
///
/// Here the game ended before black could make a move. I am not sure if
///
///  |snip| 1. f4 f5 2. Nf3 Nc6 3. *
///
/// is valid, but it is probably safe to assume this is invalid, therefore
/// the move from white is required 
/// 
/// The standard seems to indicate superfluous move indicators are allowed as
/// long as they are correct. For now, we are going to be lax and not check for
/// correctness of the indicators, though that is something to consider
static void movetext(struct parser *parser, struct pgn_variation *variation)
{
	while (accept(parser, TK_SYMBOL) || accept(parser, TK_INTEGER)) {
		if (accept(parser, TK_INTEGER))
			move_indicator(parser);

		ply(parser, variation);

		if (accept(parser, TK_INTEGER))
			move_indicator(parser);

		if (accept(parser, TK_SYMBOL)) 
			ply(parser, variation);
	}
}

/// A pgn game is a series of tags followed by a movetext and finally a
/// termination marker. The termination markers are 
///  "1-0" (White wins),
///  "0-1" (Black wins),
///  "1/2-1/2" (drawn game),
///  "*" (game in progress, result unknown, or game abandoned).
/// For easier parsing, the lexer has a token TK_TERMINATION
/// to account for "*" along with the rest which are symbols
/// But will be considered a termination token.
static void pgn_game_block(struct parser *parser)
{
	struct pgn_game game = {0};
	vec_init(game.tags);
	vec_init(game.plies);
	vec_push(parser->pgn->games, game);

	while (accept(parser, TK_LBRACKET))
		tag(parser);
	movetext(parser, NULL);

	if (expect(parser, TK_TERMINATION)) {
		if (strncmp(parser->token.value, "1/2-1/2", 7) == 0)
			game.termination = DRAW;
		else if (strncmp(parser->token.value, "1-0", 3) == 0)
			game.termination = WHITE_WIN;
		else if (strncmp(parser->token.value, "0-1", 3) == 0)
			game.termination = BLACK_WIN;
		else
			game.termination = UNKNOWN;
	}
}

enum pgn_result pgn_read(struct pgn* pgn, char* filename)
{
	// Initialize pgn 
	vec_init(pgn->games);
	pgn->filename = filename;
	pgn->number_errors = 0;

	// Initialize parser
	struct parser parser = {
		.pgn = pgn,
		.result = PGN_OK,

		.file  = fopen(filename, "r"),
		.y = 1,
		.x = 1,

		.unhandled_error = false,
		.token = {0},
		.prev_token = {0},
	};

	if (parser.file == NULL)
		return PGN_FILE_ERROR;

	// Start Parsing
	next_token(&parser);
	while (!accept(&parser, TK_EOF))
		pgn_game_block(&parser);

	// cleanup
	fclose(parser.file);
	return parser.result;
}

void pgn_free(struct pgn *pgn)
{
	for (int i = 0; i < vec_len(pgn->games); ++i) {
		struct pgn_game *game = &pgn->games[i];
		for (int j = 0; j < vec_len(game->tags); ++j) {
			free(game->tags[j].name);
			free(game->tags[j].desc);
		}
		vec_free(game->tags);

		for (int k = 0; k < vec_len(game->plies); ++k)
			free(game->plies[k].comment);
		vec_free(game->plies);
	}
	vec_free(pgn->games);
}
