/*
 * PS2Claw - Terminal Display Module
 * Virtual terminal with scroll buffer
 * Green/cyan text on dark background
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <tamtypes.h>
#include <debug.h>
#include "terminal.h"

/* Terminal line structure */
typedef struct {
    char text[TERM_COLS + 1];
    int color;        /* -1 for no color */
} TermLine;

/* Terminal state */
static TermLine lines[TERM_ROWS];
static int scroll_offset = 0;      /* How many lines scrolled from bottom */
static int total_lines = 0;        /* Total lines in buffer */
static char prompt_text[32] = "> ";
static int needs_redraw = 1;

/* Ring buffer for unlimited history */
#define MAX_LINES 256
static TermLine line_buffer[MAX_LINES];
static int line_buffer_size = 0;
static int line_head = 0;
static int line_count = 0;

/* Initialize terminal */
void term_init(void) {
    /* Clear buffer */
    for (int i = 0; i < MAX_LINES; i++) {
        line_buffer[i].text[0] = '\0';
        line_buffer[i].color = -1;
    }
    
    /* Clear display lines */
    for (int i = 0; i < TERM_ROWS; i++) {
        lines[i].text[0] = '\0';
        lines[i].color = -1;
    }
    
    scroll_offset = 0;
    total_lines = 0;
    line_buffer_size = 0;
    line_head = 0;
    line_count = 0;
    
    /* Set default prompt */
    strcpy(prompt_text, "> ");
    
    /* Add welcome message */
    term_println(TERM_COLOR_INFO, "PS2Claw Terminal v1.3 initialized");
    term_println(TERM_COLOR_INFO, "Type /help for commands");
    term_println(TERM_COLOR_DEFAULT, "");
    
    needs_redraw = 1;
}

/* Clear terminal */
void term_clear(void) {
    if (!line_buffer) return;
    
    for (int i = 0; i < MAX_LINES; i++) {
        line_buffer[i].text[0] = '\0';
        line_buffer[i].color = -1;
    }
    
    line_head = 0;
    line_count = 0;
    scroll_offset = 0;
    total_lines = 0;
    
    needs_redraw = 1;
}

/* Add a line to terminal */
static void add_line(const char *text, int color) {
    if (!text) return;
    
    /* Truncate to max width */
    int len = strlen(text);
    if (len > TERM_COLS) len = TERM_COLS;
    
    /* Copy to buffer */
    strncpy(line_buffer[line_head].text, text, TERM_COLS);
    line_buffer[line_head].text[TERM_COLS] = '\0';
    line_buffer[line_head].color = color;
    
    /* Advance head */
    line_head = (line_head + 1) % MAX_LINES;
    
    if (line_count < MAX_LINES) {
        line_count++;
    }
    
    total_lines++;
    needs_redraw = 1;
}

/* Print to terminal with color */
void term_printf(int color, const char *fmt, ...) {
    if (!line_buffer || !fmt) return;
    
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    /* Split into lines at newlines */
    char *line_start = buffer;
    char *newline;
    
    while ((newline = strchr(line_start, '\n')) != NULL) {
        *newline = '\0';
        add_line(line_start, color);
        line_start = newline + 1;
    }
    
    /* Add final part */
    if (*line_start) {
        add_line(line_start, color);
    }
}

/* Print raw to terminal */
void term_print(const char *str) {
    add_line(str, TERM_COLOR_DEFAULT);
}

/* Print line to terminal */
void term_println(int color, const char *str) {
    if (str) {
        add_line(str, color);
    } else {
        add_line("", color);
    }
}

/* Scroll terminal up */
void term_scroll(void) {
    if (line_count <= TERM_ROWS) return;
    
    scroll_offset++;
    if (scroll_offset > line_count - TERM_ROWS) {
        scroll_offset = line_count - TERM_ROWS;
    }
    needs_redraw = 1;
}

/* Scroll multiple lines */
void term_scroll_lines(int lines) {
    for (int i = 0; i < lines; i++) {
        term_scroll();
    }
}

/* Get prompt */
const char* term_get_prompt(void) {
    return prompt_text;
}

