#ifndef ZMC_H
#define ZMC_H

#define MAX_FILES 80        // Aumentamos el buffer de archivos
#define FILENAME_LEN 13
#define SCREEN_HEIGHT 32    
#define PANEL_HEIGHT 30     // 30 renglones de marco (28 de archivos reales)
#define PANEL_WIDTH 40
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
#endif
