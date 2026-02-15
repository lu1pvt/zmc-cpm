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
#include <string.h>
#include "zmc.h"

void draw_frame(int x, int y, int w, int h, char *title) {
    int i;
    printf("\x1b[%d;%dH+", y, x);
    for(i=0; i<w-2; i++) putchar('-');
    putchar('+');
    printf("\x1b[%d;%dH[ %s ]", y, x + 2, title);
    for(i=1; i<h-1; i++) {
        printf("\x1b[%d;%dH|", y + i, x);
        printf("\x1b[%d;%dH|", y + i, x + w - 1);
    }
    printf("\x1b[%d;%dH+", y + h - 1, x);
    for(i=0; i<w-2; i++) putchar('-');
    putchar('+');
}

void draw_file_info( Panel *p, int f_idx ) {
    if (p->active && f_idx == p->current_idx)
        set_invers();

    printf("%c%-12s  %4uK", p->files[f_idx].selected ? '*' : ' ',
           p->files[f_idx].cpmname,
           p->files[f_idx].size_kb
           );
    // size calculation does not yet work
    // printf("%c%-12s  %4uK", p->files[f_idx].cpmname, p->files[f_idx].size_kb);

    if (p->files[f_idx].date) { // date and time defined
        printf(" %04d-%02d-%02d %02X:%02X",
            p->files[f_idx].date,
            p->files[f_idx].month,
            p->files[f_idx].day,
            p->files[f_idx].hour,
            p->files[f_idx].minute
        );
    } else {
        printf("                 " );
    }
    if (p->active && f_idx == p->current_idx)
        set_normal();
}


void draw_panel(Panel *p, uint8_t x_offset) {
    int i;
    char title[20];
    //int visible_rows = VISIBLE_ROWS; // macro  Wormetti
    /* Lógica de Scroll para 28 filas visibles */
    if (p->current_idx < p->scroll_offset) {
        p->scroll_offset = p->current_idx;
    }
    if (p->current_idx >= p->scroll_offset + VISIBLE_ROWS) {
        p->scroll_offset = p->current_idx - (VISIBLE_ROWS - 1);
    }
    printf("\x1b[m"); // normal
    sprintf(title, " DISK %c: ", p->drive);
    draw_frame(x_offset, 1, PANEL_WIDTH, PANEL_HEIGHT, title);

    for (i = 0; i < VISIBLE_ROWS; i++) {
        int f_idx = i + p->scroll_offset;
        printf("\x1b[%d;%dH", i + 2, x_offset + 1); // gotoyx
        if (f_idx < p->num_files)
            draw_file_info( p, f_idx );
        else
            printf("                                     ");
    }
}


void draw_file_line(Panel *p, uint8_t x_offset, uint16_t file_idx) {
    int screen_row = (file_idx - p->scroll_offset) + 2;
    if (file_idx >= p->scroll_offset && file_idx < p->scroll_offset + VISIBLE_ROWS) {
        printf("\x1b[%d;%dH", screen_row, x_offset + 1); // gotoyx
        draw_file_info( p, file_idx );
    }
}

