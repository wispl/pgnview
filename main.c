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

// static struct board board;

void draw_square(int x, int y, uintattr_t fg, uintattr_t bg)
{
	tb_print(x, y,     bg, fg, "▄▄▄▄▄");
	tb_print(x, y + 1, fg, bg, "  N  ");
	tb_print(x, y + 2, bg,  0, "▀▀▀▀▀");
}

void draw_board()
{
	int x = LEFTX;
	int y = LEFTY;
	int shift = 0;
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			if ((file + shift) & 1) {
				draw_square(x, y, TB_GREEN, TB_BLACK);
			} else {
				draw_square(x, y, TB_BLACK, TB_GREEN);
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
	// board_init(&board);

	struct tb_event event;
	int y = 0;

	tb_init();
	tb_hide_cursor();

	draw_board();
	tb_present();

	tb_poll_event(&event);

	tb_shutdown();
}
