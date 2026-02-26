/*
Z80 Management Commander (ZMC)
Copyright (C) 2026 Volney Torres & Martin Homuth-Rosemann

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// interface functions for VT100 terminal output

#include <stdint.h>
#include <stdio.h>
#include "zmc.h"


// handle VT100 function keys starting with <ESC>
uint8_t parse_function_keys( uint8_t k ) {
    uint8_t loop = 1;
    // the VT100 CSI commands starting with <ESC>[
    if ( k == '[' ) {
        k = wait_key_hw();
        if ( k == 'A' ) { // "<ESC>[A" LINE_UP
            line_up();
        } else if ( k == 'B' ) { // "<ESC>[B" LINE_DOWN
            line_down();
        } else if ( k == '5' && wait_key_hw() == '~' ) { // "<ESC>[5~" PAGE_UP
            page_up();
        } else if ( k == '6' && wait_key_hw() == '~' ) { // "<ESC>[6~" PAGE_DOWN
            page_down();
        } else if ( k == 'H' ) { // <HOME> = "<ESC>[H"
            first_file();
        } else if ( k == 'F' ) { // <END> = "<ESC>[F"
            last_file();
        } else if ( k == '1' ) { // F5 = "<ESC>[15~" / F8 = "<ESC>[19~"
            k = wait_key_hw();
            if ( k == '5' && wait_key_hw() == '~' ) { // F5 = "<ESC>[15~" COPY
                copy();
            } else if ( k == '9' && wait_key_hw() == '~' ) { // F8 = "<ESC>[19~" DELETE
                delete();
            } else if ( k > '5' && k < '9' ) {
                wait_key_hw(); // remove '~'
            }
        } else if ( k == '2' ) {
            k = wait_key_hw();
            if ( k == '~' ) { // <INSERT> = "<ESC>[2~"
                select_file();
            } else if ( k == '1' && wait_key_hw() == '~' ) { // F10 = "<ESC>[21~"
                loop = 0; // ready, leave loop
            } else if ( k >= '0' && k <= '9') {
                wait_key_hw(); // remove '~'
            }
        }
    }
    // the VT100 PF1 ... PF4 keys <ESC>OP ... <ESC>OS
    else if ( k == 'O' ) {
        k = wait_key_hw();
        if ( k == 'P' ) { // F1 = "<ESC>OP" HELP
            help();
        }
        else if ( k == 'R' ) { // F3 = "<ESC>OR" VIEW
            view_file();
        }
        else if ( k == 'S' ) { // F4 = "<ESC>OS" DUMP
            dump_file();
        }
    }
    return loop;
}


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


void clr_scr() {
    printf("\x1b[2J");
}


void goto_xy( uint8_t col, uint8_t row ) {
    printf( "\x1b[%d;%dH", row, col );
}


void putchar_xy( uint8_t col, uint8_t row, char c ) {
    printf( "\x1b[%d;%dH%c", row, col, c );
}

void clr_line_right() {
    printf( "\x1b[K" );
}

void clr_line_left() { // erase to begin of line
    printf( "\x1b[1K" );
}

void clr_line() {
    printf( "\x1b[2K" );
}