/* Set prompt */
void term_set_prompt(const char *prompt) {
    if (prompt) {
        strncpy(prompt_text, prompt, sizeof(prompt_text) - 1);
        prompt_text[sizeof(prompt_text) - 1] = '\0';
    }
}

/* Check if needs redraw */
int term_needs_redraw(void) {
    return needs_redraw;
}

/* Mark for redraw */
void term_invalidate(void) {
    needs_redraw = 1;
}

/* Get line from buffer by index from end */
static const char* get_line_from_end(int idx_from_end) {
    if (!line_buffer || idx_from_end >= line_count) {
        return "";
    }
    
    int idx = (line_head - 1 - idx_from_end + MAX_LINES) % MAX_LINES;
    return line_buffer[idx].text;
}

/* Get line color from buffer */
static int get_line_color(int idx_from_end) {
    if (!line_buffer || idx_from_end >= line_count) {
        return TERM_COLOR_DEFAULT;
    }
    
    int idx = (line_head - 1 - idx_from_end + MAX_LINES) % MAX_LINES;
    return line_buffer[idx].color;
}

/* Render terminal to screen */
void term_render(void) {
    if (!needs_redraw) return;
    
    /* Clear screen */
    scr_printf("\x1b[2J");  /* Clear entire screen */
    scr_printf("\x1b[H");   /* Home cursor */
    
    /* Draw header */
    term_draw_header();
    
    /* Calculate which lines to show */
    int start_idx = line_count - TERM_ROWS - scroll_offset;
    if (start_idx < 0) start_idx = 0;
    
    /* Show terminal lines */
    for (int row = 0; row < TERM_ROWS - 1; row++) {
        int line_idx = start_idx + row;
        
        if (line_idx < line_count) {
            const char *text = get_line_from_end(line_count - 1 - line_idx);
            int color = get_line_color(line_count - 1 - line_idx);
            
            /* Apply color */
            if (color == TERM_COLOR_ERROR) {
                scr_printf("\x1b[31m");  /* Red */
            } else if (color == TERM_COLOR_INFO) {
                scr_printf("\x1b[36m");  /* Cyan */
            } else if (color == TERM_COLOR_PROMPT) {
                scr_printf("\x1b[32m");  /* Green */
            } else if (color == TERM_COLOR_HEADER) {
                scr_printf("\x1b[33m");  /* Yellow */
            } else {
                scr_printf("\x1b[32m");  /* Default green */
            }
            
            scr_printf("%s\x1b[0m\n", text);
        } else {
            scr_printf("\n");
        }
    }
    
    needs_redraw = 0;
}

/* Draw border box */
void term_draw_box(const char *title) {
    scr_printf("+");
    for (int i = 0; i < TERM_COLS; i++) scr_printf("-");
    scr_printf("+\n");
    
    if (title) {
        scr_printf("| \x1b[33m%s\x1b[0m", title);
        int len = strlen(title);
        for (int i = len + 1; i <= TERM_COLS; i++) {
            scr_printf(" ");
        }
        scr_printf("|\n");
    }
}

/* Draw status bar */
void term_draw_status(const char *provider, const char *model, int demo_mode) {
    scr_printf("\x1b[30m\x1b[42m");  /* Dark bg, green text */
    scr_printf(" %s | %s | ", provider, model);
    
    if (demo_mode) {
        scr_printf("\x1b[33mDEMO\x1b[0m");
    } else {
        scr_printf("\x1b[32mONLINE\x1b[0m");
    }
    
    /* Pad to full width */
    int len = strlen(provider) + strlen(model) + (demo_mode ? 4 : 6) + 7;
    for (int i = len; i < TERM_COLS; i++) {
        scr_printf(" ");
    }
    
    scr_printf("\x1b[0m\n");
}

/* Draw header */
void term_draw_header(void) {
    scr_printf("\x1b[30m\x1b[46m");  /* Dark bg, cyan header */
    scr_printf("> PS2Claw - Terminal Mode");
    
    int len = 27;
    for (int i = len; i < TERM_COLS; i++) {
        scr_printf(" ");
    }
    
    scr_printf("\x1b[0m\n");
}