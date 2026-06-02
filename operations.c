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
#include <cpm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zmc.h"


uint8_t fcb_src[ 36 ];
uint8_t fcb_dst[ 36 ];


static void prepare_fcb( const char *name, Panel *src, Panel *dst ) {
    // setup one or two FCBs for reading, copying or deleting
    const char *name_ptr = name;
    if ( src ) {
        *fcb_src = ( src->drive - 'A' ) + 1;
        memset( fcb_src + 1, ' ', 11 );
        memset( fcb_src + 12, 0, sizeof( fcb_src ) - 12 );
        for ( uint8_t j = 0; j < 8 && name_ptr[ j ] != '.' && name_ptr[ j ] != '\0'; j++ )
            fcb_src[ 1 + j ] = name_ptr[ j ];
        while ( *name_ptr && *name_ptr != '.' )
            name_ptr++;
        if ( *name_ptr == '.' ) {
            name_ptr++;
            for ( uint8_t j = 0; j < 3 && name_ptr[ j ] != '\0'; j++ )
                fcb_src[ 9 + j ] = name_ptr[ j ];
        }
    }
    if ( dst ) {
        name_ptr = name;
        *fcb_dst = ( dst->drive - 'A' ) + 1;
        memset( fcb_dst + 1, ' ', 11 );
        memset( fcb_dst + 12, 0, sizeof( fcb_dst ) - 12 );
        for ( uint8_t j = 0; j < 8 && name_ptr[ j ] != '.' && name_ptr[ j ] != '\0'; j++ )
            fcb_dst[ 1 + j ] = name_ptr[ j ];
        while ( *name_ptr && *name_ptr != '.' )
            name_ptr++;
        if ( *name_ptr == '.' ) {
            name_ptr++;
            for ( uint8_t j = 0; j < 3 && name_ptr[ j ] != '\0'; j++ )
                fcb_dst[ 9 + j ] = name_ptr[ j ];
        }
    }
}


// Function to check if a year is leap
static uint8_t is_leap_year( int year ) { return ( year % 4 == 0 && year % 100 != 0 ) || ( year % 400 == 0 ); }

// Array of days in month for normal and leap years
static const int days_in_month[ 2 ][ 12 ] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, // normal year
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }  // leap year
};

// convert CP/M "Days since 1.1.1978" to YYMD
// input:  date -> DD from CP/M directory
// return: date -> YYMD
static void days_to_date( void *date ) {
    uint16_t *cpm_date = (uint16_t *)date;
    uint8_t *cpm_month = (uint8_t *)( date + 2 );
    uint8_t *cpm_day = (uint8_t *)( date + 3 );

    // Handle day 0;
    if ( *cpm_date == 0 ) {
        *cpm_day = 0;
        *cpm_month = 0;
    } else {
        // Start from 1978
        *cpm_date -= 1;
        uint16_t year = 1978;

        // Approximate year to reduce iterations
        while ( *cpm_date >= 365 ) {
            uint16_t days_in_year = is_leap_year( year ) ? 366 : 365;
            if ( *cpm_date < days_in_year )
                break;
            *cpm_date -= days_in_year;
            ++year;
        }

        // Find exact month and day
        uint8_t leap = is_leap_year( year ) ? 1 : 0;
        uint8_t month = 0;

        while ( *cpm_date >= days_in_month[ leap ][ month ] ) {
            *cpm_date -= days_in_month[ leap ][ month ];
            ++month;
        }

        *cpm_day = *cpm_date + 1;
        *cpm_month = month + 1;
        *cpm_date = year;
    }
}


