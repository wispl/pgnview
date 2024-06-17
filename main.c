#include "array.h"
#include "board.h"
#include "parser.h"
#include "pgn_movelist.h"

#include "termbox2.h"

#include <stdio.h>
#include <stdbool.h>

#define CELLW  5
#define CELLH  2

#define SQUARES 64
#define ROWS    (8 * CELLW)
#define COLS    (8 * CELLH)

#define LEFTX  (tb_width() / 2) - (ROWS / 2)
#define LEFTY  4

#define RIGHTX (LEFTX + ROWS / 2)
#define RIGHTY (LEFTY + COLS)

static const char piece_str[PIECE_ID_MAX] = {
	[W_KING]   = 'K',
	[W_QUEEN]  = 'Q',
	[W_ROOK]   = 'R',
	[W_BISHOP] = 'B',
	[W_KNIGHT] = 'N',
	[W_PAWN]   = 'P',
	[B_KING]   = 'k',
	[B_QUEEN]  = 'q',
	[B_ROOK]   = 'r',
	[B_BISHOP] = 'b',
	[B_KNIGHT] = 'n',
	[B_PAWN]   = 'p',
	[EMPTY]    = ' ',
};

void draw_square(int x, int y, char ch, uintattr_t fg, uintattr_t bg)
{
	tb_print(x, y,     bg, fg, "▄▄▄▄▄");
	tb_printf(x, y + 1, fg, bg, "  %c  ", ch);
	tb_print(x, y + 2, bg,  0, "▀▀▀▀▀");
}

void draw_board(struct board *board)
{
	int x = LEFTX;
	int y = LEFTY;
	int shift = 0;
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			enum piece_id piece = board->squares[square];
			char ch = piece_str[piece];
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

int main(int argc, char **argv)
{
	init_lineattacks_table();
	tb_init();
	tb_hide_cursor();

	struct board board;
	board_init(&board);

	struct pgn pgn;
	struct movelist ARRAY(moves);

	pgn_read(&pgn, "tests/test.pgn");
	pgn_movelist(&pgn.moves, &moves);

	bool running = true;
	struct tb_event event;
	int result;
	int curr = 0;

	draw_board(&board);
	tb_present();

	while (running) {
		result = tb_poll_event(&event);
		if (result != TB_OK) {
			if (result == TB_ERR_POLL && tb_last_errno() == EINTR) {
				continue;
			}
			break;
		}

		switch (event.type) {
		case TB_EVENT_KEY:
			if (event.ch == 'q') {
				running = false;
			}
			if (event.key == TB_KEY_ARROW_RIGHT) {
				if (curr < moves.len) {
					board_move(&board, &array_get(&moves, curr));
					++curr;
					draw_board(&board);
					tb_present();
				}
			}
			break;
		default: break;
		}
	}

	array_free(&moves);
	pgn_free(&pgn);

	tb_shutdown();
}
