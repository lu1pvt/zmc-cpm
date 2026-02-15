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

#define MAX_FILES 256
#define FILENAME_LEN 13
#define SCREEN_HEIGHT (*LINES) // 32
#define PANEL_WIDTH (*COLUMNS/2) //40
#define PANEL_HEIGHT (SCREEN_HEIGHT - 2) // 30  // Ajustable según la terminal
#define VISIBLE_ROWS (PANEL_HEIGHT - 2)

#define ESC 0x1B


extern uint8_t *CONFIG;

extern uint8_t *LINES;
extern uint8_t *COLUMNS;

extern uint8_t DEBUG;

/* https://www.seasip.info/Cpm/format22.html
 *
 * The CP/M 2.2 directory has only one type of entry:
 *
 * UU F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 EX S1 S2 RC   .FILENAMETYP....
 * AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL   ................
 *
 * UU = User number. 0-15 (on some systems, 0-31). The user number allows multiple
 *     files of the same name to coexist on the disc.
 *      User number = 0E5h => File deleted
 * Fn - filename
 * Tn - filetype. The characters used for these are 7-bit ASCII.
 *        The top bit of T1 (often referred to as T1') is set if the file is
 *      read-only.
 *        T2' is set if the file is a system file (this corresponds to "hidden" on
 *      other systems).
 * EX = Extent counter, low byte - takes values from 0-31
 * S2 = Extent counter, high byte.
 *
 *       An extent is the portion of a file controlled by one directory entry.
 *     If a file takes up more blocks than can be listed in one directory entry,
 *     it is given multiple entries, distinguished by their EX and S2 bytes. The
 *     formula is: Entry number = ((32*S2)+EX) / (exm+1) where exm is the
 *     extent mask value from the Disc Parameter Block.
 *
 * S1 - reserved, set to 0.
 * RC - Number of records (1 record=128 bytes) used in this extent, low byte.
 *     The total number of records used in this extent is
 *
 *     (EX & exm) * 128 + RC
 *
 *     If RC is >=80h, this extent is full and there may be another one on the
 *     disc. File lengths are only saved to the nearest 128 bytes.
 *
 * AL - Allocation. Each AL is the number of a block on the disc. If an AL
 *     number is zero, that section of the file has no storage allocated to it
 *     (ie it does not exist). For example, a 3k file might have allocation
 *     5,6,8,0,0.... - the first 1k is in block 5, the second in block 6, the
 *     third in block 8.
 *      AL numbers can either be 8-bit (if there are fewer than 256 blocks on the
 *     disc) or 16-bit (stored low byte first).
 */

