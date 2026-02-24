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
#include "zmc.h"

void draw_frame(int x, int y, int w, int h, char *title) {
    int i;
    putchar_xy( x, y, '+' );
    for(i=0; i<w-2; ++i)
        putchar('-');
    putchar('+');
    goto_xy( x+2, y );
    printf("[ %s ]", title);
    for(i=1; i<h-1; ++i) {
        putchar_xy( x, y+i, '|' );
        putchar_xy( x+w-1, y+i, '|' );
    }
    putchar_xy( x , y+h-1, '+' );
    for( i = 0; i < w-2; ++i )
        putchar( '-' );
    putchar('+');
}


void draw_file_info( Panel *p, int f_idx ) {
    if (p->active && f_idx == p->current_idx)
        set_invers();

    printf("%c%-12s %c%c%c",
           p->files[f_idx].attrib & B_SEL ? '*' : ' ',
           p->files[f_idx].cpmname,
           p->files[f_idx].attrib & B_RO ? 'R' : ' ',
           p->files[f_idx].attrib & B_SYS ? 'S' : ' ',
           p->files[f_idx].attrib & B_ARCH ? 'A' : ' '
    );

    if ( p->files[f_idx].extent < 512) // file size < 64K
        printf( "%6u", p->files[f_idx].extent << 7 ); // *128
    else if ( p->files[f_idx].extent < 7812) // 64K <= file size < 1E6
        printf( "%6lu", (uint32_t)p->files[f_idx].extent << 7 ); // *128 -> uint32_t
    else // 1E6 <= file size < 8 MB
        printf( "%5uK", (uint16_t)(p->files[f_idx].extent + 7) >> 3 ); // *128/1024

    uint8_t w;
    if ( p->show_date ) {
        if ( p->files[f_idx].date) { // date and time defined
            printf(" %04d%s%02d%s%02d %02X%s%02X",
                p->files[f_idx].date,
                PANEL_WIDTH < 42 ? "" : "-",
                p->files[f_idx].month,
                PANEL_WIDTH < 42 ? "" : "-",
                p->files[f_idx].day,
                p->files[f_idx].hour,
                PANEL_WIDTH < 42 ? "" : ":",
                p->files[f_idx].minute
            );
        } else { // spaces instead of date
            w = PANEL_WIDTH < 42 ? 14 : 17;
            while ( w-- )
                putchar( ' ' );
        }
        if (p->active && f_idx == p->current_idx)
            set_normal();
        // fill to end of panel line
        w = PANEL_WIDTH < 42 ? PANEL_WIDTH - 39 : PANEL_WIDTH - 42;
        while ( w-- )
            putchar( ' ' );
    } else { // no date for complete drive or panel too narrow
        if (p->active && f_idx == p->current_idx)
            set_normal();
        w = PANEL_WIDTH - 25;
        while ( w-- )
            putchar( ' ' );
    }
}


void draw_panel(Panel *p, uint8_t x_offset) {
    uint8_t i;
    char title[10];
    if (p->current_idx < p->scroll_offset) {
        p->scroll_offset = p->current_idx;
    }
    if (p->current_idx >= p->scroll_offset + VISIBLE_ROWS) {
        p->scroll_offset = p->current_idx - (VISIBLE_ROWS - 1);
    }
    set_normal();
    sprintf(title, " DISK %c: ", p->drive);
    draw_frame(x_offset, 1, PANEL_WIDTH, PANEL_HEIGHT, title);

    for (i = 0; i < VISIBLE_ROWS; i++) {
        int f_idx = i + p->scroll_offset;
        goto_xy( x_offset + 1, i + 2 );
        if (f_idx < p->num_files)
            draw_file_info( p, f_idx );
        else {
            uint8_t w = PANEL_WIDTH-2;
            while ( w-- )
                putchar( ' ' );
        }
    }
}


void draw_file_line(Panel *p, uint8_t x_offset, uint16_t file_idx) {
    if (file_idx >= p->scroll_offset && file_idx < p->scroll_offset + VISIBLE_ROWS) {
        goto_xy( x_offset + 1, file_idx - p->scroll_offset + 2 );
        draw_file_info( p, file_idx );
    }
}

