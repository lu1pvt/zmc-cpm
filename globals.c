#include <stdio.h>
#include <stdint.h>


uint8_t CONFIG[] = { // 80x40
    80,  // Columns
    32   // Lines
};

uint8_t DEBUG = 0;

const uint8_t *COLUMNS = CONFIG;
const uint8_t *LINES = CONFIG+1;


void set_invers() {
    printf("\x1b[7m");
}


void set_normal() {
    printf("\x1b[0m");
}


void clrscr() {
    printf("\x1b[2J\x1b[?25l");
}


void print_cpm_name (uint8_t *cpmname) {
    uint8_t *cn = cpmname;
    int8_t j = 0;
    for ( uint8_t i=0; i<8; ++i, ++cn )
        if (*cn != ' ' ) {
            putchar( *cn & 0x7F );
            ++j;
        }
    if ( *cn != ' ' ) {
        putchar( '.' );
        ++j;
    }
    for ( uint8_t i=0; i<3; ++i, ++cn )
        if (*cn != ' ' ) {
            putchar( *cn & 0x7F );
            ++j;
        }
    while ( ++j <= 12 )
        putchar( ' ' );
    // show file attributes
    cn = cpmname+8;
    printf( "  %c%c%c  ",
            *cn++ > 0x7F ? 'R' : ' ', // READ ONLY
            *cn++ > 0x7F ? 'S' : ' ', // SYSTEM
            *cn++ > 0x7F ? 'B' : ' '  // file was BACKED UP
    );
}
