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
#include "zmc.h"

AppState App;
/*
unsigned char wait_key_hw(void) {
    #asm
    loophw_zmc:
        in a, (025h)
        and 01h
        jr z, loophw_zmc
        in a, (020h)
        ld l, a
        ld h, 0
    #endasm
}
*/


unsigned char wait_key_hw() {
#asm
loop_bdos_kbd:
    ld c, 06h    ; Función 6: Direct I/O
    ld e, 0FFh   ; Modo entrada
    call 0005h   ; Llamada al BDOS
    or a         ; ¿Hay tecla?
    jr z, loop_bdos_kbd
    ld l, a      ; Retornar tecla en HL
    ld h, 0
#endasm
}

void refresh_ui() {
    draw_panel(&App.left, 1);
    draw_panel(&App.right, 41);

    printf("\x1b[31;1H\x1b[0m%c> ", App.active_panel->drive);
    printf("\x1b[32;1H\x1b[47;30m F1:Help | F3:View | F4:Dump | U:Drive | F5:Copy | F8:Del | TAB:Sw | ^X:Exit \x1b[0m");
}

int main() {
    App.left.drive = 'A'; App.left.active = 1;
    App.right.drive = 'B'; App.right.active = 0;
    App.active_panel = &App.left;

    load_directory(&App.left);
    load_directory(&App.right);
    printf("\x1b[?25l"); // Ocultar cursor físico de la terminal
    printf("\x1b[2J\x1b[H");
    refresh_ui();

    while(1) {
        unsigned char k = wait_key_hw();

        // 1. TAB: Cambio de panel
        /* REEMPLAZAR el bloque del TAB (0x09) en main.c */
	if (k == 0x09) {
	    int old_left_idx = App.left.current_idx;
	    int old_right_idx = App.right.current_idx;

	    // Cambiar el foco
	    App.left.active = !App.left.active;
	    App.right.active = !App.right.active;
	    App.active_panel = App.left.active ? &App.left : &App.right;

	    // Dibujo quirúrgico: refrescar solo las líneas donde están los cursores
	    draw_file_line(&App.left, 1, old_left_idx);
	    draw_file_line(&App.right, 41, old_right_idx);
	    
	    // Actualizar solo el prompt inferior
	    printf("\x1b[31;1H\x1b[0m%c> ", App.active_panel->drive);
	    continue;
	}

        // 2. U: Cambio de Unidad
        if (k == 'u' || k == 'U') {
            printf("\x1b[31;1H\x1b[45;37m Seleccione Unidad (A-Z): \x1b[0m");
            unsigned char drive_char = wait_key_hw();
            if (drive_char >= 'a' && drive_char <= 'z') drive_char -= 32;
            if (drive_char >= 'A' && drive_char <= 'Z') {
                App.active_panel->drive = drive_char;
                load_directory(App.active_panel);
            }
            refresh_ui();
            continue;
        }

        // 3. Ctrl+X: Salir
        if (k == 24) break;
	/* MODIFICACIÓN PARCIAL en el while(1) de main.c */
	// 1. Selección con Espacio (Cierre de ciclo de refresco)
        if (k == ' ') { 
            int idx = App.active_panel->current_idx;
            int offset = (App.active_panel == &App.left) ? 1 : 41;
            
            // A. Invertir el estado de selección en la memoria
            App.active_panel->files[idx].seleccionado = !App.active_panel->files[idx].seleccionado;
            
            // B. REDIBUJAR el renglón actual inmediatamente para mostrar el '*' [cite: 2026-01-31]
            // IMPORTANTE: Todavía no hemos movido el current_idx, así que se dibuja con el cursor.
            draw_file_line(App.active_panel, offset, idx);

            // C. AVANZAR el cursor al siguiente renglón [cite: 2026-01-31]
            if (App.active_panel->current_idx < App.active_panel->num_files - 1) {
                int old_idx = App.active_panel->current_idx;
                App.active_panel->current_idx++;
                
                // D. REDIBUJAR el renglón anterior (para que pierda el cursor pero mantenga el '*') [cite: 2026-01-31]
                draw_file_line(App.active_panel, offset, old_idx);
                
                // E. REDIBUJAR el nuevo renglón (para que gane el cursor) [cite: 2026-01-31]
                draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
            }
            continue;
        }

        // 4. Secuencias de Escape (Flechas y Teclas de Función)
        if (k == 27) {
            k = wait_key_hw();
            if (k == '[') {
                k = wait_key_hw();
                switch(k) {
                    /* REEMPLAZAR los casos 'A' y 'B' dentro del switch(k) en main.c */
		    case 'A': // Arriba
		        if (App.active_panel->current_idx > 0) {
		            int old_idx = App.active_panel->current_idx;
		            App.active_panel->current_idx--;
		            
		            // Si hay scroll, redibujamos todo; si no, solo las dos líneas
		            if (App.active_panel->current_idx < App.active_panel->scroll_offset) {
		                refresh_ui();
		            } else {
		                int offset = (App.active_panel == &App.left) ? 1 : 41;
		                draw_file_line(App.active_panel, offset, old_idx);
		                draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
		            }
		        }
		        break;
		    case 'B': // Abajo
		        if (App.active_panel->current_idx < App.active_panel->num_files - 1) {
		            int old_idx = App.active_panel->current_idx;
		            App.active_panel->current_idx++;
		            
		            // Si hay scroll, redibujamos todo; si no, solo las dos líneas
		            if (App.active_panel->current_idx >= App.active_panel->scroll_offset + 28) {
		                refresh_ui();
		            } else {
		                int offset = (App.active_panel == &App.left) ? 1 : 41;
		                draw_file_line(App.active_panel, offset, old_idx);
		                draw_file_line(App.active_panel, offset, App.active_panel->current_idx);
		            }
		        }
		        break;
		    case '5': // Page Up (ESC [ 5 ~)
		            wait_key_hw(); // Consumir ~
		            if (App.active_panel->current_idx >= 18)
		                App.active_panel->current_idx -= 18;
		            else
		                App.active_panel->current_idx = 0;
		            refresh_ui();
		            break;
		    case '6': // Page Down (ESC [ 6 ~)
		            wait_key_hw(); // Consumir ~
		            App.active_panel->current_idx += 18;
		            if (App.active_panel->current_idx >= App.active_panel->num_files)
		                App.active_panel->current_idx = App.active_panel->num_files - 1;
		            refresh_ui();
		            break;
                    case '1': // F5 (15~) y F8 (19~)
                        k = wait_key_hw();
                        
                        if (k == '5') { // F5 Detectado
                            wait_key_hw(); // Consumir ~
                            Panel *dest = (App.active_panel == &App.left) ? &App.right : &App.left;
                            
                            // Limpiar línea de diálogo y preguntar
                            printf("\x1b[31;1H\x1b[K COPIAR A %c:? (S/N) ", dest->drive);
                            
                            k = wait_key_hw();
                            if (k == 's' || k == 'S') {
                                // Ahora sí: Copia múltiple habilitada
                                ejecutar_copia_multiple(App.active_panel, dest);
                            }
                            // Limpiar rastro al terminar
                            printf("\x1b[31;1H\x1b[K"); 
                        } 
                        else if (k == '9') { // F8 Detectado
                            wait_key_hw(); // Consumir ~
                            
                            // Limpiar línea de diálogo y preguntar
                            printf("\x1b[31;1H\x1b[K BORRAR SELECCIONADOS? (S/N) ");
                            
                            k = wait_key_hw();
                            if (k == 's' || k == 'S') {
                                // Llamamos a la nueva función maestra [cite: 2026-01-31]
                                ejecutar_borrado_multiple(App.active_panel);
                                load_directory(App.active_panel);
                            }
                            // Limpiar rastro al terminar
                            printf("\x1b[31;1H\x1b[K");
                        }
                        refresh_ui();
                        break;
                }
            } else if (k == 'O') {
	        k = wait_key_hw();
	        if (k == 'P') { // F1: AYUDA
	            printf("\x1b[31;1H\x1b[44;37m AYUDA: ZMC v1.0 - Volney Torres \x1b[0m");
	            wait_key_hw();
	            refresh_ui();
	        }
	        else if (k == 'R') { // F3: VIEW
	            view_file(App.active_panel);
                    printf("\x1b[2J");   // Limpieza total de la pantalla (Borra el texto del TYPE)
	            printf("\x1b[?25l"); // Asegurar que el cursor siga oculto al volver
	            refresh_ui();
	        } 
                /* Dentro de main.c */
		else if (k == 'S') { // F4: DUMP
		    dump_file(App.active_panel);
		    printf("\x1b[2J\x1b[?25l"); // Limpieza y ocultar cursor
		    refresh_ui();
		}
            }
        }
    }
    printf("\x1b[?25h"); // Volver a mostrar el cursor antes de salir
    printf("\x1b[0m\x1b[2J\x1b[H Saliendo de ZMC...\n");
    return 0;
}