// Three-way compare function for the name field of FileEntry used by qsort
static int fileNameExtentCompare( const void *a, const void *b ) {
    // 1. compare cpmname
    // 2. if equal, compare extent and mark files with lower extent as invalid
    int res = strcmp( ( (const FileEntry *)a )->cpmname, ( (const FileEntry *)b )->cpmname );
    if ( res ) // names are different
        return res;
    // only the 1st extent has date/time info
    if ( ( (const FileEntry *)a )->date && !( (const FileEntry *)b )->date )
        memcpy( &( (FileEntry *)b )->date, &( (const FileEntry *)a )->date, 6 );
    else if ( ( (const FileEntry *)b )->date && !( (const FileEntry *)a )->date )
        memcpy( &( (FileEntry *)a )->date, &( (const FileEntry *)b )->date, 6 );
    // now compare the extents
    res = ( (const FileEntry *)a )->extent - ( (const FileEntry *)b )->extent;
    if ( res < 0 )
        *( (FileEntry *)a )->cpmname = '~';
    else
        *( (FileEntry *)b )->cpmname = '~';
    return res;
}


// Three-way compare function for the name field of FileEntry used by qsort
static int fileNameCompare( const void *a, const void *b ) {
    // setting up rules for comparison
    return strcmp( ( (const FileEntry *)a )->cpmname, ( (const FileEntry *)b )->cpmname );
}


void load_directory( Panel *p ) {
    cpm_dir *dir_entry;
    uint16_t count = 0;
    uint8_t result;

    p->num_files = 0;
    p->current_idx = 0;
    p->top_idx = 0;
    p->show_date = 0;
    p->files->cpmname[ 0 ] = '\0';

    if ( DEVEL && p->drive == 'P' ) { // simulate empty drive
        *( p->files[ 0 ].cpmname ) = '\0';
        return;
    }
    // Reset DMA to default!
    bdos( CPM_SDMA, DEF_DMA );

    if ( p->drive == '@' ) // '@' -> select current drive
        p->drive = bdos( CPM_IDRV, fcb_src ) + 'A';

    /* 1. change drive to fetch the complete directory */
    result = bdos( CPM_LGIN, p->drive - 'A' );

    if ( result ) {
        p->drive = '?';
        return;
    }

    /* 2. Prepare FCB to match all files (*.*) and all extents */
    memset( fcb_src, 0, sizeof( fcb_src ) );
    memset( &fcb_src[ 1 ], '?', 11 + 4 ); // name, type, EXTENT,S1,S2,RC: "????????.???"????
    /* 3. Find 1st file */
    result = bdos( CPM_FFST, fcb_src ); // BDOS function 17 (F_SFIRST) - search for first

    while ( result != 255 && count < MAX_FILES ) { // OK: result = 0..3
        /* record is in default DMA (0x80) */
        /* 32 bytes dir entries according index (0-3) in 128 bytes record */
        dir_entry = (cpm_dir *)( DEF_DMA + ( result * 32 ) );

        /* only if not erased (0xE5) */
        if ( dir_entry->user != 0xE5 ) {
            char clean_name[ 9 ], clean_ext[ 4 ];

            // clean attribute bits and spaces
            for ( int i = 0; i < 8; i++ )
                clean_name[ i ] = dir_entry->name[ i ] & 0x7F;
            clean_name[ 8 ] = '\0';
            for ( int i = 0; i < 3; i++ )
                clean_ext[ i ] = dir_entry->type[ i ] & 0x7F;
            clean_ext[ 3 ] = '\0';
            // save attributes
            p->files[ count ].attrib = 0;
            for ( uint8_t bit = 0; bit < 3; ++bit )
                if ( dir_entry->type[ bit ] > 0x7F )
                    p->files[ count ].attrib |= 1 << bit;

            p->files[ count ].extent = ( (uint16_t)( dir_entry->s2 ) * 32 ) + dir_entry->ex;
            p->files[ count ].rc = dir_entry->rc;
            // Format (e.g.: "NAME    EXT" -> "NAME.EXT")
            if ( *clean_ext != ' ' ) // "NAME.EXT"
                sprintf( p->files[ count ].cpmname, "%s.%s", strtok( clean_name, " " ), clean_ext );
            else // "NAME"
                strcpy( p->files[ count ].cpmname, strtok( clean_name, " " ) );

            // handle the CP/M3 date/time entry
            // check if date time info exists in the 4th 32 byte directory entry at default DMA
            if ( result < 3 && *( (uint8_t *)( DEF_DMA + 3 * 0x20 ) ) == '!' ) { // yes
                if ( PANEL_WIDTH >= 40 )                                         // no date/time display for narrow panels
                    p->show_date = 1;
                date_time_dir *dtd = (date_time_dir *)( DEF_DMA + 3 * 0x20 );
                p->files[ count ].date = dtd->dt[ result ].update.date;
                p->files[ count ].hour = dtd->dt[ result ].update.hour;
                p->files[ count ].minute = dtd->dt[ result ].update.minute;
                days_to_date( &( p->files[ count ].date ) );
            } else { // no date/time file info
                p->files[ count ].date = 0;
                p->files[ count ].month = 0;
                p->files[ count ].day = 0;
                p->files[ count ].hour = 0;
                p->files[ count ].minute = 0;
            }
            count++;
        }

        /* find all other files */
        result = bdos( CPM_FNXT, fcb_src ); // BDOS function 18 (F_SNEXT) - search for next
    }

    // sort file names and extents,
    // mark all extents except the last one
    // move (most of) the marked extents to the end of list

    qsort( (void *)p->files, count, sizeof( FileEntry ), fileNameExtentCompare );

    uint16_t f_idx;

    // remove all marked extents at the end of the array
    f_idx = count;
    while ( f_idx-- && *( p->files[ f_idx ].cpmname ) == '~' )
        --count;

    // remove marked extents within the array, copy array one up
    FileEntry *it = p->files;
    FileEntry *end = &p->files[ count ];
    while ( it < end ) {
        while ( *( (char *)it->cpmname ) == '~' ) {
            memcpy( it, it + 1, ( end - it - 1 ) * sizeof( FileEntry ) );
            --count;
            --end;
        }
        ++it;
    }

    for ( uint16_t f_idx = 0; f_idx < count; ++f_idx )
        p->files[ f_idx ].extent = ( ( p->files[ f_idx ].extent << 7 ) + p->files[ f_idx ].rc );

    p->num_files = count;
}


