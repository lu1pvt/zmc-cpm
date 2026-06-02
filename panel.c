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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vlib.h"
#include "zmc.h"

static void putchar_xy( uint8_t col, uint8_t row, char c ) {
    static char buf[ 4 ];
    sprintf( buf, "%c%c%c", row, col, c );
    gxymsg( buf );
}


// draw one line with file name, attributes, size, date etc.
static void draw_file_info( Panel *p, int16_t f_idx ) {
    if ( App.active_panel == p && p->num_files && f_idx == p->current_idx )
        stndout();

    printf( "%c%-12s %c%c%c", p->files[ f_idx ].attrib & B_SEL ? '*' : ' ', p->files[ f_idx ].cpmname,
            p->files[ f_idx ].attrib & B_RO ? 'R' : ' ', p->files[ f_idx ].attrib & B_SYS ? 'S' : ' ',
            p->files[ f_idx ].attrib & B_ARCH ? 'A' : ' ' );

    if ( !*( p->files[ f_idx ].cpmname ) )
        printf( "      " );
    else if ( p->files[ f_idx ].extent < 512 )                             // file size < 64K
        printf( "%6u", p->files[ f_idx ].extent << 7 );                    // *128
    else if ( p->files[ f_idx ].extent < 7812 )                            // 64K <= file size < 1E6
        printf( "%6lu", (uint32_t)p->files[ f_idx ].extent << 7 );         // *128 -> uint32_t
    else                                                                   // 1E6 <= file size < 8 MB
        printf( "%5uK", (uint16_t)( p->files[ f_idx ].extent + 7 ) >> 3 ); // *128/1024

    uint8_t w;
    if ( p->show_date ) {
        if ( p->files[ f_idx ].date ) { // date and time defined
            printf( " %04d%s%02d%s%02d %02X%s%02X", p->files[ f_idx ].date, PANEL_WIDTH < 42 ? "" : "-", p->files[ f_idx ].month,
                    PANEL_WIDTH < 42 ? "" : "-", p->files[ f_idx ].day, p->files[ f_idx ].hour, PANEL_WIDTH < 42 ? "" : ":",
                    p->files[ f_idx ].minute );
        } else { // spaces instead of date
            w = PANEL_WIDTH < 42 ? 14 : 17;
            while ( w-- )
                putchar( ' ' );
        }
        if ( App.active_panel == p && f_idx == p->current_idx )
            stndend();
        // fill to end of panel line
        w = PANEL_WIDTH < 42 ? PANEL_WIDTH - 39 : PANEL_WIDTH - 42;
        while ( w-- )
            putchar( ' ' );
    } else { // no date for complete drive or panel too narrow
        if ( App.active_panel == p && f_idx == p->current_idx )
            stndend();
        w = PANEL_WIDTH - 25;
        while ( w-- )
            putchar( ' ' );
    }
}


// draw the empty wire frame
void draw_frame( Panel *p ) {
    uint16_t i;

    // B2 = 1 if Graphics On/Off present, 0 if absent
    // B3 = 1 if 13 Graphics chars exist, 0 if any absent
    if ( ( VLIB_STATUS & 0b1100 ) == 0b1100 ) { // draw graphic box
        grxon();
        if ( &App.left == p )
            gbox( 0x0101, PANEL_WIDTH << 8 | PANEL_HEIGHT );
        else
            gbox( 0x100 | (PANEL_WIDTH + 1), PANEL_WIDTH << 8 | PANEL_HEIGHT );
        grxoff();
    } else { // use ASCII char for box
        if ( &App.left == p )
            gotoxy( 1 << 8 | 1 );
        else
            gotoxy( 1 << 8 | PANEL_WIDTH + 1 );
        // top margin
        putchar( ' ' );
        i = PANEL_WIDTH - 2;
        while ( i-- )
            putchar( '_' );
        putchar( ' ' );
        // left and right margin
        for ( i = 0; i < VISIBLE_ROWS; i++ ) {
            int16_t f_idx = i + p->top_idx;
            if ( &App.left == p ) {
                putchar_xy( PANEL_WIDTH, i + 2, '|' );
                gotoxy( ( i + 2 ) << 8 | PANEL_WIDTH - 1 );
                putchar_xy( 1, i + 2, '|' );
            } else {
                putchar_xy( PANEL_WIDTH + 1, i + 2, '|' );
                // ereol();
                putchar_xy( 2 * PANEL_WIDTH, i + 2, '|' );
            }
        }
        // bottom margin
        if ( &App.left == p )
            gotoxy( PANEL_HEIGHT << 8 | 1 );
        else
            gotoxy( PANEL_HEIGHT << 8 | PANEL_WIDTH + 1 );
        i = PANEL_WIDTH - 2;
        putchar( '|' );
        while ( i-- )
            putchar( '_' );
        putchar( '|' );
    }
}


// fill the prepared panel with the file lines
void fill_panel( Panel *p ) {
    uint8_t x_offset = p == &App.left ? 2 : PANEL_WIDTH + 2;
    uint8_t i;

    if ( p->current_idx < p->top_idx ) {
        p->top_idx = p->current_idx;
    }
    if ( p->current_idx >= p->top_idx + VISIBLE_ROWS ) {
        p->top_idx = p->current_idx - ( VISIBLE_ROWS - 1 );
    }
    stndend();
    for ( i = 0; i < VISIBLE_ROWS; i++ ) {
        int f_idx = i + p->top_idx;
        gotoxy( ( i + 2 ) << 8 | x_offset );
        if ( f_idx < p->num_files ) {
            draw_file_info( p, f_idx );
        } else {
            uint8_t j = PANEL_WIDTH - 2;
            while ( j-- )
                putchar( ' ' );
        }
    }
}


