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

#define MAX_FILES 255        // Aumentamos el buffer de archivos
#define FILENAME_LEN 13
#define SCREEN_HEIGHT (*LINES) // 32
#define PANEL_WIDTH (*COLUMNS/2) //40
#define PANEL_HEIGHT (SCREEN_HEIGHT - 2) // 30  // Ajustable según la terminal
#define VISIBLE_ROWS (PANEL_HEIGHT - 2)

#define ESC 0x1B

#define INVERS do {printf("\x1b[7m");} while(0)
#define NORMAL do {printf("\x1b[0m");} while(0)
#define CLRSCR do {printf("\x1b[2J\x1b[?25l");} while(0)

extern uint8_t *CONFIG;

extern uint8_t *LINES;
extern uint8_t *COLUMNS;

extern uint8_t DEBUG;


typedef struct { // CP/M Plus date time format
    uint16_t date;
    uint8_t hour;
    uint8_t minute;
} datetime;

typedef struct { // directory date/time entry
    datetime crea_acc; // create or access date/time
    datetime update;   // update date/time
    uint8_t pw_mode;   // not used
    uint8_t reserved;  // fill to 10 byte
} date_time_info;

typedef struct { // CP/M Plus directory info for 3 files
    uint8_t type; // '!'
    date_time_info dt[3]; // 3x10 byte info
    uint8_t dummy; // fill to 32 byte
} date_time_dir;


typedef struct {
    char name[FILENAME_LEN];
    uint16_t size_kb;
    uint8_t seleccionado;  // 0 = No, 1 = Sí (NUEVO)
    uint16_t date; // days since 31.12.1977 or year
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
} FileEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    uint16_t num_files;
    uint16_t current_idx;
    uint16_t scroll_offset;
    char drive;
    uint8_t active;
} Panel;


typedef struct {
    Panel left;
    Panel right;
    Panel *active_panel;
} AppState;


void draw_panel(Panel *p, uint8_t x_offset);
void load_directory(Panel *p);
uint8_t wait_key_hw(void);
int borrar_archivo(Panel *p);
int copiar_archivo(Panel *src, Panel *dst);
void draw_file_line(Panel *p, uint8_t x_offset, uint16_t file_idx);
void view_file(Panel *p);
void dump_file(Panel *p);
int copiar_archivo_por_indice(Panel *src, Panel *dst, uint16_t idx);
void ejecutar_copia_multiple(Panel *src, Panel *dst);
void ejecutar_borrado_multiple(Panel *p);

#endif