// copy panel content when both panels show the same drive
static void copy_panel_content( Panel *src, Panel *dst ) {
    if ( src == dst )
        return;
    memcpy( dst->files, src->files, MAX_FILES * sizeof( FileEntry ) );
    dst->num_files = src->num_files;
    dst->current_idx = src->current_idx;
    dst->top_idx = src->top_idx;
    dst->show_date = src->show_date;
}


// Delete the active file on active panel
static int8_t delete_active_file() {
    Panel *p = App.active_panel;
    if ( p->num_files == 0 )
        return -1;
    prepare_fcb( p->files[ p->current_idx ].cpmname, p, NULL );
    return bdos( CPM_DEL, fcb_src ); // BDOS function 19 (F_DELETE) - delete file
}


static void more( const char *action, const char *file_name ) {
    stndout();
    printf( " %s: %s (<SPACE>: more | <ESC>: exit) ", action, file_name );
    stndend();
}


void view_file() {
    // unsigned char fcb[36];
    Panel *p = App.active_panel;
    int i;
    int line_count = -1;
    char *name = p->files[ p->current_idx ].cpmname;

    cls();
    gotoxy( 1 << 8 | 1 ); // home
    curoff();

    // Reset DMA to default!
    bdos( CPM_SDMA, DEF_DMA );

    prepare_fcb( name, p, NULL );
    // open and read
    if ( bdos( CPM_OPN, fcb_src ) != 255 ) {       // BDOS function 15 - (F_OPEN) - Open file
        while ( bdos( CPM_READ, fcb_src ) == 0 ) { // BDOS function 20 (F_READ) - read next record
            for ( i = 0; i < 128; i++ ) {
                char c = *( (char *)( DEF_DMA + i ) );
                if ( c == 0x1A )
                    goto end_of_file; // EOF (Ctrl+Z)
                putchar( c );
                if ( c == '\n' ) {
                    putchar( '\r' ); // CR for terminal
                    line_count++;
                    // wait after one screen page
                    if ( line_count >= PANEL_HEIGHT ) {
                        more( "VIEW", name );
                        if ( wait_key_hw() == ESC )
                            goto esc_file;
                        putchar( '\r' );
                        ereol();
                        line_count = 0;
                    }
                }
            }
        }
    } else {
        printf( "\r\nError opening file." );
    }
end_of_file:
    puts( "" ); // CRLF
    stndout();
    printf( " --- End Of File --- " ); // inv / normal
    stndend();
    wait_key_hw();
esc_file:
    cls(); // clear screen, hide cursor
    refresh_ui( PAN_BOTH );
}


