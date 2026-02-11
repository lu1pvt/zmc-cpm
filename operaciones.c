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
#include <cpm.h>
#include "zmc.h"

extern uint8_t *LINES;
extern uint8_t *COLUMNS;

/* Estructura de un registro de directorio de CP/M (32 bytes) */
struct cpm_dir {
    unsigned char drive;
    char name[8];
    char ext[3];
    unsigned char extent;
    unsigned char s1, s2;
    unsigned char rc;
    unsigned char map[16];
};


void load_directory(Panel *p) {
    struct cpm_dir *dir_entry;
    unsigned char fcb[36];
    int count = 0;
    int result;

    p->num_files = 0;
    p->current_idx = 0;
    p->scroll_offset = 0;

    if (p->drive == '@') // select current drive
        p->drive = bdos( 25, fcb ) + 'A';
    /* 1. Cambiar a la unidad deseada antes de buscar */
    bdos(14, p->drive - 'A'); 

    /* 2. Preparar FCB para buscar todos los archivos (*.*) */
    memset(fcb, 0, sizeof(fcb));
    fcb[0] = 0; // Unidad actual
    memset(&fcb[1], '?', 11); // Comodines para nombre y extensión

    /* 3. Buscar el primer archivo */
    result = bdos(17, fcb);

    while (result != 255 && count < MAX_FILES) {
        /* La entrada encontrada está en el buffer de DMA (por defecto 0x80) */
        /* result nos da el índice (0-3) dentro del sector de 128 bytes */
        dir_entry = (struct cpm_dir *)(0x80 + (result * 32));

        /* Solo procesar si no es un archivo borrado (0xE5) */
        if (dir_entry->drive != 0xE5) {
            char clean_name[9], clean_ext[4];

            // Limpiar nombre (quitar bits de atributos y espacios)
            for(int i=0; i<8; i++) clean_name[i] = dir_entry->name[i] & 0x7F;
            clean_name[8] = '\0';
            for(int i=0; i<3; i++) clean_ext[i] = dir_entry->ext[i] & 0x7F;
            clean_ext[3] = '\0';

            // Formatear para el Panel (Ej: COMMAND.COM)
            sprintf(p->files[count].name, "%s.%s", strtok(clean_name, " "), clean_ext);

            // Tamaño aproximado (RC * 128 bytes / 1024)
            p->files[count].size_kb = (dir_entry->rc * 128) / 1024;
            if (p->files[count].size_kb == 0 && dir_entry->rc > 0) p->files[count].size_kb = 1;

            count++;
        }

        /* Buscar siguiente */
        result = bdos(18, fcb);
    }
    p->num_files = count;
}


/* IMPLEMENTACIÓN COMPLETA: Función para borrar el archivo seleccionado */
/* Asegúrate de incluir string.h para el memset */
#include <string.h>

int borrar_archivo(Panel *p) {
    unsigned char fcb[36];
    char *name_ptr;
    int i;

    if (p->num_files == 0) return -1;

    memset(fcb, 0, sizeof(fcb));
    fcb[0] = (p->drive - 'A') + 1; // 1=A, 2=B...

    /* Copiar nombre (8 bytes) y extensión (3 bytes) al FCB */
    memset(&fcb[1], ' ', 11);
    name_ptr = p->files[p->current_idx].name;

    // Lógica para separar nombre de extensión (Ej: "DUMP.COM")
    for(i = 0; i < 8 && name_ptr[i] != '.' && name_ptr[i] != '\0'; i++) {
        fcb[1+i] = name_ptr[i];
    }
    while(*name_ptr && *name_ptr != '.') name_ptr++;
    if(*name_ptr == '.') {
        name_ptr++;
        for(i = 0; i < 3 && name_ptr[i] != '\0'; i++) {
            fcb[9+i] = name_ptr[i];
        }
    }

    return bdos(19, fcb); // Función 19: Borrar archivo
}


/* NUEVA RUTINA: Copiar el archivo seleccionado al panel opuesto */
int copiar_archivo(Panel *src, Panel *dst) {
    unsigned char fcb_src[36], fcb_dst[36];
    char *name_ptr;
    int i, result;

    if (src->num_files == 0) return -1;

    /* 1. Preparar FCBs (Origen y Destino) */
    memset(fcb_src, 0, 36);
    memset(fcb_dst, 0, 36);
    fcb_src[0] = (src->drive - 'A') + 1;
    fcb_dst[0] = (dst->drive - 'A') + 1;

    name_ptr = src->files[src->current_idx].name;
    memset(&fcb_src[1], ' ', 11);
    memset(&fcb_dst[1], ' ', 11);

    // Mapear nombre y extensión a ambos FCB
    for(i = 0; i < 8 && name_ptr[i] != '.' && name_ptr[i] != '\0'; i++) {
        fcb_src[1+i] = fcb_dst[1+i] = name_ptr[i];
    }
    while(*name_ptr && *name_ptr != '.') name_ptr++;
    if(*name_ptr == '.') {
        name_ptr++;
        for(i = 0; i < 3 && name_ptr[i] != '\0'; i++) {
            fcb_src[9+i] = fcb_dst[9+i] = name_ptr[i];
        }
    }

    /* 2. Operación de archivos vía BDOS */
    bdos(19, fcb_dst);              // Borrar destino si ya existe
    if (bdos(15, fcb_src) == 255) return -2; // Abrir origen (error si no abre)
    if (bdos(22, fcb_dst) == 255) return -3; // Crear destino

    /* 3. Bucle de transferencia (Sector a Sector) */
    while (bdos(20, fcb_src) == 0) { // Leer registro de 128 bytes de SRC
        // El dato ya está en 0x80 (DMA), lo escribimos en DST
        if (bdos(21, fcb_dst) != 0) break; // Escribir registro en DST
    }

    bdos(16, fcb_dst); // CERRAR archivo (Fundamental para salvar cambios)
    return 0;
}


