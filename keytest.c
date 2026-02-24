#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cpm.h>

// strip the MS-DOS protection header
#pragma output noprotectmsdos

// do not insert the file redirection option while parsing the command line arguments
#pragma output noredir

#define BS 0x08
#define ESC 0x1B
#define RUB 0x7F


// use raw BIOS CONIO (fkt 3) to ignore XON/XOFF (^Q is used as fkt key)
uint8_t wait_key_hw() {
    uint8_t k = bios( BIOS_CONIN, 0, 0 );
    if ( k == RUB ) // translate RUB to BS
        k = BS;
    return k;
}


// test for terminal function keys, exit with <ESC><ESC>
int main() {
    uint8_t k;
    puts( "function key test - exit with <ESC><ESC>" );
    for(;;) {
        static uint8_t esc = 0;
        k = wait_key_hw();
        printf( "0x%02X  ", k );
        if ( k == ESC )
            puts( "ESC" );
        else if (k < ESC )
            printf( "^%c\n", k + '@');
        else    // show printable chars, else '.'
            printf( "%c\n", k >= ' ' && k < 128 ? k : '.' );
        if ( esc && k == ESC ) // <ESC><ESC>
            return 0;
        esc = k == ESC; // remember <ESC>
    }
    return 0;
}