// put one file info at defined position
void draw_file_line( Panel *p, int16_t file_idx ) {
    uint8_t x_offset = p == &App.left ? 2 : PANEL_WIDTH + 2;
    if ( file_idx >= p->top_idx && file_idx < p->top_idx + VISIBLE_ROWS ) {
        gotoxy( ( file_idx - p->top_idx + 2 ) << 8 | x_offset );
        draw_file_info( p, file_idx );
    }
}


static void print_cpm_attrib( uint8_t *ca ) {
    // show file attributes
    printf( "%c%c%c",
            *ca++ > 0x7F ? 'R' : ' ', // READ ONLY
            *ca++ > 0x7F ? 'S' : ' ', // SYSTEM
            *ca++ > 0x7F ? 'B' : ' '  // file was BACKED UP
    );
}


// show the disk on top of panel, mark the active panel
void draw_header( Panel *p ) {
    uint8_t x_offset = p == &App.left ? 3 : PANEL_WIDTH + 3;
    gotoxy( 1 << 8 | x_offset );
    if ( ( VLIB_STATUS & 0b1100 ) == 0b1100 ) {
        grxon();
        rtisec();
        if ( p == App.active_panel )
            stndout();
        printf( " DISK %c: ", p->drive );
        if ( p == App.active_panel )
            stndend();
        ltisec();
        grxoff();
    } else {
        if ( p == App.active_panel )
            stndout();
        printf( "[ DISK %c: ]", p->drive );
        if ( p == App.active_panel )
            stndend();
    }
}


// show function key help
void draw_footer( void ) {
    gotoxy( SCREEN_HEIGHT << 8 | 1 );
    stndout();
    if ( PANEL_WIDTH >= 40 ) {
        printf( "| A: - P: | TAB:Sw | F1:Help | F3:View | F4:Dump | F5:Copy | F8:Del | F10:Exit |" );
    } else if ( PANEL_WIDTH >= 30 ) {
        printf( "A:-P:|TAB:Sw|F1:Help|F3:View|F4:Dump|F5:Copy|F8:Del|F10:Exit" );
    }
    stndend();
}


// cmd line with active drive
void show_prompt() {
    gotoxy( STATUS_ROW << 8 | 1 );
    printf( "%c> %s", App.active_panel->drive, cmdline );
    ereol();
    curon();
}


// update none, one or both panels
void refresh_ui( uint8_t which_panel ) {
    curoff();
    if ( which_panel & 0b01 ) {
        draw_frame( App.active_panel );
        draw_header( App.active_panel );
        fill_panel( App.active_panel );
    }
    if ( which_panel & 0b10 ) {
        draw_frame( App.inactive_panel );
        draw_header( App.inactive_panel );
        fill_panel( App.inactive_panel );
    }
    draw_footer();
}


void change_focus() {
    Panel *tmp = App.active_panel;
    App.active_panel = App.inactive_panel;
    App.inactive_panel = tmp;
    draw_header( &App.left );
    draw_header( &App.right );
    // chirurgical update: refresh only the lines with cursors
    draw_file_line( &App.left, App.left.current_idx );
    draw_file_line( &App.right, App.right.current_idx );
}


void select_file() {
    if ( !App.active_panel->num_files )
        return;
    int16_t idx = App.active_panel->current_idx;
    // A. invert the selection state in memory
    App.active_panel->files[ idx ].attrib ^= B_SEL;
    // B. redraw current line to show '*'
    // IMPORTANT: current_idx was not changed, line is drawn with cursor.
    draw_file_line( App.active_panel, idx );
    // C. move the cursor to the next line
    line_down();
}


// is this index on the panel?
uint8_t is_on_panel( int16_t idx ) {
    return ( idx >= App.active_panel->top_idx && idx < App.active_panel->top_idx + VISIBLE_ROWS );
}


static void goto_line( int16_t new_idx ) {
    int16_t old_idx = App.active_panel->current_idx;
    if ( new_idx < 0 || old_idx == new_idx ) // no files or already there
        return;
    App.active_panel->current_idx = new_idx;
    if ( is_on_panel( new_idx ) ) {                  // update only two lines
        draw_file_line( App.active_panel, old_idx ); // deselect
        draw_file_line( App.active_panel, new_idx ); // select
    } else                                           // redraw everything
        fill_panel( App.active_panel );
}


void line_up() {
    if ( App.active_panel->current_idx > 0 )
        goto_line( App.active_panel->current_idx - 1 );
}


void line_down() {
    if ( App.active_panel->current_idx + 1 < App.active_panel->num_files )
        goto_line( App.active_panel->current_idx + 1 );
}


void page_up() {
    if ( App.active_panel->current_idx >= VISIBLE_ROWS )
        goto_line( App.active_panel->current_idx - VISIBLE_ROWS );
    else
        goto_line( 0 );
}


void page_down() {
    if ( App.active_panel->current_idx + VISIBLE_ROWS < App.active_panel->num_files )
        goto_line( App.active_panel->current_idx + VISIBLE_ROWS );
    else
        goto_line( App.active_panel->num_files - 1 );
}


void first_file() { goto_line( 0 ); }


void last_file() { goto_line( App.active_panel->num_files - 1 ); }
