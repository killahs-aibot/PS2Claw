/*
 * PS2Claw - Simple Menu System
 * D-pad navigation with X button selection
 */

#include <stdio.h>
#include <string.h>
#include <libpad.h>
#include <tamtypes.h>
#include <debug.h>
#include "pad.h"
#include "menu.h"
#include "terminal_shared.h"

/* Menu items */
static const MenuItem main_menu_items[] = {
    { "Chat",       MODE_CHAT },
    { "Settings",   MODE_SETTINGS },
    { "About",      MODE_ABOUT }
};

#define MAIN_MENU_COUNT (sizeof(main_menu_items) / sizeof(MenuItem))

/* Current state */
static AppMode current_mode = MODE_MENU;
static int menu_cursor = 0;         /* Current selection */
static int in_about = 0;            /* Show about screen */
static int in_settings = 0;        /* Show settings screen */

/* Debounce for menu navigation */
static int menu_debounce = 0;
#define MENU_DEBOUNCE_FRAMES 10

/* Show about screen */
static void show_about(void)
{
    print_border();
    scr_printf("| PS2Claw v1.2 - DualShock Edition           |\n");
    scr_printf("| War Games / Nuclear Winter Theme           |\n");
    scr_printf("|                                           |\n");
    scr_printf("| AI: OpenAI, Google, Anthropic,             |\n");
    scr_printf("|     DeepSeek, xAI                         |\n");
    scr_printf("|                                           |\n");
    scr_printf("| Controls:                                  |\n");
    scr_printf("|   D-pad: Navigate menu                    |\n");
    scr_printf("|   X button: Select                        |\n");
    scr_printf("|   O button: Back                          |\n");
    print_border();
    scr_printf("\x1b[33m[ Press O to go back ]\x1b[0m\n");
}

/* Show settings screen */
static void show_settings(void)
{
    print_border();
    scr_printf("| PS2Claw Settings                          |\n");
    print_border();
    scr_printf("| Provider: %-33s|\n", providers[current_provider].name);
    scr_printf("| Model: %-34s|\n", providers[current_provider].model);
    scr_printf("| Demo Mode: %-29s|\n", demo_mode ? "ON" : "OFF");
    print_border();
    scr_printf("\x1b[33m[ Press O to go back ]\x1b[0m\n");
}

/* Render main menu */
static void render_main_menu(void)
{
    int i;

    print_border();
    scr_printf("| PS2Claw - Main Menu                      |\n");
    print_border();

    /* Menu items */
    for (i = 0; i < MAIN_MENU_COUNT; i++) {
        if (i == menu_cursor) {
            scr_printf("|  \x1b[32m> %-20s\x1b[0m                 |\n", main_menu_items[i].label);
        } else {
            scr_printf("|    %-22s                 |\n", main_menu_items[i].label);
        }
    }

    print_border();

    /* Provider footer */
    print_provider_footer();
    print_border();
}

/* Handle menu navigation */
static void handle_menu_input(void)
{
    if (menu_debounce > 0) {
        menu_debounce--;
        return;
    }

    /* D-pad Up */
    if (pad_button_just_pressed(PAD_UP)) {
        menu_cursor--;
        if (menu_cursor < 0) {
            menu_cursor = MAIN_MENU_COUNT - 1;
        }
        menu_debounce = MENU_DEBOUNCE_FRAMES;
        return;
    }

    /* D-pad Down */
    if (pad_button_just_pressed(PAD_DOWN)) {
        menu_cursor++;
        if (menu_cursor >= MAIN_MENU_COUNT) {
            menu_cursor = 0;
        }
        menu_debounce = MENU_DEBOUNCE_FRAMES;
        return;
    }

    /* X button - select */
    if (pad_button_just_pressed(PAD_CROSS)) {
        current_mode = main_menu_items[menu_cursor].target_mode;

        if (current_mode == MODE_ABOUT) {
            in_about = 1;
        } else if (current_mode == MODE_SETTINGS) {
            in_settings = 1;
        }

        menu_debounce = MENU_DEBOUNCE_FRAMES;
        return;
    }
}

/* Handle about/settings input */
static void handle_submenu_input(void)
{
    if (menu_debounce > 0) {
        menu_debounce--;
        return;
    }

    /* O button - go back */
    if (pad_button_just_pressed(PAD_CIRCLE)) {
        in_about = 0;
        in_settings = 0;
        current_mode = MODE_MENU;
        menu_debounce = MENU_DEBOUNCE_FRAMES;
        return;
    }
}

/* Initialize menu system */
void menu_init(void)
{
    current_mode = MODE_MENU;
    menu_cursor = 0;
    in_about = 0;
    in_settings = 0;
    menu_debounce = 0;
}

/* Handle menu input - call each frame */
void menu_update(void)
{
    /* Poll controller */
    pad_poll();

    /* Check for mode switches */
    if (pad_button_just_pressed(PAD_START)) {
        /* Start button - toggle between menu and chat */
        if (current_mode == MODE_MENU || current_mode == MODE_CHAT) {
            current_mode = (current_mode == MODE_MENU) ? MODE_CHAT : MODE_MENU;
        }
    }

    if (in_about) {
        handle_submenu_input();
    } else if (in_settings) {
        handle_submenu_input();
    } else if (current_mode == MODE_MENU) {
        handle_menu_input();
    }
    /* MODE_CHAT - handled by main.c */
}

/* Render menu */
void menu_render(void)
{
    if (in_about) {
        show_about();
    } else if (in_settings) {
        show_settings();
    } else if (current_mode == MODE_MENU) {
        render_main_menu();
    }
}

/* Get current app mode */
AppMode menu_get_mode(void)
{
    return current_mode;
}

/* Set app mode */
void menu_set_mode(AppMode mode)
{
    current_mode = mode;
    if (mode != MODE_MENU) {
        in_about = 0;
        in_settings = 0;
    }
}