/* https://www.seasip.info/Cpm/format31.html
 *
 * CP/M 3.1 directory
 * ==================
 * The CP/M 3.1 directory has four types of entry:
 *
 * Files:
 * 0U F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 EX S1 S2 RC   .FILENAMETYP....
 * AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL   ................
 *
 * 0U = User number. 0-15. The user number allows multiple files of the same name
 *     to coexist on the disc.
 *      User number = 0E5h => File deleted
 * Fn - filename
 * Tn - filetype. The characters used for these are 7-bit ASCII.
 *        The top bit of T1 (often referred to as T1') is set if the file is
 *      read-only.
 *        T2' is set if the file is a system file (this corresponds to "hidden" on
 *      other systems). System files with user number 0 can be read from any user
 *      number.
 *        T3' is set if the file has been backed up.
 * EX = Extent counter, low byte - takes values from 0-31
 * S2 = Extent counter, high byte.
 *
 *       An extent is the portion of a file controlled by one directory entry.
 *     If a file takes up more blocks than can be listed in one directory entry,
 *     it is given multiple entries, distinguished by their EX and S2 bytes. The
 *     formula is: Entry number = ((32*S2)+EX) / (exm+1) where exm is the
 *     extent mask value from the Disc Parameter Block.
 *
 * S1 - Last Record Byte Count
 * RC - Number of records (1 record=128 bytes) used in this extent, low byte.
 *     The total number of records used in this extent is
 *
 *     (EX & exm) * 128 + RC
 *
 *     If RC is >= 80h, this extent is full and there may be another one on
 *     the disc. File lengths are optionally saved exactly (using the S1 byte)
 *     but this system is hardly ever used.
 * AL - Allocation. Each AL is the number of a block on the disc. If an AL
 *     number is zero, that section of the file has no storage allocated to it
 *     (ie it does not exist). For example, a 3k file might have allocation
 *     5,6,8,0,0.... - the first 1k is in block 5, the second in block 6, the
 *     third in block 8.
 *      AL numbers can either be 8-bit (if there are fewer than 256 blocks on the
 *     disc) or 16-bit (stored low byte first).
 *
 * Disc label
 * ==========
 * 20 F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 LB PB RR RR     LABENAMETYP....
 * P1 P2 P3 P4 P5 P6 P7 P8 D1 D1 D1 D1 D2 D2 D2 D2    ................
 *
 * 20h - Characteristic number of a disc label
 * F1-F8, T1-T3 - Label name, 7-bit ASCII
 * LB - Label byte. Bit 0 set => Label exists
 *                  Bit 4 set => Time stamp on create --+
 *                  Bit 5 set => Time stamp on update   +--These 2 are mutually
 *                  Bit 6 set => Time stamp on access --+   exclusive
 *                  Bit 7 set => Password protection enabled
 * PB - Used to decode the label password
 * RR - Reserved, set to zero.
 * P1-P8 - password, rather feebly encrypted.
 * D1 - Label create datestamp
 * D2 - Label update datestamp
 *
 * Date stamps
 * ===========
 * If date stamps are in use, then every fourth directory entry will be
 * a date stamp entry, containing stamps for the preceding three entries.
 * 21 D1 D1 D1 D1 D2 D2 D2 D2 M1 00 D3 D3 D3 D3 D4    !...............
 * D4 D4 D4 M2 00 D5 D5 D5 D5 D6 D6 D6 D6 M3 00 00    ................
 *
 * 21h - Characteristic number of a date stamp.
 * D1  - File 1 create OR access date
 * D2  - File 1 update date
 * D3  - File 2 create OR access date
 * D4  - File 2 update date
 * D5  - File 3 create OR access date
 * D6  - File 3 update date
 * M1  - File 1 password mode
 * M2  - File 2 password mode
 * M3  - File 3 password mode
 * 00  - Reserved.
 *
 * The format of a date stamp is:
 *
 * 	DW	day	;Julian day number, stored low byte first.
 * 			;Day 1 = 1 Jan 1978.
 * 	DB	hour	;BCD hour, eg 13h => 13:xx
 * 	DB	min	;BCD minute
 *
 * Password control
 * ================
 * 1U F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 PM PB RR RR   .FILENAMETYP....
 * P1 P2 P3 P4 P5 P6 P7 P8 RR RR RR RR RR RR RR RR   ................
 *
 * 1U = 16+User number (ie 16-31). The user number will be the number of
 *     the file to which the password belongs.
 * F1-F8 - Filename of the file to which the password belongs
 * T1-T3 - Filetype of the file to which the password belongs
 * PM    - Password mode byte
 *          Bit 7 set => Password required to read from file
 *          Bit 6 set => Password required to write to file
 *          Bit 5 set => Password required to delete file
 * PB    - Used to decode the password
 * P1-P8 - The password, rather feebly encrypted.
 * RR    - Reserved, set to 0.
 * Password encryption system
 * This system is extremely simple:
 *
 * When making the password, add all 8 bytes together (packing with spaces if necessary).
 * This becomes PB (the decode byte). XOR each byte with PB and store them backwards in
 * the directory (ie the last byte becomes P1).
 * To decode the password, XOR PB with the 8 bytes of the password and read it off backwards.
 */


/* CP/M directory entry (32 bytes) */
typedef struct cpm_dir {
    uint8_t user;
    uint8_t name[8]; // F1..F8, hi bit can be user flag
    uint8_t ext[3];  // T1..T3, hi bits are system flags
    uint8_t extent;
    uint8_t s1, s2;
    uint8_t rc;
    uint8_t map[16];
} cpm_dir;


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
    uint8_t cpmname[12]; // "NAME    EXT\0"
    uint16_t size_kb;
    uint8_t selected;  // 0 = No, 1 = Yes
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


void set_invers( void );
void set_normal( void );
void clrscr( void );
void print_cpm_name( char *cn );
void draw_panel(Panel *p, uint8_t x_offset);
void load_directory(Panel *p);
uint8_t wait_key_hw(void);
int delete_file(Panel *p);
int copy_file(Panel *src, Panel *dst);
void draw_file_line(Panel *p, uint8_t x_offset, uint16_t file_idx);
void view_file(Panel *p);
void dump_file(Panel *p);
int copy_file_by_index(Panel *src, Panel *dst, uint16_t idx);
void exec_multi_copy(Panel *src, Panel *dst);
void exec_multi_delete(Panel *p);
#endif
