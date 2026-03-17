/*
 * PS2Claw - Terminal Display Module
 * Virtual terminal with scroll buffer
 * Green/cyan text on dark background
 */

#ifndef __PS2CLAW_TERMINAL_H__
#define __PS2CLAW_TERMINAL_H__

#include <stdio.h>

/* Terminal dimensions */
#define TERM_COLS 60
#define TERM_ROWS 20

/* Terminal colors */
#define TERM_COLOR_DEFAULT   0
#define TERM_COLOR_PROMPT    1
#define TERM_COLOR_INPUT     2
#define TERM_COLOR_OUTPUT    3
#define TERM_COLOR_ERROR     4
#define TERM_COLOR_INFO      5
#define TERM_COLOR_HEADER    6

/* Initialize terminal */
void term_init(void);

/* Clear terminal */
void term_clear(void);

/* Print to terminal with color */
void term_printf(int color, const char *fmt, ...);

/* Print raw to terminal (no color processing) */
void term_print(const char *str);

/* Print line to terminal */
void term_println(int color, const char *str);

/* Scroll terminal up one line */
void term_scroll(void);

/* Scroll multiple lines */
void term_scroll_lines(int lines);

/* Render terminal to screen */
void term_render(void);

/* Get prompt line (for current input display) */
const char* term_get_prompt(void);

/* Set prompt text */
void term_set_prompt(const char *prompt);

/* Draw border box */
void term_draw_box(const char *title);

/* Draw status bar */
void term_draw_status(const char *provider, const char *model, int demo_mode);

/* Draw header */
void term_draw_header(void);

/* Check if terminal needs redraw */
int term_needs_redraw(void);

/* Mark terminal as needing redraw */
void term_invalidate(void);

#endif /* __PS2CLAW_TERMINAL_H__ */