void show_header() {
    printf("\x1b[2J\x1b[H\x1b[?25l"); // erase, home, hide cursor
}


void show_footer( const char *action, const char *file_name ) {
    printf("\x1b[7m %s: %s (<SPACE>: more | <ESC>: exit) \x1b[0m", action, file_name); // inv / normal
}


/* FUNCIÓN VIEW CORREGIDA: Con paginación, salida por ESC y llaves balanceadas */
void view_file(Panel *p) {
    unsigned char fcb[36];
    int i;
    int line_count = -1;
    char *name_ptr = p->files[p->current_idx].name;
    char *temp_ptr;

    if (p->num_files == 0) return;

    /* 1. Limpiar pantalla para el visor */
    show_header();

    /* 2. Preparar FCB */
    memset(fcb, 0, 36);
    fcb[0] = (p->drive - 'A') + 1;
    memset(&fcb[1], ' ', 11);

    temp_ptr = name_ptr;
    for(i=0; i<8 && temp_ptr[i] != '.' && temp_ptr[i] != '\0'; i++) fcb[1+i] = temp_ptr[i];
    while(*temp_ptr && *temp_ptr != '.') temp_ptr++;
    if(*temp_ptr == '.') {
        temp_ptr++;
        for(i=0; i<3 && temp_ptr[i] != '\0'; i++) fcb[9+i] = temp_ptr[i];
    }

    /* 3. Abrir y leer */
    if (bdos(15, fcb) != 255) { // open
        while (bdos(20, fcb) == 0) { // Leer registro de 128 bytes
            for (i = 0; i < 128; i++) {
                char c = *((char *)(0x80 + i));
                if (c == 0x1A) goto end_of_file; // EOF (Ctrl+Z)

                putchar(c);

                if (c == '\n') {
                    putchar('\r'); // Retorno de carro para CP/M
                    line_count++;

                    // Pausa cuando se llena la pantalla (aprox VISIBLE_ROWS líneas)
                    if (line_count >= PANEL_HEIGHT) {
                        show_footer( "VIEW", name_ptr );
                        unsigned char k = wait_key_hw();
                        printf("\r\x1b[K"); // CR, era EOL
                        if (k == 27) return; // Salida por ESC
                        line_count = 0;
                    }
                }
            }
        }
    } else {
        printf("\r\nError opening file.");
    }
end_of_file:
    printf("\r\n\x1b[7m --- End Of File --- \x1b[0m"); // inv / normal
    wait_key_hw();
}


/* FUNCIÓN DUMP: Volcado Hexadecimal y ASCII (16 bytes por línea) */
void dump_file(Panel *p) {
    unsigned char fcb[36];
    int i, j, line_count = -1;
    long address = 0;
    char *name_ptr = p->files[p->current_idx].name;

    if (p->num_files == 0) return;

    /* 1. Limpiar pantalla para el visor */
    show_header();

    /* Preparar FCB */
    memset(fcb, 0, 36);
    fcb[0] = (p->drive - 'A') + 1;
    memset(&fcb[1], ' ', 11);
    // (Lógica de mapeo de nombre idéntica a view_file...)
    char *temp = name_ptr;
    for(i=0; i<8 && temp[i] != '.' && temp[i] != '\0'; i++) fcb[1+i] = temp[i];
    while(*temp && *temp != '.') temp++;
    if(*temp == '.') {
        temp++;
        for(i=0; i<3 && temp[i] != '\0'; i++) fcb[9+i] = temp[i];
    }

    if (bdos(15, fcb) != 255) {
        while (bdos(20, fcb) == 0) { // Leer 128 bytes
            for (i = 0; i < 128; i += 16) {
                // 1. Mostrar dirección
                printf("%04X  ", (unsigned int)address);

                // 2. Bloque Hexadecimal
                for (j = 0; j < 16; j++) {
                    printf("%02X ", *((unsigned char *)(0x80 + i + j)));
                }
                printf(" |");

                // 3. Bloque ASCII
                for (j = 0; j < 16; j++) {
                    unsigned char c = *((unsigned char *)(0x80 + i + j));
                    if (c >= 32 && c <= 126) putchar(c);
                    else putchar('.');
                }
                printf("|\r\n");

                address += 16;
                line_count++;

                if (line_count >= PANEL_HEIGHT) {
                    show_footer( "DUMP", name_ptr );
                    if (wait_key_hw() == 27) return;
                    printf("\r                       \r");
                    line_count = 0;
                }
            }
        }
    } else {
        printf("\r\nError opening file.");
    }
    printf("\r\n\x1b[7m --- End Of File --- \x1b[0m"); // inv / normal
    wait_key_hw();
}