// HEX and ASCII dump (16 bytes per line)
void dump_file() {
    Panel *p = App.active_panel;
    int i, j, line_count = -1;
    long address = 0;
    char *name_ptr = p->files[ p->current_idx ].cpmname;

    cls();
    gotoxy( 1 << 8 | 1 ); // home
    curoff();

    // Reset DMA to default!
    bdos( CPM_SDMA, DEF_DMA );

    prepare_fcb( p->files[ p->current_idx ].cpmname, p, NULL );

    if ( bdos( CPM_OPN, fcb_src ) != 255 ) {       // BDOS function 15 - (F_OPEN) - Open file
        while ( bdos( CPM_READ, fcb_src ) == 0 ) { // BDOS function 20 (F_READ) - read next record
            for ( i = 0; i < 128; i += 16 ) {
                printf( "%04X  ", (unsigned int)address );
                for ( j = 0; j < 16; j++ ) {
                    printf( "%02X ", *( (unsigned char *)( DEF_DMA + i + j ) ) );
                }
                printf( " |" );
                for ( j = 0; j < 16; j++ ) {
                    unsigned char c = *( (unsigned char *)( DEF_DMA + i + j ) );
                    if ( c >= 32 && c <= 126 )
                        putchar( c );
                    else
                        putchar( '.' );
                }
                printf( "|\r\n" );

                address += 16;
                line_count++;

                if ( line_count >= PANEL_HEIGHT ) {
                    more( "DUMP", name_ptr );
                    if ( wait_key_hw() == 27 )
                        goto esc_file;
                    putchar( '\r' );
                    ereol();
                    line_count = 0;
                }
            }
        }
    } else {
        printf( "\r\nError opening file." );
    }
    puts( "" );
    stndout();
    printf( " --- End Of File --- " );
    stndend();
    wait_key_hw();
esc_file:
    cls(); // clear screen, hide cursor
    refresh_ui( PAN_BOTH );
}


// copy a specific file by its name using asm function
static int16_t copy_file_by_name( Panel *src, Panel *dst, const char *name ) {
    int16_t rc = 0;
    prepare_fcb( name, src, dst );
    if ( DEBUG ) {
        gotoxy( DEBUG_ROW << 8 | 1 );
        ereol();
        printf( "copy: %s %s ", fcb_src + 1, fcb_dst + 1 );
    }
    rc = cpsrcdst();
    if ( DEBUG )
        printf( " status: %u", rc );
    return rc;
}


static uint8_t yes_no() {
    char k = wait_key_hw();
    return ( k == 'y' || k == 'Y' );
}


