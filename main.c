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

// strip the MS-DOS protection header
#pragma output noprotectmsdos

// do not insert the file redirection option while parsing the command line arguments
#pragma output noredir

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <cpm.h>
#include "zmc.h"


extern AppState App;


uint8_t wait_key_hw() {
    // use raw BIOS CONIO (fkt 3) to ignore XON/XOFF (^Q is used as fkt key)
    uint8_t k = bios( BIOS_CONIN, 0, 0 ); // function, BC, DE, returns A
    if ( k == RUB ) // translate RUB to BS
        k = BS;
    return k;
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
    draw_file_line(&App.right, PANEL_WIDTH+1, old_right_idx);
    show_prompt();
}


void change_drive( char k ) {
    App.active_panel->drive = k;
    load_directory(App.active_panel);
    refresh_ui( PAN_ACTIVE );
}


void select_file() {
    int idx = App.active_panel->current_idx;
    int offset = (App.active_panel == &App.left) ? 1 : PANEL_WIDTH+1;

    // A. invert the selection state in memory
    App.active_panel->files[idx].attrib ^= B_SEL;

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
            refresh_ui( PAN_ACTIVE );
        } else {
            int offset = (App.active_panel == &App.left) ? 1 : PANEL_WIDTH+1;
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
            refresh_ui( PAN_ACTIVE );
        } else {
            int offset = (App.active_panel == &App.left) ? 1 : PANEL_WIDTH+1;
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
    refresh_ui( PAN_ACTIVE );
}


void page_down() {
    App.active_panel->current_idx += VISIBLE_ROWS/2;
    if (App.active_panel->current_idx >= App.active_panel->num_files)
        App.active_panel->current_idx = App.active_panel->num_files - 1;
    refresh_ui( PAN_ACTIVE );
}


void first_file() {
    App.active_panel->current_idx = 0;
    refresh_ui( PAN_ACTIVE );
}


void last_file() {
    App.active_panel->current_idx = App.active_panel->num_files - 1;
    refresh_ui( PAN_ACTIVE );
}


uint8_t yes_no() {
    char k = wait_key_hw();
    return (k == 'y' || k == 'Y');
}


void copy() {
    Panel *dest = (App.active_panel == &App.left) ? &App.right : &App.left;
    // clear dialog box and ask
    goto_xy( 1, PANEL_HEIGHT+1 );
    clr_line_right();
    printf(" COPY SELECTED FILE(S) TO %c:? (Y/N) ", dest->drive);
    if ( yes_no() )
        // Y: copy multiple files
        exec_multi_copy(App.active_panel, dest);
    // clear status line
    goto_xy( 1, PANEL_HEIGHT+1 );
    clr_line_right();
    refresh_ui( PAN_OTHER );
}


void delete() {
    // clear dialog box and ask
    goto_xy( 1, PANEL_HEIGHT+1 );
    clr_line_right();
    printf(" DELETE SELECTED FILE(S)? (Y/N) " );
    if ( yes_no() ) {
        // Y: call master function
        exec_multi_delete(App.active_panel);
        load_directory(App.active_panel);
    }
    // clear status line
    goto_xy( 1, PANEL_HEIGHT+1 );
    clr_line_right();
    refresh_ui( PAN_ACTIVE ); // file(s) deleted, refresh active panel
}


void help() {
    uint8_t line = 1;
    hide_cursor();
    set_normal();
    clr_scr();
    goto_xy( 1, line++ );
    if ( SCREEN_HEIGHT >= 20) {
        puts( "                           " );
        puts( " #######  #     #   #####  " );
        puts( "      #   ##   ##  #     # " );
        puts( "     #    # # # #  #       " );
        puts( "    #     #  #  #  #       " );
        puts( "   #      #     #  #       " );
        puts( "  #       #     #  #     # " );
        puts( " #######  #     #   #####  " );
        puts( "                           " );
        line = 12;
    }

#ifndef i8080
    puts( "= ZMC 1.3 - Volney Torres =" );
#else
    puts( "= ZMC 1.3 (8080 version) - Volney Torres =" );
#endif

    goto_xy( 1, line );
    printf( "A: ... P:" );
    goto_xy( 32, line++ );
    puts( "Select drive" );
    printf( "[TAB]" );
    goto_xy( 32, line++ );
    puts( "Change panel" );
    printf( "[F3], TYPE, VIEW, CAT");
    goto_xy( 32, line++ );
    puts( "Show file" );
    printf( "[F4], DUMP, HEX" );
    goto_xy( 32, line++ );
    puts( "Hexdump file" );
    printf( "[F5], COPY, CP" );
    goto_xy( 32, line++ );
    puts( "Copy file(s)");
    printf( "[F8], DEL, ERA, RM" );
    goto_xy( 32, line++ );
    puts( "Delete file(s)" );
    printf( "[F9], [ESC][ESC], QUIT, EXIT" );
    goto_xy( 32, line++ );
    puts( "Exit" );
    wait_key_hw();
    refresh_ui( PAN_BOTH );
}

