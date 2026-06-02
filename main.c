/*
Z80 Management Commander (ZMC)
Copyright (C) 2026 Volney Torres & Martin Homuth-Rosemann

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

// strip the MS-DOS protection header
#pragma output noprotectmsdos

// do not insert the file redirection option while parsing the command line arguments
#pragma output noredir

#include <cpm.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "internal_env.h"
#include "zmc.h"

// the status of both panels
AppState App;

char cmdline[ CMDLINELEN + 1 ];

#define NUMBUF 64

uint8_t DEBUG = 0; // increase with "zmc --debug", can be used to enable messages etc.
uint8_t DEVEL = 0; // increase with "zmc --devel", can be used to enable new features
uint8_t CONFIG = 0;
uint8_t BUFFER = 0;
uint16_t MAX_FILES = 0;

uint8_t COLUMNS = 0;
uint8_t LINES = 0;
uint8_t LINES2 = 0;

uint8_t VLIB_STATUS = 0;

uint8_t *cpbufpt = NULL;
uint8_t cpbufsz = NUMBUF;


int wait_key_bios( void ) {
    // use raw BIOS CONIO (fkt 3) to ignore XON/XOFF (^Q and ^S are used as fkt keys)
    return bios( BIOS_CONIN, 0, 0 ); // function, BC, DE, returns A
}


int wait_key_bdos( void ) {
    // use BDOS RAWIO to ignore XON/XOFF (^Q and ^S are used as fkt keys)
    return bdos( 6, 0xFD ); // C_RAWIO, wait for char, returns A
}


// function pointer, default is BIOS, can be switched to BDOS for CP/M3
int ( *wait_key_hw )( void ) = &wait_key_bios;


void help() {
    uint8_t line = 1;
    curoff();
    stndend();
    cls();
    gotoxy( ( line++ ) << 8 | 1 );
    if ( SCREEN_HEIGHT >= 20 ) {
        puts( "                           " );
        puts( " #######  #     #   #####  " );
        puts( "      #   ##   ##  #     # " );
        puts( "     #    # # # #  #       " );
        puts( "    #     #  #  #  #       " );
        puts( "   #      #     #  #       " );
        puts( "  #       #     #  #     # " );
        puts( " #######  #     #   #####  " );
        puts( "                           " );
        line = 13;
    }

    puts( "= ZMC 1.3-rc =" );
    puts( "Volney Torres - Martin Homuth-Rosemann - Steven Hirsch\n" );
    const uint8_t helppos = 40;
    gotoxy( line << 8 | 1 );
    printf( "A: ... P:" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Select drive" );
    printf( "[TAB]" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Change panel" );
    printf( "[F3],  [ESC]3, TYPE, VIEW, CAT" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Show file" );
    printf( "[F4],  [ESC]4, DUMP, HEX" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Hexdump file" );
    printf( "[F5],  [ESC]5, COPY, CP" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Copy file(s)" );
    printf( "[F8],  [ESC]8, DEL, ERA, RM" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Delete file(s)" );
    printf( "[F10], [ESC]0, [ESC][ESC], QUIT, EXIT" );
    gotoxy( ( line++ ) << 8 | helppos );
    puts( "Exit" );
    wait_key_hw();
    refresh_ui( PAN_BOTH );
}


typedef void ( *command_func_t )( void );

typedef struct {
    const char *keyword;
    const command_func_t func;
} command_t;

// parse zmc cmd line and get a fkt pointer
command_func_t find_command( const char *input, command_t commands[], int num_commands ) {
    for ( int i = 0; i < num_commands; i++ ) {
        if ( strncmp( input, commands[ i ].keyword, strlen( commands[ i ].keyword ) ) == 0 ) {
            return commands[ i ].func;
        }
    }
    return NULL;
}


// list of text commands from prompt line and called function
const command_t commands[] = {
    { "HELP", help },
    { "VIEW", view_file }, { "TYPE", view_file },  { "CAT", view_file },
    { "DUMP", dump_file }, { "HEX", dump_file },
    { "COPY", copy_cmd },  { "CP", copy_cmd },
    { "DEL", delete_cmd }, { "ERA", delete_cmd },  { "RM", delete_cmd },
    { "TOP", first_file }, { "POS1", first_file },
    { "BOT", last_file },  { "END", last_file },
};


int main( int argc, char **argv ) {
    uint8_t cpmversion = bdos( 12, NULL );
    // CP/M Plus has values for screen size in System Control Block
    if ( cpmversion == 0x31 ) { // version == CP/M Plus
        // handle BDOS errors internally, do not exit
        bdos( 45, 0xFF );                       // set BDOS return error mode 1
        bdos( 109, 0x0A );                      // C_MODE, ignore ^C and ^S
        wait_key_hw = &wait_key_bdos;           // use BDOS RAWIO instead of BIOS CONIO
        uint8_t scbpb[ 4 ] = { 0x1A, 0, 0, 0 }; // SCB parameter block, get col - 1
        COLUMNS = bdos( 49, scbpb ) + 1;
        scbpb[ 0 ] = 0x1C; // lines - 1
        LINES = bdos( 49, scbpb ) + 1;
        LINES2 = LINES - 2;
    }

    uint8_t **envptr = (void *)0x109;

    uint16_t total;
    uint16_t largest;
    // total   = address where the total number of free bytes in the heap will be stored
    // largest = address where the size of the largest available block in the heap will be stored
    mallinfo( &total, &largest );

    // calculate number of file entries
    MAX_FILES = largest / sizeof( FileEntry ) / 2 - 1;

    // Set current drive for both panels
    char drive_left = bdos( 25, fcb_src ) + 'A'; // get current drive
    char drive_right = drive_left;

    // cmd line argument "--config" shows address of screen size constants
    // in zmc.com to help the user to patch with a HEX editor, e.g. BE.
    while ( --argc ) {
        ++argv;
        if ( !strcmp( *argv, "--CONFIG" ) ) {
            ++CONFIG;
        } else if ( !strcmp( *argv, "--DEVEL" ) ) {
            ++DEVEL;
        } else if ( !strcmp( *argv, "--DEBUG" ) ) {
            ++DEBUG;
        } else if ( !strcmp( *argv, "--TCAP" ) || !strcmp( *argv, "--ENV" ) ) {
            char *opt = *argv;
            --argc;
            ++argv;
            // TODO: &env is location of our internal Z3 environment. The storage is
            // allocated by linking in internal_env. There really needs to be a
            // function that checks for native Z and allocates the internal buffer
            // dynamically only if required (i.e. no native Z3).
            int fd = open( *argv, O_RDONLY, 0 );
            if ( fd >= 0 ) {
                if ( !strcmp( opt, "--TCAP" ) ) {
                    size_t count = read( fd, (void *)&env + 128, 128 );
                    if ( count != 128 ) {
                        printf( "Error reading TCAP file %s\n", *argv );
                        exit( 1 );
                    }
                } else { // debug function to load a different env & TCAP
                    size_t count = read( fd, (void *)&env, 256 );
                    if ( count != 256 ) {
                        printf( "Error reading environment file %s\n", *argv );
                        exit( 1 );
                    }
                }
                close( fd );
                // Reset DMA to default!
                bdos( CPM_SDMA, DEF_DMA );
                VLIB_STATUS = gz3init( &env ); // init local
            } else {
                printf( "Cannot open TCAP or environment file: %s\n", *argv );
                uint8_t k = wait_key_hw();
                if ( ESC == k || CTRL_C == k )
                    return -1;
            }
        } else if ( !strcmp( *argv, "--BUFFER" ) ) {
            --argc;
            uint16_t arg = atoi( *( ++argv ) );
            if ( arg && arg < 128 )
                BUFFER = arg;
        } else if ( !strcmp( *argv, "--KEY" ) ) {
            // test for terminal function keys, exit with <ESC><ESC>
            uint8_t k;
            printf( "CP/M version %02X: function key test - exit with <ESC><ESC>\n", cpmversion );
            for ( ;; ) {
                static uint8_t esc = 0;
                k = wait_key_hw();
                printf( "0x%02X  ", k );
                if ( k == ESC )
                    puts( "ESC" );
                else if ( k < ESC )
                    printf( "^%c\n", k + '@' );
                else // show printable chars, else '.'
                    printf( "%c\n", k >= ' ' && k < 128 ? k : '.' );
                if ( esc && k == ESC ) // <ESC><ESC>
                    return 0;
                esc = k == ESC; // remember <ESC>
            }
        } else if ( *argv[ 0 ] >= 'A' && *argv[ 0 ] <= 'P' ) {
            if ( argc == 1 )
                drive_right = *argv[ 0 ];
            else
                drive_left = *argv[ 0 ];
        }
    }

    if ( DEBUG )
        --LINES; // debugging output in the last line

    if ( BUFFER )
        cpbufsz = BUFFER;

    cpbufpt = malloc( cpbufsz * 128 ); // reserve and init heap space
    if ( cpbufpt == NULL ) {
        fprintf( stderr, "cpbufpt: not enough memory!\n" );
        return -1;
    }

    mallinfo( &total, &largest );

    MAX_FILES = largest / sizeof( FileEntry ) / 2 - 1;

    FileEntry *f_left;
    FileEntry *f_right;

    f_left = calloc( MAX_FILES, sizeof( FileEntry ) ); // reserve and init heap space
    if ( f_left == NULL ) {
        fprintf( stderr, "f_left: not enough memory!\n" );
        return -1;
    }
    f_right = calloc( MAX_FILES, sizeof( FileEntry ) ); // reserve and init heap space
    if ( f_right == NULL ) {
        fprintf( stderr, "f_right: not enough memory!\n" );
        return -1;
    }

    if ( DEBUG > 1 ) {
        mallinfo( &total, &largest );
        printf( "total: %u, largest: %u, cpbufsz: %u, MAX_FILES: %u\n", total, largest, cpbufsz, MAX_FILES );
        return 0;
    }


    if ( COLUMNS ) { // got value from CP/M3 SCB
        if ( *envptr ) // update global ENV
            *((uint8_t*)( *envptr + ( &INTCOLUMNS - &INTENV ) ) ) = COLUMNS;
    } else {
        if ( *envptr ) // use global value
            COLUMNS = *((uint8_t*)( *envptr + ( &INTCOLUMNS - &INTENV ) ) );
        else
            COLUMNS = INTCOLUMNS; // fallback to internal value
    }
    if ( LINES ) {
        if ( *envptr )
            *((uint8_t*)( *envptr + ( &INTLINES - &INTENV ) ) ) = LINES;
    } else {
        if ( *envptr )
            LINES = *((uint8_t*)( *envptr + ( &INTLINES - &INTENV ) ) );
        else
            LINES = INTLINES;
    }

    if ( CONFIG ) {
        uint8_t *pcol;
        uint8_t *plin;
        printf( "CP/M version: %02X\n", cpmversion );
        if ( *envptr ) {
            printf( "Z3 environment @ 0x%04X\n", *envptr );
            pcol = (uint8_t*)( *envptr + ( &INTCOLUMNS - &INTENV ) );
            plin = (uint8_t*)( *envptr + ( &INTLINES - &INTENV ) );
            printf( "COLUMNS @ 0x%04X: %d\n", pcol, *pcol );
            printf( "LINES @ 0x%04X: %d\n", plin, *plin );
        } else {
            uint8_t *pintenv = &INTENV;
            pcol = &INTCOLUMNS;
            plin = &INTLINES;
            printf( "Using internal environment at file offset %04X\n", pintenv - 0x100 );
            printf( "COLUMNS @ 0x%04X: %d\n", pcol - 0x100, *pcol );
            printf( "LINES @ 0x%04X: %d\n", plin - 0x100, *plin );
        }
        printf( "COPY BUFFER: %u\n", cpbufsz );
        printf( "MAX_FILES: %u\n", MAX_FILES );
        return 0;
    }

    App.left.files = f_left;
    App.right.files = f_right;
    App.left.drive = drive_left;
    App.right.drive = drive_right;

    App.active_panel = &App.left;
    App.inactive_panel = &App.right;

    bdos( CPM_SDMA, DEF_DMA ); // set default DMA

    if ( !*envptr ) {      // no global env
        VLIB_STATUS = gz3init( &env ); // init local
    } else {
        VLIB_STATUS = gz3init( *envptr );
    }

    cls();
    curoff();

    draw_frame( &App.left );
    draw_header( &App.left );
    load_directory( &App.left );
    fill_panel( &App.left );

    draw_frame( &App.right );
    draw_header( &App.right );
    load_directory( &App.right );
    fill_panel( &App.right );

    draw_footer();
    show_prompt();

    uint8_t loop = 1;
    uint8_t k;

    char *cp = cmdline;
    *cp = '\0';

    int num_commands = sizeof( commands ) / sizeof( commands[ 0 ] );

    while ( loop ) { // terminal key input loop
        k = wait_key_hw();
        curoff();
        // printable char go to the prompt line, BS/RUB deletes, CR executes
        if ( ( k > SPC && k < RUB ) || ( k == SPC && *cmdline ) ) {
            if ( cp < cmdline + CMDLINELEN ) {
                *cp++ = toupper( k );
                *cp = '\0';
            }
        } else if ( k == BS || k == RUB ) {
            if ( cp > cmdline )
                *--cp = '\0';
        } else if ( k == CR ) { // cmd line parser
            // "A:" .. "P:"
            if ( cmdline[ 1 ] == ':' && *cmdline >= 'A' && *cmdline <= 'P' ) {
                change_drive( *cmdline );
            } else if ( !strncmp( cmdline, "EXIT", 4 ) || !strncmp( cmdline, "QUIT", 4 ) ) {
                loop = 0;
            } else if ( *cmdline ) { // scan cmd list and get function
                command_func_t function = find_command( cmdline, commands, num_commands );
                if ( function ) // if found ..
                    function(); // .. execute it
            }
            // clear cmd string
            cp = cmdline;
            *cp = '\0';
        } else if ( k == TAB ) { // TAB: OTHER_PANEL
            change_focus();
        }
        // here come the function keys, first the WS bindings
        else if ( k == ' ' || k == 'V' - '@' ) { // ' ' or ^V -> SELECT
            select_file();
        } else if ( k == 'E' - '@' ) { // ^E
            line_up();
        } else if ( k == 'X' - '@' ) { // ^X
            line_down();
        } else if ( k == 'R' - '@' ) { // ^R
            page_up();
        } else if ( k == 'C' - '@' ) { // ^C
            page_down();
        } else if ( k == 'Q' - '@' ) { // ^Q
            k = wait_key_hw();
            if ( k == 'S' - '@' ) // ^Q^S
                first_file();
            else if ( k == 'D' - '@' ) // ^Q^D
                last_file();
        }
        // now the multi character function keys starting with <ESC>
        else if ( k == ESC ) { // ESC sequences
            k = wait_key_hw();
            if ( k == ESC ) { // <ESC><ESC>
                loop = 0;     // quit program
            }
            // now comes the mc style coding ESC1...ESC0 as proposed by SvenMb
            else if ( k == '1' ) { // <ESC>1
                help();
            } else if ( k == '3' ) { // <ESC>3
                view_file();
            } else if ( k == '4' ) { // <ESC>4
                dump_file();
            } else if ( k == '5' ) { // <ESC>5
                copy_cmd();
            } else if ( k == '8' ) { // <ESC>8
                delete_cmd();
            } else if ( k == '0' ) { // <ESC>0 (ZERO)
                loop = 0;
            } else {
                loop = parse_function_keys( k ); // VT100 function keys
            }
        }
        if ( loop )
            show_prompt();
    } // while ( loop )

    cls();
    gotoxy( 1 << 8 | 1 );
    curon();
    stndend();
    return 0;
}
