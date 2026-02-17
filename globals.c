#include <stdio.h>
#include <stdint.h>


uint8_t CONFIG[] = { // 80x40
    80,  // Columns
    32   // Lines
};

uint8_t DEBUG = 0;
uint8_t DEVEL = 0;

const uint8_t *COLUMNS = CONFIG;
const uint8_t *LINES = CONFIG+1;

uint16_t MAX_FILES = 0;

void set_invers() {
    printf("\x1b[7m");
}


void set_normal() {
    printf("\x1b[0m");
}


void clrscr() {
    printf("\x1b[2J\x1b[?25l");
}

void print_cpm_attrib( uint8_t *ca) {
    // show file attributes
    printf( "%c%c%c",
            *ca++ > 0x7F ? 'R' : ' ', // READ ONLY
            *ca++ > 0x7F ? 'S' : ' ', // SYSTEM
            *ca++ > 0x7F ? 'B' : ' '  // file was BACKED UP
    );
}
