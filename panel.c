#include <stdio.h>
#include <string.h>
#include "zmc.h"

void draw_frame(int x, int y, int w, int h, char *title) {
    int i;
    printf("\x1b[44;37m\x1b[%d;%dH+", y, x);
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
    int visible_rows = 28; // PANEL_HEIGHT (30) - 2 bordes

    /* Lógica de Scroll para 28 filas visibles */
    if (p->current_idx < p->scroll_offset) {
        p->scroll_offset = p->current_idx;
    }
    if (p->current_idx >= p->scroll_offset + visible_rows) { 
        p->scroll_offset = p->current_idx - (visible_rows - 1);
    }

    sprintf(title, " Unidad %c: ", p->drive);
    draw_frame(x_offset, 1, PANEL_WIDTH, PANEL_HEIGHT, title);
    
    for (i = 0; i < visible_rows; i++) {
        int f_idx = i + p->scroll_offset;
        printf("\x1b[%d;%dH", i + 2, x_offset + 1);
        
        if (f_idx < p->num_files) {
            if (p->active && f_idx == p->current_idx) {
                printf("\x1b[7m %-12s %8uK \x1b[27m", p->files[f_idx].name, p->files[f_idx].size_kb);
                printf("\x1b[44;37m"); 
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

    if (file_idx >= p->scroll_offset && file_idx < p->scroll_offset + 28) {
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
            printf("\x1b[44;37m%c%-12s %8uK ", 
                   selector, 
                   p->files[file_idx].name, 
                   p->files[file_idx].size_kb);
        }
    }
}
