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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cpm.h>

#include "zmc.h"


uint8_t fcb_src[36];
uint8_t fcb_dst[36];


void prepare_fcb( char *name_ptr, Panel *src, Panel *dst ) {
// setup one or two FCBs for reading, copying or deleting

    if ( src ) {
        memset(fcb_src, 0, sizeof(fcb_src));
        *fcb_src = (src->drive - 'A') + 1;
        memset(fcb_src+1, ' ', 11);
        for( uint8_t j = 0; j < 8 && name_ptr[j] != '.' && name_ptr[j] != '\0'; j++)
            fcb_src[1+j] = name_ptr[j];
        while( *name_ptr && *name_ptr != '.')
            name_ptr++;
        if( *name_ptr == '.') {
            name_ptr++;
            for(uint8_t j = 0; j < 3 && name_ptr[j] != '\0'; j++)
                fcb_src[9+j] = name_ptr[j];
        }
    }
    if ( dst ) {
        memset(fcb_dst, 0, sizeof(fcb_dst));
        *fcb_dst = (dst->drive - 'A') + 1;
        memset(fcb_dst+1, ' ', 11);
        for( uint8_t j = 0; j < 8 && name_ptr[j] != '.' && name_ptr[j] != '\0'; j++)
            fcb_dst[1+j] = name_ptr[j];
        while( *name_ptr && *name_ptr != '.')
            name_ptr++;
        if( *name_ptr == '.') {
            name_ptr++;
            for(uint8_t j = 0; j < 3 && name_ptr[j] != '\0'; j++)
                fcb_dst[9+j] = name_ptr[j];
        }
    }
}


// Function to check if a year is leap
uint8_t is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Array of days in month for normal and leap years
const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},  // normal year
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}   // leap year
};

// convert CP/M "Days since 1.1.1978" to YYMD
// input:  date -> DD from CP/M directory
// return: date -> YYMD
void days_to_date( void *date ) {
    uint16_t *cpm_date = (uint16_t *)date;
    uint8_t *cpm_month = (uint8_t *)(date+2);
    uint8_t *cpm_day = (uint8_t *)(date+3);

    // Handle day 0;
    if (*cpm_date == 0) {
        *cpm_day = 0;
        *cpm_month = 0;
    } else {
        // Start from 1978
        *cpm_date -= 1;
        uint16_t year = 1978;

        // Approximate year to reduce iterations
        while (*cpm_date >= 365) {
            uint16_t days_in_year = is_leap_year(year) ? 366 : 365;
            if (*cpm_date < days_in_year) break;
            *cpm_date -= days_in_year;
            ++year;
        }

        // Find exact month and day
        uint8_t leap = is_leap_year(year) ? 1 : 0;
        uint8_t month = 0;

        while (*cpm_date >= days_in_month[leap][month]) {
            *cpm_date -= days_in_month[leap][month];
            ++month;
        }

        *cpm_day = *cpm_date + 1;
        *cpm_month = month + 1;
        *cpm_date = year;
    }
}


// Three-way compare function for the name field of FileEntry used by qsort
static int fileNameExtentCompare(const void* a, const void* b) {
    // 1. compare cpmname
    // 2. if equal, compare extent and mark files with lower extent as invalid
    int res = strcmp( ((const FileEntry*)a)->cpmname, ((const FileEntry*)b)->cpmname );
    if ( res ) // names are different
        return res;
    // only the 1st extent has date/time info
    if (((const FileEntry*)a)->date  && !((const FileEntry*)b)->date )
        memcpy( &((FileEntry*)b)->date, &((const FileEntry*)a)->date, 6 );
    else if (((const FileEntry*)b)->date  && !((const FileEntry*)a)->date )
        memcpy( &((FileEntry*)a)->date, &((const FileEntry*)b)->date, 6 );
    // now compare the extents
    res = ((const FileEntry*)a)->extent - ((const FileEntry*)b)->extent;
    if ( res < 0 )
        *((FileEntry*)a)->cpmname = '~';
    else
        *((FileEntry*)b)->cpmname = '~';
    return res;
}


// Three-way compare function for the name field of FileEntry used by qsort
static int fileNameCompare(const void* a, const void* b) {
    // setting up rules for comparison
    return strcmp( ((const FileEntry*)a)->cpmname, ((const FileEntry*)b)->cpmname );
}


