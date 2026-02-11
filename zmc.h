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
#ifndef ZMC_H
#define ZMC_H

#include <stdint.h>

#define MAX_FILES 200        // Aumentamos el buffer de archivos
#define FILENAME_LEN 13
#define SCREEN_HEIGHT (*LINES) // 32
#define PANEL_WIDTH (*COLUMNS/2) //40
#define PANEL_HEIGHT (SCREEN_HEIGHT - 2) // 30  // Ajustable según la terminal
#define VISIBLE_ROWS (PANEL_HEIGHT - 2)

/* Macros para colores VT100 */
//#define CLR_PANEL "\x1b[0m"  // Azul/Blanco
//#define CLR_RESET "\x1b[0m"

#define ESC 0x1B

typedef struct {
    char name[FILENAME_LEN];
    unsigned int size_kb;
    int seleccionado;  // 0 = No, 1 = Sí (NUEVO)
} FileEntry;


typedef struct {
    FileEntry files[MAX_FILES];
    int num_files;
    int current_idx;
    int scroll_offset;
    char drive;
    int active;
} Panel;


typedef struct {
    Panel left;
    Panel right;
    Panel *active_panel;
} AppState;


void draw_panel(Panel *p, int x_offset);
void load_directory(Panel *p);
unsigned char wait_key_hw(void);
int borrar_archivo(Panel *p);
int copiar_archivo(Panel *src, Panel *dst);
void draw_file_line(Panel *p, int x_offset, int file_idx);
void view_file(Panel *p);
void dump_file(Panel *p);
int copiar_archivo_por_indice(Panel *src, Panel *dst, int idx);
void ejecutar_copia_multiple(Panel *src, Panel *dst);
void ejecutar_borrado_multiple(Panel *p);
#endif
