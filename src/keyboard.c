/*
 * PS2Claw - Virtual Keyboard for Terminal Input
 * D-pad navigation, X to select, O for backspace
 * L1/R1 to switch pages (letters/numbers/symbols)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tamtypes.h>
#include <debug.h>
#include "pad.h"
#include "keyboard.h"

/* Max input length */
#define MAX_INPUT 256

/* Keyboard layout - 4 rows, 10 columns */
#define KB_ROWS 4
#define KB_COLS 10

/* Keyboard pages */
static const char kb_letters[KB_ROWS][KB_COLS] = {
    {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '\0'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '-', '_'},
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}
};

static const char kb_numbers[KB_ROWS][KB_COLS] = {
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
    {'.', ',', '!', '?', '$', '%', '&', '*', '@', '#'},
    {'-', '_', '=', '+', '/', '\\', '|', ':', ';', '\0'},
    {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}
};

static const char kb_symbols[KB_ROWS][KB_COLS] = {
    {'<', '>', '{', '}', '[', ']', '(', ')', '\'', '"'},
    {'`', '~', '^', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
    {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
    {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'}
};

/* State */
static char input_buffer[MAX_INPUT];
static int input_len = 0;
static int cursor_row = 0;
static int cursor_col = 0;
static KeyboardPage current_page = KB_PAGE_LETTERS;
static int visible = 0;
static int enter_pressed_flag = 0;
static int escape_pressed_flag = 0;
static int debounce = 0;
static int cursor_blink = 0;
static int blink_counter = 0;

/* Command history */
#define MAX_HISTORY 20
#define MAX_HISTORY_LEN 128
static char history[MAX_HISTORY][MAX_HISTORY_LEN];
static int history_count = 0;
static int history_index = -1;

/* Autocomplete suggestions */
#define MAX_SUGGESTIONS 4
static const char* suggestions[MAX_SUGGESTIONS];
static const char* commands[] = {
    "/help", "/clear", "/status", "/model", "/provider", "/quit", 
    "/demo", "/history", NULL
};

/* Get current keyboard layout */
static const char (*get_kb_layout(void))[KB_COLS] {
    switch (current_page) {
        case KB_PAGE_NUMBERS: return kb_numbers;
        case KB_PAGE_SYMBOLS: return kb_symbols;
        default: return kb_letters;
    }
}

/* Initialize keyboard */
void kb_init(void) {
    input_buffer[0] = '\0';
    input_len = 0;
    cursor_row = 0;
    cursor_col = 0;
    current_page = KB_PAGE_LETTERS;
    visible = 0;
    enter_pressed_flag = 0;
    escape_pressed_flag = 0;
    debounce = 0;
    history_count = 0;
    history_index = -1;
}

/* Update keyboard state */
void kb_update(void) {
    if (!visible) return;
    
    /* Cursor blink */
    blink_counter++;
    if (blink_counter > 30) {
        blink_counter = 0;
        cursor_blink = !cursor_blink;
    }
    
    if (debounce > 0) {
        debounce--;
    }
}

/* Check if keyboard is visible */
int kb_is_visible(void) {
    return visible;
}

/* Show keyboard */
void kb_show(void) {
    visible = 1;
    cursor_col = 0;
    cursor_row = 0;
}

/* Hide keyboard */
void kb_hide(void) {
    visible = 0;
}

/* Get current input */
const char* kb_get_text(void) {
    return input_buffer;
}

/* Clear input */
void kb_clear(void) {
    input_buffer[0] = '\0';
    input_len = 0;
    history_index = -1;
}

/* Check enter pressed */
int kb_enter_pressed(void) {
    if (enter_pressed_flag) {
        enter_pressed_flag = 0;
        return 1;
    }
    return 0;
}

/* Check escape pressed */
int kb_escape_pressed(void) {
    if (escape_pressed_flag) {
        escape_pressed_flag = 0;
        return 1;
    }
    return 0;
}

/* Handle keyboard input */
int kb_handle_input(void) {
    if (!visible) return 0;
    if (debounce > 0) return 0;
    
    const char (*kb)[KB_COLS] = get_kb_layout();
    
    /* D-pad Up */
    if (pad_button_just_pressed(PAD_UP)) {
        cursor_row--;
        if (cursor_row < 0) cursor_row = KB_ROWS - 1;
        /* Skip empty cells */
        while (cursor_row > 0 && kb[cursor_row][cursor_col] == '\0') {
            cursor_row--;
        }
        if (kb[cursor_row][cursor_col] == '\0') {
            cursor_row = 0;
        }
        debounce = 8;
        return 1;
    }
    
    /* D-pad Down */
    if (pad_button_just_pressed(PAD_DOWN)) {
        cursor_row++;
        if (cursor_row >= KB_ROWS) cursor_row = 0;
        while (cursor_row < KB_ROWS - 1 && kb[cursor_row][cursor_col] == '\0') {
            cursor_row++;
        }
        if (kb[cursor_row][cursor_col] == '\0') {
            cursor_row = KB_ROWS - 1;
            while (cursor_row > 0 && kb[cursor_row][cursor_col] == '\0') {
                cursor_row--;
            }
        }
        debounce = 8;
        return 1;
    }
    
    /* D-pad Left */
    if (pad_button_just_pressed(PAD_LEFT)) {
        cursor_col--;
        if (cursor_col < 0) cursor_col = KB_COLS - 1;
        /* Skip empty cells */
        while (cursor_col > 0 && kb[cursor_row][cursor_col] == '\0') {
            cursor_col--;
        }
        if (kb[cursor_row][cursor_col] == '\0') {
            cursor_col = 0;
        }
        debounce = 8;
        return 1;
    }
    
    /* D-pad Right */
    if (pad_button_just_pressed(PAD_RIGHT)) {
        cursor_col++;
        if (cursor_col >= KB_COLS) cursor_col = 0;
        while (cursor_col < KB_COLS - 1 && kb[cursor_row][cursor_col] == '\0') {
            cursor_col++;
        }
        if (kb[cursor_row][cursor_col] == '\0') {
            cursor_col = KB_COLS - 1;
            while (cursor_col > 0 && kb[cursor_row][cursor_col] == '\0') {
                cursor_col--;
            }
        }
        debounce = 8;
        return 1;
    }
    
    /* X button - select key */
    if (pad_button_just_pressed(PAD_CROSS)) {
        char key = kb[cursor_row][cursor_col];
        if (key != '\0' && input_len < MAX_INPUT - 1) {
            /* Handle shift (if needed - for now just add char) */
            input_buffer[input_len++] = key;
            input_buffer[input_len] = '\0';
        }
        debounce = 10;
        return 1;
    }
    
    /* O button - backspace */
    if (pad_button_just_pressed(PAD_CIRCLE)) {
        if (input_len > 0) {
            input_len--;
            input_buffer[input_len] = '\0';
        }
        debounce = 10;
        return 1;
    }
    
    /* L1 - previous page */
    if (pad_button_just_pressed(PAD_L1)) {
        if (current_page > 0) {
            current_page--;
        } else {
            current_page = KB_PAGE_MAX - 1;
        }
        cursor_row = 0;
        cursor_col = 0;
        debounce = 15;
        return 1;
    }
    
    /* R1 - next page */
    if (pad_button_just_pressed(PAD_R1)) {
        current_page = (current_page + 1) % KB_PAGE_MAX;
        cursor_row = 0;
        cursor_col = 0;
        debounce = 15;
        return 1;
    }
    
    /* Triangle - toggle case (simple shift) */
    if (pad_button_just_pressed(PAD_TRIANGLE)) {
        /* Toggle case of last character if alphabetic */
        if (input_len > 0) {
            char *p = &input_buffer[input_len - 1];
            if (*p >= 'a' && *p <= 'z') {
                *p = *p - 'a' + 'A';
            } else if (*p >= 'A' && *p <= 'Z') {
                *p = *p - 'A' + 'a';
            }
        }
        debounce = 10;
        return 1;
    }
    
    /* Square - space */
    if (pad_button_just_pressed(PAD_SQUARE)) {
        if (input_len < MAX_INPUT - 1) {
            input_buffer[input_len++] = ' ';
            input_buffer[input_len] = '\0';
        }
        debounce = 10;
        return 1;
    }
    
    /* Select - enter (send message) */
    if (pad_button_just_pressed(PAD_SELECT)) {
        if (input_len > 0) {
            enter_pressed_flag = 1;
        }
        debounce = 10;
        return 1;
    }
    
    /* Start - escape (close keyboard) */
    if (pad_button_just_pressed(PAD_START)) {
        escape_pressed_flag = 1;
        debounce = 10;
        return 1;
    }
    
    /* Up on left analog - history up */
    if (pad_get_ljoy_v() < 60 && history_count > 0) {
        if (history_index < history_count - 1) {
            history_index++;
            strncpy(input_buffer, history[history_count - 1 - history_index], MAX_INPUT - 1);
            input_len = strlen(input_buffer);
        }
        debounce = 10;
        return 1;
    }
    
    /* Down on left analog - history down */
    if (pad_get_ljoy_v() > 196 && history_index > 0) {
        history_index--;
        strncpy(input_buffer, history[history_count - 1 - history_index], MAX_INPUT - 1);
        input_len = strlen(input_buffer);
        debounce = 10;
        return 1;
    }
    
    return 0;
}

/* Add to command history */
void kb_add_history(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    
    /* Don't add duplicates of last entry */
    if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0) {
        return;
    }
    
    /* Shift history if full */
    if (history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        history_count = MAX_HISTORY - 1;
    }
    
    strncpy(history[history_count], cmd, MAX_HISTORY_LEN - 1);
    history[history_count][MAX_HISTORY_LEN - 1] = '\0';
    history_count++;
    history_index = -1;
}

/* Get autocomplete suggestions */
const char** kb_get_suggestions(int *count) {
    *count = 0;
    
    if (input_len == 0) {
        return suggestions;
    }
    
    /* Check if input starts with / */
    if (input_buffer[0] != '/') {
        return suggestions;
    }
    
    int sug = 0;
    for (int i = 0; commands[i] != NULL && sug < MAX_SUGGESTIONS; i++) {
        if (strncmp(input_buffer, commands[i], input_len) == 0) {
            suggestions[sug++] = commands[i];
        }
    }
    
    *count = sug;
    return suggestions;
}

/* Render keyboard */
void kb_render(void) {
    if (!visible) return;
    
    const char (*kb)[KB_COLS] = get_kb_layout();
    
    scr_printf("\x1b[30m\x1b[42m");  /* Dark bg, light green for keyboard */
    
    /* Page indicator */
    const char *page_name = "LETTERS";
    if (current_page == KB_PAGE_NUMBERS) page_name = "NUMBERS";
    else if (current_page == KB_PAGE_SYMBOLS) page_name = "SYMBOLS";
    
    scr_printf("\x1b[0m");  /* Reset */
    scr_printf(" \x1b[33m[Page: %s]\x1b[0m L1=< R1=>\n", page_name);
    
    /* Draw keyboard grid */
    for (int row = 0; row < KB_ROWS; row++) {
        scr_printf(" ");
        for (int col = 0; col < KB_COLS; col++) {
            char key = kb[row][col];
            
            if (key == '\0') {
                scr_printf("  ");
                continue;
            }
            
            if (row == cursor_row && col == cursor_col) {
                /* Selected key */
                if (cursor_blink) {
                    scr_printf("\x1b[37m\x1b[44m[%c]\x1b[0m", key);
                } else {
                    scr_printf("\x1b[30m\x1b[47m[%c]\x1b[0m", key);
                }
            } else {
                scr_printf(" %c ", key);
            }
        }
        scr_printf("\n");
    }
    
    /* Controls hint */
    scr_printf(" \x1b[36mX=Type\x1b[0m \x1b[36mO=Backspace\x1b[0m \x1b[36mSelect=Send\x1b[0m \x1b[36mSquare=Space\x1b[0m\n");
    scr_printf(" \x1b[36mStart=Exit\x1b[0m \x1b[36mTriangle=Shift\x1b[0m\n");
}