void load_directory(Panel *p) {
    cpm_dir *dir_entry;
    uint16_t count = 0;
    uint8_t result;

    p->num_files = 0;
    p->current_idx = 0;
    p->scroll_offset = 0;

    if (p->drive == '@') // '@' -> select current drive
        p->drive = bdos( 25, fcb_src ) + 'A';
    /* 1. change drive to fetch the complete directory */
    bdos(14, p->drive - 'A'); 

    /* 2. Prepare FCB to match all files (*.*) and all extents */
    memset(fcb_src, 0, sizeof(fcb_src));
    fcb_src[0] = 0; // current drive
    memset(&fcb_src[1], '?', 11+4); // name, type, EXTENT,S1,S2,RC: "????????.???"????
    /* 3. Find 1st file */
    result = bdos(17, fcb_src); // BDOS function 17 (F_SFIRST) - search for first

    while (result != 255 && count < MAX_FILES) { // OK: result = 0..3
        /* record is in default DMA (0x80) */
        /* 32 bytes dir entries according index (0-3) in 128 bytes record */
        dir_entry = (cpm_dir *)(0x80 + (result * 32));

        /* only if not erased (0xE5) */
        if (dir_entry->user != 0xE5) {
            char clean_name[9], clean_ext[4];

            // clean attribute bits and spaces
            for(int i=0; i<8; i++) clean_name[i] = dir_entry->name[i] & 0x7F;
            clean_name[8] = '\0';
            for(int i=0; i<3; i++) clean_ext[i] = dir_entry->type[i] & 0x7F;
            clean_ext[3] = '\0';
            // save attributes
            p->files[count].attrib = 0;
            for ( uint8_t bit = 0; bit < 3; ++bit )
            if (dir_entry->type[bit] > 0x7F)
                p->files[count].attrib |= 1 << bit;

            p->files[count].extent = ((uint16_t)(dir_entry->s2) * 32 ) + dir_entry->ex;
            p->files[count].rc = dir_entry->rc;
            // Format (e.g.: "NAME    EXT" -> "NAME.EXT")
            sprintf(p->files[count].cpmname, "%s.%s", strtok(clean_name, " "), clean_ext);

            // handle the CP/M3 date/time entry
            // check if date time info exists in the 4th 32 byte directory entry
            if ( result < 3 && *((uint8_t *)(0xE0)) == '!' ) { // yes
                date_time_dir *dtd = (date_time_dir *)(0xE0);
                p->files[count].date = dtd->dt[result].update.date;
                p->files[count].hour = dtd->dt[result].update.hour;
                p->files[count].minute = dtd->dt[result].update.minute;
                days_to_date( &(p->files[count].date) );
            } else { // no date/time file info
                p->files[count].date = 0;
                p->files[count].month = 0;
                p->files[count].day = 0;
                p->files[count].hour = 0;
                p->files[count].minute = 0;
            }
            count++;
        }

        /* find all other files */
        result = bdos(18, fcb_src); // BDOS function 18 (F_SNEXT) - search for next
    }

    // sort file names and extents,
    // mark all extents except the last one
    // move (most of) the marked extents to the end of list

    qsort((void *)p->files, count, sizeof(FileEntry), fileNameExtentCompare);

    uint16_t f_idx;

    // remove all marked extents at the end of the array
    f_idx = count;
    while (f_idx-- && *(p->files[f_idx].cpmname ) == '~' )
        --count;

    // remove marked extents within the array, copy array one up
    FileEntry *it = p->files;
    FileEntry *end = &p->files[count];
    while ( it < end ) {
        while ( *((char *)it->cpmname) == '~' ) {
            memcpy( it, it+1, (end-it-1)*sizeof(FileEntry) );
            --count;
            --end;
        }
        ++it;
    }

    for ( uint16_t f_idx = 0; f_idx < count; ++f_idx )
        p->files[f_idx].extent = ( (p->files[f_idx].extent << 7 ) + p->files[f_idx].rc);

    p->num_files = count;

}


// Delete the selected file(s) on active panel
int delete_file(Panel *p) {
    if (p->num_files == 0) return -1;
    prepare_fcb(p->files[p->current_idx].cpmname, p, NULL );
    return bdos(19, fcb_src); // BDOS function 19 (F_DELETE) - delete file
}


// Copy the selected file(s) to the opposite panel
int copy_file(Panel *src, Panel *dst) {
    // any files to copy?
    if (src->num_files == 0) return -1;
    // fill the FCBs
    prepare_fcb(src->files[src->current_idx].cpmname, src, dst);
    // prepare transfer
    bdos(19, fcb_dst); // BDOS function 19 (F_DELETE) - delete file
    if (bdos(15, fcb_src) == 255) return -2; // BDOS function 15 - Open directory
    if (bdos(22, fcb_dst) == 255) return -3; // BDOS function 22 (F_MAKE) - create file
    // do a sector to sector transfer
    while (bdos(20, fcb_src) == 0) // BDOS function 20 (F_READ) - read next record
        // write data in 0x80 (DMA) to DST
        if (bdos(21, fcb_dst) != 0) break; // BDOS function 21 (F_WRITE) - write next record
    // finish the transfer
    bdos(16, fcb_dst); // BDOS function 16 - Close directory
    return 0;
}


void show_header() {
    printf("\x1b[2J\x1b[H\x1b[?25l"); // erase, home, hide cursor
}