typedef void (*command_func_t)(void);

typedef struct {
    const char *keyword;
    command_func_t func;
} command_t;

// parse zmc cmd line and get a fkt pointer
command_func_t find_command(const char *input, command_t commands[], int num_commands) {
    for (int i = 0; i < num_commands; i++) {
        if (strncmp(input, commands[i].keyword, strlen(commands[i].keyword)) == 0) {
            return commands[i].func;
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    // CP/M Plus has values for screen size in System Control Block
    if ( bdos( 12, NULL ) == 0x31 ) { // version == CP/M Plus
        // handle BDOS errors internally, do not exit
        bdos( 45, 0xFF ); // set BDOS return error mode 1
        uint8_t scbpb[ 4 ] = { 0x1A, 0, 0, 0 }; // SCB parameter block, get col - 1
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
                puts( "function key test - exit with <ESC><ESC>" );
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
    clr_scr();
    goto_xy( 1, 1 );
    hide_cursor();
    refresh_ui( PAN_BOTH ); // refresh/init both panels

    uint8_t loop = 1;
    uint8_t k;

    char *cp = cmdline;
    *cp = '\0';

    // list of text commands from prompt line and called function
    command_t commands[] = {
        { "HELP", help },

        { "VIEW", view_file },
        { "TYPE", view_file },
        { "CAT", view_file },

        { "DUMP", dump_file },
        { "HEX", dump_file },

        { "COPY", copy },
        { "CP", copy },

        { "DEL", delete },
        { "ERA", delete },
        { "RM", delete },

        { "TOP", first_file },
        { "POS1", first_file },

        { "BOT", last_file },
        { "END", last_file },
    };

    int num_commands = sizeof(commands) / sizeof(commands[0]);

    while( loop ) { // terminal key input loop
        k = wait_key_hw();
        show_cursor();
        // printable char go to the prompt line, BS deletes, CR executes
        if ( k > SPC ) {
            if ( cp < cmdline + CMDLINELEN ) {
                *cp++ = toupper( k );
                *cp = '\0';
            }
        } else if ( k == BS ) {
            if ( cp > cmdline )
                *--cp = '\0';
        } else if ( k == CR ) { // cmd line parser
            // "A:" .. "P:"
            if ( cmdline[1] == ':' && *cmdline >= 'A' && *cmdline <= 'P' ) {
                change_drive( *cmdline );
            }
            else if ( !strncmp( cmdline, "EXIT", 4 )
                || !strncmp( cmdline, "QUIT", 4 ) ) {
                loop = 0;
            }
            else if ( *cmdline ) { // scan cmd list and get function
                command_func_t function = find_command( cmdline, commands, num_commands );
                if ( function ) // if found ..
                    function(); // .. execute it
            }
            // clear cmd string
            cp = cmdline;
            *cp = '\0';
        }
        else if ( k == TAB ) { // TAB: OTHER_PANEL
            other_panel();
        }
        // here come the function keys, first the WS bindings
        else if (k == ' ' || k == 'V'-'@') { // ' ' or ^V -> SELECT
            select_file();
        } else if ( k == 'E'-'@' ) { // ^E
            line_up();
        } else if ( k == 'X'-'@' ) { // ^X
            line_down();
        } else if ( k == 'R'-'@' ) { // ^R
            page_up();
        } else if ( k == 'C'-'@' ) { // ^C
            page_down();
        }
        // now the multi character function keys starting with <ESC>
        else if ( k == ESC ) { // ESC sequences
            k = wait_key_hw();
            if ( k == ESC ) // <ESC><ESC>
                loop = 0; // quit program
            else
                loop = esc_seq( k );
        }
        if ( loop )
            show_prompt();
    } // while ( loop )
    clr_scr();
    goto_xy( 1, 1 );
    show_cursor();
    set_normal();
    return 0;
}

