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
#include <stdio.h>
#include <string.h>
#include "zmc.h"

extern uint8_t *LINES;
extern uint8_t *COLUMNS;

void draw_frame(int x, int y, int w, int h, char *title) {
    int i;
    // printf("\x1b[0;37m\x1b[%d;%dH+", y, x);
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


void draw_panel(Panel *p, int x_offset) {
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
        printf("\x1b[%d;%dH", i + 2, x_offset + 1);

        if (f_idx < p->num_files) {
            if (p->active && f_idx == p->current_idx) {
                printf("\x1b[7m %-12s %8uK \x1b[27m", p->files[f_idx].name, p->files[f_idx].size_kb);
                printf("\x1b[m");
            } else {
                printf(" %-12s %8uK ", p->files[f_idx].name, p->files[f_idx].size_kb);
            }
        } else {
            printf("                          "); 
        }
    }
}


void draw_file_line(Panel *p, int x_offset, int file_idx) {
    int screen_row = (file_idx - p->scroll_offset) + 2;
    char selector;

    // Determinar el carácter de selección: '*' si está marcado, espacio si no
    selector = p->files[file_idx].seleccionado ? '*' : ' ';

    if (file_idx >= p->scroll_offset && file_idx < p->scroll_offset + VISIBLE_ROWS) {
        // Posicionar cursor en la fila y columna correspondiente
        printf("\x1b[%d;%dH", screen_row, x_offset + 1);

        if (p->active && file_idx == p->current_idx) {
            /* CURSOR ACTIVO: 
               Usamos video inverso (\x1b[7m). El asterisco ayuda a identificar 
               si el archivo bajo el cursor también está marcado para copia múltiple.
            */
            printf("\x1b[7m%c%-12s %8uK \x1b[27m", 
                   selector, 
                   p->files[file_idx].name, 
                   p->files[file_idx].size_kb);
        } else {
            /* LÍNEA NORMAL O SELECCIONADA SIN FOCO:
               Si está seleccionado, podrías usar \x1b[1m (Negrita) para resaltarlo más,
               pero mantendremos el esquema de colores azul/blanco para no perder la estética.
            */
            printf("\x1b[m%c%-12s %8uK ",
                   selector, 
                   p->files[file_idx].name, 
                   p->files[file_idx].size_kb);
        }
    }
}