void show_footer( const char *action, const char *file_name ) {
    printf("\x1b[7m %s: %s (<SPACE>: more | <ESC>: exit) \x1b[0m", action, file_name); // inv / normal
}


void view_file(Panel *p) {
    // unsigned char fcb[36];
    int i;
    int line_count = -1;
    char *name_ptr = p->files[p->current_idx].cpmname;
    char *temp_ptr;

    if (p->num_files == 0) return;
    show_header();
    prepare_fcb(p->files[p->current_idx].cpmname, p, NULL);
    // open and read
    if (bdos(15, fcb_src) != 255) { // BDOS function 15 - Open directory
        while (bdos(20, fcb_src) == 0) { // BDOS function 20 (F_READ) - read next record
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
                        printf("\r\x1b[K"); // CR, erase EOL
                        if (k == 27) return; // exit with ESC
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


// HEX and ASCII dump (16 bytes per line)
void dump_file(Panel *p) {
    int i, j, line_count = -1;
    long address = 0;
    char *name_ptr = p->files[p->current_idx].cpmname;

    if (p->num_files == 0) return;

    show_header();
    prepare_fcb(p->files[p->current_idx].cpmname, p, NULL);

    if (bdos(15, fcb_src) != 255) { // BDOS function 15 - Open directory
        while (bdos(20, fcb_src) == 0) { // BDOS function 20 (F_READ) - read next record
            for (i = 0; i < 128; i += 16) {
                printf("%04X  ", (unsigned int)address);
                for (j = 0; j < 16; j++) {
                    printf("%02X ", *((unsigned char *)(0x80 + i + j)));
                }
                printf(" |");
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


// copy a specific file by its index
int copy_file_by_index(Panel *src, Panel *dst, uint16_t f_idx) {
    prepare_fcb(src->files[f_idx].cpmname, src, dst);
    bdos(19, fcb_dst); // BDOS function 19 (F_DELETE) - delete file
    if (bdos(15, fcb_src) == 255) return -1; // BDOS function 15 - Open directory
    if (bdos(22, fcb_dst) == 255) return -1; // BDOS function 22 (F_MAKE) - create file
    while (bdos(20, fcb_src) == 0) // BDOS function 20 (F_READ) - read next record
        if (bdos(21, fcb_dst) != 0) break; // BDOS function 21 (F_WRITE) - write next record
    bdos(16, fcb_dst); // BDOS function 16 - Close directory
    return 0;
}


/* 2. process multi selections */
void exec_multi_copy(Panel *src, Panel *dst) {
    int i, marcados = 0, procesados = 0;

    for (i = 0; i < src->num_files; i++) {
        if (src->files[i].attrib & B_SEL) marcados++;
    }

    if (marcados == 0) {
        printf("\x1b[%d;1H\x1b[7m Copying: %s... \x1b[0m",
               SCREEN_HEIGHT-1, src->files[src->current_idx].cpmname);
        copy_file_by_index(src, dst, src->current_idx);
    } else {
        for (i = 0; i < src->num_files; i++) {
            if (src->files[i].attrib & B_SEL) {
                procesados++;
                printf("\x1b[%d;1H\x1b[7m [%d/%d] Copying: %s \x1b[0m",
                       SCREEN_HEIGHT-1, procesados, marcados, src->files[i].cpmname);

                // USAR EL NOMBRE CORRECTO AQUÍ:
                copy_file_by_index(src, dst, i);
                src->files[i].attrib &= ~B_SEL;
            }
        }
    }
    load_directory(dst);
    // the refresh will be done by main.c after calling this function.
}

void exec_multi_delete(Panel *p) {
    int i, marcados = 0, procesados = 0;
    // count number of selections
    for (i = 0; i < p->num_files; i++) {
        if (p->files[i].attrib & B_SEL) marcados++;
    }
    if (marcados == 0) {
        // if none selected, delete  the current file (original functionality)
        printf("\x1b[%d;1H\x1b[K Deleting: %s... ", SCREEN_HEIGHT-1, p->files[p->current_idx].cpmname);
        delete_file(p);
    } else {
        // batch deletion
        for (i = 0; i < p->num_files; i++) {
            if (p->files[i].attrib & B_SEL) {
                procesados++;
                printf("\x1b[%d;1H\x1b[K [%d/%d] Deleting: %s ", // pos, erase to EOL
                       SCREEN_HEIGHT-1,  procesados, marcados, p->files[i].cpmname);

                prepare_fcb(p->files[i].cpmname, p, NULL);
                bdos(19, fcb_src); // BDOS function 19 (F_DELETE) - delete file
                p->files[i].attrib &= ~B_SEL;
            }
        }
    }
    // clear dialog part
    printf("\x1b[%d;1H\x1b[K", SCREEN_HEIGHT-1 ); // pos, erase EOL
}
