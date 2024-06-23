#include "array.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "pgn.h"
#include "pgn_movelist.h"

#include "termbox2.h"

#include <stdbool.h>
#include <stdio.h>

#define malloc_array(count, size) (malloc(count * size))

#define CELLW  5
#define CELLH  2

#define SQUARES 64
#define ROWS    (8 * CELLH)
#define COLS    (8 * CELLW)

// Upper left corner of board
#define LEFTX  (tb_width() / 2) - (COLS / 2)
#define LEFTY  4
// Bottom right corner of board
#define RIGHTX (LEFTX + COLS)
#define RIGHTY (LEFTY + ROWS)

// Converts bitboard coordinates to UI coordinates for termbox
#define ui_x(x) ((x & 7) * CELLW + LEFTX)
// Board is flipped due to bitboard
#define ui_y(y) (((63 - y) / 8) * CELLH + LEFTY)

// Used to access the backbuffer for termbox
#define tb_coord(x, y) ((y) * tb_width() + (x))

static char* piece_str[PIECE_ID_MAX] = {
	[W_KING]   = "K",
	[W_QUEEN]  = "Q",
	[W_ROOK]   = "R",
	[W_BISHOP] = "B",
	[W_KNIGHT] = "N",
	[W_PAWN]   = "P",
	[B_KING]   = "k",
	[B_QUEEN]  = "q",
	[B_ROOK]   = "r",
	[B_BISHOP] = "b",
	[B_KNIGHT] = "n",
	[B_PAWN]   = "p",
	[EMPTY]    = " ",
};

struct state {
	struct pgn pgn;
	struct board board;

	struct move *moves;
	int moves_idx;

	// Stores captured pieces for unwinding
	// 30 is the number of capturable pieces on a board
	enum piece_id captures[30];
	int captures_idx;
};

static struct state state = {
	.moves_idx = -1,
	.captures_idx = 0
};

void draw_square(int x, int y, char *str, uintattr_t fg, uintattr_t bg)
{
	// sample top and bottom squares to blend them
	struct tb_cell up = tb_cell_buffer()[tb_coord(x, y - 1)];
	struct tb_cell down = tb_cell_buffer()[tb_coord(x, y + CELLH + 1)];

	tb_print( x, y,     bg,   up.bg, "▄▄▄▄▄");
	tb_printf(x, y + 1, fg,      bg, "  %s  ", str);
	tb_print( x, y + 2, bg, down.bg, "▀▀▀▀▀");
}

void highlight_square(int square)
{
	int x = ui_x(square);
	int y = ui_y(square);

	struct tb_cell cell = tb_cell_buffer()[tb_coord(x + 2, y + 1)];
	char ch[7];
	tb_utf8_unicode_to_char(ch, cell.ch);

	draw_square(x, y, ch, TB_BLACK, TB_YELLOW);
}

// TODO: investigate not drawing the whole board but only changes
void draw_board(struct board *board)
{
	int x = LEFTX;
	int y = LEFTY;
	int shift = 0;
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			enum piece_id id = board->squares[square];
			char *ch = piece_str[id];
			if ((file + shift) & 1) {
				draw_square(x, y, ch, TB_GREEN, TB_BLACK);
			} else {
				draw_square(x, y, ch, TB_BLACK, TB_GREEN);
			}
			x += CELLW;
		}
		shift += 1;

		x = LEFTX;
		y += CELLH;
	}
}

void draw_moves(struct pgn_movelist *moves, int current)
{
	int x = RIGHTX + CELLW;
	int y = LEFTY  + 2;

	int chunk = RIGHTY - y;
	int start = (current < chunk) ? 0 : (current / chunk);

	for (int i = start * chunk; y < RIGHTY; ++i) {
		// number indicator at white moves (even index)
		if (!(i & 1)) {
			// supports up to 3 digit amount of moves
			if (i < moves->len) {
				tb_printf(x, y, 0, 0, "%-4d", (i / 2) + 1);
			} else {
				tb_printf(x, y, 0, 0, "%4s", " ");
			}
			x += 4;
		}

		char *str = (i < moves->len) ? array_get(moves, i).text : " ";
		tb_printf(x, y, (current == i) * TB_YELLOW, 0, "%-8s", str);
		x += 8;

		// newline at black moves (odd index)
		if (i & 1) {
			y += CELLH;
			x  = RIGHTX + CELLW;
		}
	}
}

void do_move(bool undo)
{
	struct move curr;
	if (undo) {
		curr = state.moves[state.moves_idx];
		enum piece_id piece = EMPTY;
		if (curr.movetype == CAPTURE) {
			--state.captures_idx;
			piece = state.captures[state.captures_idx];
		}
		board_undo_move(&state.board, &curr, piece);
		--state.moves_idx;
	} else {
		++state.moves_idx;

		curr = state.moves[state.moves_idx];
		board_move(&state.board, &curr);

		if (curr.movetype == CAPTURE) {
			state.captures[state.captures_idx] = state.board.squares[curr.to];
			++state.captures_idx;
		}
	}

	draw_board(&state.board);
	highlight_square(curr.from);
	highlight_square(curr.to);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Please specify a file!\n");
		return 0;
	}
	init_lineattacks_table();
	board_init(&state.board);
	// TODO: handle read and file errors
	pgn_read(&state.pgn, argv[1]);

	// TODO: handle length mismatch
	state.moves   = malloc_array(state.pgn.moves.len, sizeof(struct move));
	int moves_len = pgn_to_moves(&state.pgn.moves, state.moves);

	tb_init();
	tb_hide_cursor();

	// initial draw
	draw_board(&state.board);
	draw_moves(&state.pgn.moves, state.moves_idx);
	tb_present();

	int result;
	struct tb_event event;
	bool running = true;
	while (running) {
		result = tb_poll_event(&event);
		if (result != TB_OK) {
			if (result == TB_ERR_POLL && tb_last_errno() == EINTR) {
				continue;
			}
			break;
		}

		switch (event.type) {
		case TB_EVENT_RESIZE:
			tb_clear();
			draw_board(&state.board);
			draw_moves(&state.pgn.moves, state.moves_idx);
			tb_present();
			break;
		case TB_EVENT_KEY:
			if (event.ch == 'q') {
				running = false;
			}
			if (event.key == TB_KEY_ARROW_RIGHT) {
				if (state.moves_idx < state.pgn.moves.len - 1) {
					do_move(false);
				}
			}
			if (event.key == TB_KEY_ARROW_LEFT) {
				if (state.moves_idx > -1) {
					do_move(true);
				}
			}
			if (event.key == TB_KEY_ARROW_UP) {
				while (state.moves_idx != -1) {
					do_move(true);
				}
			}
			if (event.key == TB_KEY_ARROW_DOWN) {
				while (state.moves_idx != state.pgn.moves.len - 1) {
					do_move(false);
				}
			}
			draw_moves(&state.pgn.moves, state.moves_idx);
			tb_present();
			break;
		default: break;
		}
	}

	free(state.moves);
	pgn_free(&state.pgn);

	tb_shutdown();

	return 1;
}
