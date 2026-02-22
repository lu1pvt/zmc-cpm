#include <stdio.h>
#include <stdint.h>
#include "zmc.h"


AppState App;


uint8_t CONFIG[] = { // 80x40
    80,  // Columns
    32   // Lines
};


char cmdline[CMDLINELEN+1];

uint8_t DEBUG = 0;
uint8_t DEVEL = 0;

const uint8_t *COLUMNS = CONFIG;
const uint8_t *LINES = CONFIG+1;

uint16_t MAX_FILES = 0;


void show_cursor() {
    printf("\x1b[?25h");
}


void hide_cursor() {
    printf("\x1b[?25l");
}


void set_invers() {
    printf("\x1b[7m");
}


void set_normal() {
    printf("\x1b[0m");
}


void clrscr() {
    printf("\x1b[2J\x1b[?25l");
}

void print_cpm_attrib( uint8_t *ca) {
    // show file attributes
    printf( "%c%c%c",
            *ca++ > 0x7F ? 'R' : ' ', // READ ONLY
            *ca++ > 0x7F ? 'S' : ' ', // SYSTEM
            *ca++ > 0x7F ? 'B' : ' '  // file was BACKED UP
    );
}


void show_prompt() {
    printf("\x1b[%d;1H\x1b[0m%c> %s\x1b[?25h\x1b[K",
           PANEL_HEIGHT+1, App.active_panel->drive, cmdline );
}


void refresh_ui(uint8_t which_panel) {
    if ( which_panel & 0b01) {
        if ( App.left.active )
            draw_panel(&App.left, 1);
        else if ( App.right.active )
            draw_panel(&App.right, *COLUMNS/2+1);
    }
    if ( which_panel & 0b10) {
        if ( App.right.active )
            draw_panel(&App.left, 1);
        else if ( App.left.active )
            draw_panel(&App.right, *COLUMNS/2+1);
    }
    if ( PANEL_WIDTH >= 40 )
        printf("\x1b[%d;1H\x1b[7m| A: - P: | TAB:Sw | F1:Help | F3:View | F4:Dump | F5:Copy | F8:Del | F10:Exit |\x1b[0m", PANEL_HEIGHT+2);
    else if ( PANEL_WIDTH >= 30 )
        printf("\x1b[%d;1H\x1b[7mA:-P:|TAB:Sw|F1:Help|F3:View|F4:Dump|F5:Copy|F8:Del|F10:Exit\x1b[0m", PANEL_HEIGHT+2);
    show_prompt();
}