/* Rutina base: Copia un archivo específico por su índice */
/* 1. Función base de copia (Asegúrate de que el nombre sea este) */
int copiar_archivo_por_indice(Panel *src, Panel *dst, int idx) {
    unsigned char fcb_src[36], fcb_dst[36];
    char *name_ptr;
    int i;

    memset(fcb_src, 0, 36);
    memset(fcb_dst, 0, 36);
    fcb_src[0] = (src->drive - 'A') + 1;
    fcb_dst[0] = (dst->drive - 'A') + 1;

    name_ptr = src->files[idx].name;
    memset(&fcb_src[1], ' ', 11);
    memset(&fcb_dst[1], ' ', 11);

    char *temp = name_ptr;
    for(i = 0; i < 8 && temp[i] != '.' && temp[i] != '\0'; i++) 
        fcb_src[1+i] = fcb_dst[1+i] = temp[i];
    while(*temp && *temp != '.') temp++;
    if(*temp == '.') {
        temp++;
        for(i = 0; i < 3 && temp[i] != '\0'; i++) 
            fcb_src[9+i] = fcb_dst[9+i] = temp[i];
    }

    bdos(19, fcb_dst); 
    if (bdos(15, fcb_src) == 255) return -1; 
    if (bdos(22, fcb_dst) == 255) return -1; 

    while (bdos(20, fcb_src) == 0) {
        if (bdos(21, fcb_dst) != 0) break;
    }

    bdos(16, fcb_dst);
    return 0;
}


/* 2. Función Maestra: Procesa la selección múltiple */
void ejecutar_copia_multiple(Panel *src, Panel *dst) {
    int i, marcados = 0, procesados = 0;

    for (i = 0; i < src->num_files; i++) {
        if (src->files[i].seleccionado) marcados++;
    }

    if (marcados == 0) {
        printf("\x1b[%d;1H\x1b[7m Copying: %s... \x1b[0m",
               SCREEN_HEIGHT-1, src->files[src->current_idx].name);
        copiar_archivo_por_indice(src, dst, src->current_idx);
    } else {
        for (i = 0; i < src->num_files; i++) {
            if (src->files[i].seleccionado) {
                procesados++;
                printf("\x1b[%d;1H\x1b[7m [%d/%d] Copying: %s \x1b[0m",
                       SCREEN_HEIGHT-1, procesados, marcados, src->files[i].name);

                // USAR EL NOMBRE CORRECTO AQUÍ:
                copiar_archivo_por_indice(src, dst, i);
                src->files[i].seleccionado = 0; 
            }
        }
    }
    load_directory(dst);
    // Quitamos refresh_ui() de aquí porque operaciones.c no la conoce.
    // El refresh lo hará el main.c después de llamar a esta función.
}


/* Función Maestra de Borrado en operaciones.c */
void ejecutar_borrado_multiple(Panel *p) {
    int i, marcados = 0, procesados = 0;
    unsigned char fcb[36];

    // Contar cuántos hay marcados
    for (i = 0; i < p->num_files; i++) {
        if (p->files[i].seleccionado) marcados++;
    }

    if (marcados == 0) {
        // Si no hay marcados, borramos solo el actual (funcionalidad original) [cite: 2026-01-28]
        printf("\x1b[%d;1H\x1b[K Deleting: %s... ", SCREEN_HEIGHT-1, p->files[p->current_idx].name);
        borrar_archivo(p);
    } else {
        // Borrado en lote [cite: 2026-01-31]
        for (i = 0; i < p->num_files; i++) {
            if (p->files[i].seleccionado) {
                procesados++;
                printf("\x1b[%d;1H\x1b[K [%d/%d] Deleting: %s ", // pos, erase to EOL
                       SCREEN_HEIGHT-1,  procesados, marcados, p->files[i].name);

                /* Preparamos el FCB para el archivo i */
                memset(fcb, 0, sizeof(fcb));
                fcb[0] = (p->drive - 'A') + 1;
                memset(&fcb[1], ' ', 11);

                char *name_ptr = p->files[i].name;
                int j;
                for(j = 0; j < 8 && name_ptr[j] != '.' && name_ptr[j] != '\0'; j++) fcb[1+j] = name_ptr[j];
                while(*name_ptr && *name_ptr != '.') name_ptr++;
                if(*name_ptr == '.') {
                    name_ptr++;
                    for(j = 0; j < 3 && name_ptr[j] != '\0'; j++) fcb[9+j] = name_ptr[j];
                }

                bdos(19, fcb); // Función 19: Borrar
                p->files[i].seleccionado = 0; 
            }
        }
    }
    // Limpiamos rastro del diálogo
    printf("\x1b[%d;1H\x1b[K", SCREEN_HEIGHT-1 ); // pos, erase EOL
}