// process multi selections
static void exec_multi_copy( Panel *src, Panel *dst ) {
    int i, marked = 0, done = 0;
    int8_t rc;

    if ( DEBUG ) {
        gotoxy( DEBUG_ROW << 8 | 1 ); // debugging line
        printf( "copy" );
    }

    for ( i = 0; i < src->num_files; i++ )
        if ( src->files[ i ].attrib & B_SEL )
            ++marked;
    if ( marked == 0 ) {
        gotoxy( STATUS_ROW << 8 | 1 );
        ereol();
        printf( " Copying: %s", src->files[ src->current_idx ].cpmname );
        rc = copy_file_by_name( src, dst, src->files[ src->current_idx ].cpmname );
    } else {
        // clear dialog box and ask
        gotoxy( STATUS_ROW << 8 | 1 );
        ereol();
        printf( " COPY SELECTED FILE(S) TO %c:? (Y/N) ", dst->drive );
        if ( !yes_no() )
            return;

        for ( i = 0; i < src->num_files; i++ ) {
            if ( src->files[ i ].attrib & B_SEL ) {
                done++;
                gotoxy( STATUS_ROW << 8 | 1 );
                ereol();
                printf( " Copying [%d/%d]: %s", done, marked, src->files[ i ].cpmname );
                rc = copy_file_by_name( src, dst, src->files[ i ].cpmname );
                if ( rc )
                    break;
                src->files[ i ].attrib &= ~B_SEL;
                if ( is_on_panel( i ) )
                    draw_file_line( src, i ); // remove the visible '*' marks
            }
        }
    }
    if ( rc ) {
        if ( 1 == rc )
            printf( " - directory full" );
        else if ( 2 == rc )
            printf( " - disk full" );
        else if ( 0xFF == rc )
            printf( " - HW error" );
        else
            printf( " - error %d", rc );
        printf( " (press any key)" );
        wait_key_hw();
        bdos( CPM_SRDS, 1 << ( dst->drive - 'A' ) ); // DRV_RESET - Selectively reset disc drives
    }
    // the refresh will be done by main.c after calling this function.
}


static void exec_multi_delete( Panel *p ) {
    int i, marked = 0, done = 0;
    // clear dialog box and ask
    gotoxy( STATUS_ROW << 8 | 1 );
    ereol();
    printf( " DELETE SELECTED FILE(S)? (Y/N) " );
    if ( !yes_no() )
        return;
    // count number of selections
    for ( i = 0; i < p->num_files; i++ ) {
        if ( p->files[ i ].attrib & B_SEL )
            marked++;
    }
    if ( marked == 0 ) {
        // if none selected, delete  the current file (original functionality)
        gotoxy( STATUS_ROW << 8 | 1 );
        ereol();
        printf( " Deleting: %s... ", p->files[ p->current_idx ].cpmname );
        delete_active_file( p );
    } else {
        // batch deletion
        for ( i = 0; i < p->num_files; i++ ) {
            if ( p->files[ i ].attrib & B_SEL ) {
                ++done;
                gotoxy( STATUS_ROW << 8 | 1 );
                ereol();
                printf( " [%d/%d] Deleting: %s ", done, marked, p->files[ i ].cpmname );
                prepare_fcb( p->files[ i ].cpmname, p, NULL );
                bdos( CPM_DEL, fcb_src ); // BDOS function 19 (F_DELETE) - delete file
                p->files[ i ].attrib &= ~B_SEL;
            }
        }
    }
    // the refresh will be done by main.c after calling this function.
}


void copy_cmd() {
    if ( !App.active_panel->num_files           // nothing to do
         || App.left.drive == App.right.drive ) // cannot copy to same drive
        return;
    Panel *dst = ( App.active_panel == &App.left ) ? &App.right : &App.left;
    exec_multi_copy( App.active_panel, dst );
    load_directory( App.inactive_panel );
    fill_panel( App.inactive_panel );
    gotoxy( STATUS_ROW << 8 | 1 );
    ereol(); // clear status line
}


void delete_cmd() {
    if ( !App.active_panel->num_files ) // nothing to do
        return;
    exec_multi_delete( App.active_panel );
    load_directory( App.active_panel );
    fill_panel( App.active_panel );
    // if left == right update both panels
    if ( App.left.drive == App.right.drive ) {
        if ( App.active_panel == &App.left )
            copy_panel_content( &App.left, &App.right );
        else
            copy_panel_content( &App.right, &App.left );
        fill_panel( App.inactive_panel );
    }
    gotoxy( STATUS_ROW << 8 | 1 );
    ereol(); // clear status line
}


void change_drive( char k ) {
    if ( App.active_panel->drive == k )
        return;
    App.active_panel->drive = k;
    load_directory( App.active_panel );
    refresh_ui( PAN_ACTIVE );
}
