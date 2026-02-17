/*
Z80 Management Commander (ZMC)
Copyright (C) 2026 Volney Torres

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "zmc.h"

AppState App;

unsigned char wait_key_hw() {
#asm
loop_bdos_kbd:
    ld c, 06h    ; function 6: Direct I/O
    ld e, 0FFh   ; check w/o echo
    call 0005h   ; BDOS
    or a         ; key available?
    jr z, loop_bdos_kbd
    ld l, a      ; return key in HL
    ld h, 0
#endasm
}


void refresh_ui(unsigned char all) {
    if ( all || App.left.active )
        draw_panel(&App.left, 1);
    if ( all || App.right.active )
        draw_panel(&App.right, *COLUMNS/2+1);
    printf("\x1b[%d;1H\x1b[0m%c> ", *LINES - 1, App.active_panel->drive);
    printf("\x1b[%d;1H\x1b[7m| A: - P: | TAB:Sw | F1:Help | F3:View | F4:Dump | F5:Copy | F8:Del | F10:Exit |\x1b[0m", *LINES);
}


void other_panel() {
    int old_left_idx = App.left.current_idx;
    int old_right_idx = App.right.current_idx;

    // change focus
    App.left.active = !App.left.active;
    App.right.active = !App.right.active;
    App.active_panel = App.left.active ? &App.left : &App.right;

    // chirurgical update: refresh only the lines with cursors
    draw_file_line(&App.left, 1, old_left_idx);
    draw_file_line(&App.right, *COLUMNS/2+1, old_right_idx);

    // ... and the prompt
    printf("\x1b[%d;1H\x1b[m%c> \x1b[K", *LINES-1, App.active_panel->drive); // pos, normal. clr eol
}


void change_drive( char k ) {
    if ( k >= 'a' && k <= 'p') k -= 32;
    printf("\x1b[%d;1H\x1b[7m\x1b[5m%c>\x1b[m", *LINES-1, k); // CUP row 31 col 1;
    if ( wait_key_hw() == ':' ) {
        App.active_panel->drive = k;
        load_directory(App.active_panel);
    }
    refresh_ui( 0 );
}


void select_file() {
    int idx = App.active_panel->current_idx;
    int offset = (App.active_panel == &App.left) ? 1 : *COLUMNS/2+1;

    // A. invert the selection state in memory
    App.active_panel->files[idx].attrib ^= 0x80;

    // B. redraw current line to show '*'
    // IMPORTANT: current_idx was not changed, line is drawn with cursor.
    draw_file_line(App.active_panel, offset, idx);

    // C. move the cursor to the next line
    if (App.active_panel->current_idx < App.active_panel->num_files - 1) {
        int old_idx = App.active_panel->current_idx;
        App.active_panel->current_idx++;

        // D. redraw previous line (w/o cursor but with '*')
        draw_file_line(App.active_panel, offset, old_idx);

        // E. redraw new line (with cursor)
        draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
    }
}


void line_up() {
    if (App.active_panel->current_idx > 0) {
        int old_idx = App.active_panel->current_idx;
        App.active_panel->current_idx--;

        // if scrolling, redraw everything; if not, only two lines
        if (App.active_panel->current_idx < App.active_panel->scroll_offset) {
            refresh_ui(0);
        } else {
            int offset = (App.active_panel == &App.left) ? 1 : *COLUMNS/2+1;
            draw_file_line(App.active_panel, offset, old_idx);
            draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
        }
    }
}


void line_down() {
    if (App.active_panel->current_idx < App.active_panel->num_files - 1) {
        int old_idx = App.active_panel->current_idx;
        App.active_panel->current_idx++;

        // if scrolling, redraw everything; if not, only two lines
        if (App.active_panel->current_idx >= App.active_panel->scroll_offset + VISIBLE_ROWS) {
            refresh_ui(0);
        } else {
            int offset = (App.active_panel == &App.left) ? 1 : *COLUMNS/2+1;
            draw_file_line(App.active_panel, offset, old_idx);
            draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
        }
    }
}


void page_up() {
    if (App.active_panel->current_idx >= VISIBLE_ROWS/2)
        App.active_panel->current_idx -= VISIBLE_ROWS/2;
    else
        App.active_panel->current_idx = 0;
    refresh_ui(0);
}


void page_down() {
    App.active_panel->current_idx += VISIBLE_ROWS/2;
    if (App.active_panel->current_idx >= App.active_panel->num_files)
        App.active_panel->current_idx = App.active_panel->num_files - 1;
    refresh_ui(0);
}


void first_file() {
    App.active_panel->current_idx = 0;
    refresh_ui(0);
}


void last_file() {
    App.active_panel->current_idx = App.active_panel->num_files - 1;
    refresh_ui(0);
}


uint8_t yes_no() {
    char k = wait_key_hw();
    return (k == 'y' || k == 'Y');
}


void copy() {
    Panel *dest = (App.active_panel == &App.left) ? &App.right : &App.left;
    // clear dialog box and ask
    printf("\x1b[%d;1H\x1b[K COPY SELECTED FILES TO %c:? (Y/N) ", *LINES-1, dest->drive); // pos, erase EOL
    if ( yes_no() )
        // Y: copy multiple files
        exec_multi_copy(App.active_panel, dest);
    // clear status line
    printf("\x1b[%d;1H\x1b[K", *LINES-1); // pos, erase EOL
}


void delete() {
    // clear dialog box and ask
    printf("\x1b[%d;1H\x1b[K DELETE SELECTED FILES? (Y/N) ", *LINES-1); // pos, erase EOL
    if ( yes_no() ) {
        // Y: call master function
        exec_multi_delete(App.active_panel);
        load_directory(App.active_panel);
    }
    // clear status line
    printf("\x1b[%d;1H\x1b[K", *LINES-1); // pos, erase EOL
    refresh_ui(0); // file(s) deleted, refresh active panel
}


int main(int argc, char** argv) {


    // CP/M Plus has values for screen size in System Control Block
    if ( bdos( 12, NULL ) == 0x31 ) { // version == CP/M Plus
        // handle BDOS errors internally, do not exit
        bdos( 45, 0xFF ); // set BDOS return error mode 1
        uint8_t scbpb[4] = { 0x1A, 0, 0, 0 }; // SCB parameter block, get col - 1
        *COLUMNS = bdos( 49, scbpb ) + 1;
        scbpb[0] = 0x1C; // lines - 1
        *LINES = bdos( 49, scbpb ) + 1;
    }

    uint16_t total;
    uint16_t largest;
    // total   = address where the total number of free bytes in the heap will be stored
    // largest = address where the size of the largest available block in the heap will be stored
    mallinfo( &total, &largest );

    // calculate number of file entries
    MAX_FILES = largest / sizeof( FileEntry ) / 2;

    // cmd line argument "--config" shows address of screen size constants
    // in zmc.com to help the user to patch with a HEX editor, e.g. BE.
    while ( --argc ) {
        ++argv;
        if ( !strcmp( *argv, "--CONFIG" ) ) {
            uint16_t value = bdos( 12, NULL );
            printf( "COLUMNS @ 0x%04X: %d\n", COLUMNS - 0x100, *COLUMNS );
            printf( "LINES @ 0x%04X: %d\n", LINES - 0x100, *LINES );
            printf( "MAX_FILES: %u\n", MAX_FILES );
            return 0;
        } else if ( !strcmp( *argv, "--DEVEL" ) ) {
            ++DEVEL;
        } else if ( !strcmp( *argv, "--DEBUG" ) ) {
            ++DEBUG;
        } else if ( !strcmp( *argv, "--KEY" ) ) {
            // test for terminal function keys, exit with <ESC><ESC>
            uint8_t k;
            for(;;) {
                static uint8_t esc = 0;
                k = wait_key_hw();
                printf( "0x%02X  ", k );
                if ( k == ESC )
                    puts( "ESC" );
                else if (k < ESC )
                    printf( "^%c\n", k + '@');
                else    // show printable chars, else '.'
                    printf( "%c\n", k >= ' ' && k < 128 ? k : '.' );
                if ( esc && k == ESC ) // <ESC><ESC>
                    return 0;
                esc = k == ESC; // remember <ESC>
            }
        }
    }

    FileEntry *f_left;
    FileEntry *f_right;

    f_left = calloc( MAX_FILES, sizeof( FileEntry ) ); // reserve and init heap space
    if ( f_left == NULL ) {
        fprintf( stderr, "Not enough memory!\n" );
        return -1;
    }
    f_right = calloc( MAX_FILES, sizeof( FileEntry ) ); // reserve and init heap space
    if ( f_right == NULL ) {
        fprintf( stderr, "Not enough memory!\n" );
        return -1;
    }

    App.left.files = f_left;
    App.right.files = f_right;

    App.left.drive = '@'; App.left.active = 1; // current drive
    App.right.drive = '@'; App.right.active = 0; // current drive
    App.active_panel = &App.left;

    load_directory(&App.left);
    load_directory(&App.right);
    printf("\x1b[?25l\x1b[2J\x1b[H"); // hide cursor, clear, home
    refresh_ui( 1 ); // refresh/init both panels

    uint8_t loop = 1;
    uint8_t k;
    while( loop ) { // terminal key input loop
        k = wait_key_hw();

        if (k == 0x09) { // TAB: OTHER_PANEL
            other_panel();
        } else if ( k == 'v' || k == 'V' ) { // View ASCII
            view_file(App.active_panel);
            clrscr(); // clear screen, hide cursor
            refresh_ui(1);
        } else if ( k == 'x' || k == 'X' ) { // Dump HEX
            dump_file(App.active_panel);
            clrscr(); // clear screen, hide cursor
            refresh_ui(1);
        } else if (k == ' ' || k == 'V'-'@') { // ' ' or ^V -> SELECT
            select_file();
        } else if ( ( k >= 'a' && k <= 'p' ) || ( k >= 'A' && k <= 'P' ) ) {
            change_drive( k ); // "A:" - "P:" -> CHANGE_DRIVE
        } else if (k == ' ') { // ' ' -> SELECT
            select_file();
        } else if ( k == 'E'-'@' ) { // ^E
            line_up();
        } else if ( k == 'X'-'@' ) { // ^X
            line_down();
        } else if ( k == 'R'-'@' ) { // ^R
            page_up();
        } else if ( k == 'C'-'@' ) { // ^C
            page_down();
        } else if ( k == ESC ) { // ESC sequences
            k = wait_key_hw();
            if (k == ESC ) { // <ESC><ESC>
                break;
            } else if (k == '[') { // "<ESC>["
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
                    if (k == '5' && wait_key_hw() == '~') { // F5 = "<ESC>[15~" COPY
                        copy();
                    } else if (k == '9' && wait_key_hw() == '~') { // F8 = "<ESC>[19~" DELETE
                        delete();
                    }
                    refresh_ui(1);
                } else if ( k == '2' ) {
                    k = wait_key_hw();
                    if ( k == '~' ) { // <INSERT> = "<ESC>[2~"
                        select_file();
                    } else if (k == 1 && wait_key_hw() == '~') { // F10 = "<ESC>[21~"
                        loop = 0; // ready, leave loop
                    }
                }
            } else if (k == 'O') { // <ESC>O ...
	        k = wait_key_hw();
	        if (k == 'P') { // F1 = "<ESC>OP" HELP
	            printf("\x1b[%d;1H\x1b[7m ZMC v1.2 - Volney Torres \x1b[0m", *LINES-1);
	            wait_key_hw(); // wait for any key
                    printf("\x1b[%d;1H\x1b[K", *LINES-1); // pos, erase EOL
	        }
	        else if (k == 'R') { // F3 = "<ESC>OR" VIEW
	            view_file(App.active_panel);
		    clrscr(); // clear screen, hide cursor
                    refresh_ui(1);
	        }
		else if (k == 'S') { // F4 = "<ESC>OS" DUMP
		    dump_file(App.active_panel);
		    clrscr(); // clear screen, hide cursor
                    refresh_ui(1);
		}
            }
        }
    }
    printf("\x1b[?25h"); // show cursor
    printf("\x1b[0m\x1b[2J\x1b[H"); // normal, cls, home
    return 0;
}

