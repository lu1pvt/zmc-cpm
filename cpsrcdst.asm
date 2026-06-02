; buffered file copy, using multi sector read/write on CP/M3 systems

DEFC    BDOS        =   0005H
DEFC    TBUFF       =   0080H  ;i/o buffer and command line storage
DEFC    TPA         =   0100H  ;transient program storage area
;
;
;    BDOS Control codes
;
DEFC    CPM_RCON = 1
DEFC    CPM_WCON = 2
DEFC    CPM_RRDR = 3
DEFC    CPM_WPUN = 4
DEFC    CPM_WLST = 5
DEFC    CPM_DCIO = 6
DEFC    CPM_GIOB = 7
DEFC    CPM_SIOB = 8
DEFC    CPM_PRST = 9
DEFC    CPM_RCOB = 10
DEFC    CPM_ICON = 11
DEFC    CPM_VERS = 12
DEFC    CPM_RDS  = 13
DEFC    CPM_LGIN = 14
DEFC    CPM_OPN  = 15
DEFC    CPM_CLS  = 16
DEFC    CPM_FFST = 17
DEFC    CPM_FNXT = 18
DEFC    CPM_DEL  = 19
DEFC    CPM_READ = 20
DEFC    CPM_WRIT = 21
DEFC    CPM_MAKE = 22
DEFC    CPM_REN  = 23
DEFC    CPM_ILOG = 24
DEFC    CPM_IDRV = 25
DEFC    CPM_SDMA = 26
DEFC    CPM_SUID = 32
DEFC    CPM_RRAN = 33
DEFC    CPM_WRAN = 34
DEFC    CPM_CFS  = 35
DEFC    CPM_MULT = 44


SECTION     data_user

rdsek: DEFB     0           ;read sector index
wrsek: DEFB     0           ;write sector index
endof: DEFB     0           ;end of file flag

;PUBLIC      _status
EXTERN      _fcb_src
EXTERN      _fcb_dst
EXTERN      _cpbufpt
EXTERN      _cpbufsz

DEFC    sfcbcr  = _fcb_src+32   ;current read record
DEFC    dfcbcr  = _fcb_dst+32   ;current write record

_status: DEFB    0


SECTION     code_user

PUBLIC      _cpsrcdst

_cpsrcdst:
;   open source file
    ld      de,_fcb_src
    ld      c,CPM_OPN
    call    BDOS            ;A=0xFF: error
    ld      (_status),a
    inc     a
    jp      z,exit

;   source file open, prepare destination
    ld      de,_fcb_dst
    ld      c,CPM_DEL
    call    BDOS
    ld      de,_fcb_dst
    ld      c,CPM_MAKE
    call    BDOS            ;A=0xFF: error
    ld      (_status),a
    inc     a
    jp      z,exit

;   zero the current records
    xor     a
    ld      (sfcbcr),a
    ld      (dfcbcr),a
    ld      (endof),a       ;we're not at the end of file

ifdef  USE_CPM3_MULTI_SECTOR

;   check the CP/M version
;   use multi sector rd/wr for CP/M 3 (currently buggy!)
;   use single sector rd/wr otherwise
;
    ld      c,CPM_VERS
    call    BDOS
    cp      31h             ; CP/M3?
    jp      z,copy_loop_3   ; yes

endif

;   COPY LOOP for CP/M 2.2 (SINGLE SECTOR)
;   source file open, dest file open
;   copy until end of file from source
;
copy_loop_2:
;   check we're not eof
    xor     a
    ld      hl,endof
    or      (hl)
    jr      nz,copy_done    ;jump to end of file

;   zero the sector indexes
    ld      (rdsek),a
    ld      (wrsek),a

;   set dma address
    ld      de,(_cpbufpt)

read_loop_2:
    call    setdma
    ld      de,_fcb_src     ;source
    ld      c,CPM_READ
    call    BDOS            ;read next record
    ld      (_status),a
    cp      1               ;end of file?
    jr      z,endoffile_2   ;skip to fileend if so
    or      a
    jr      nz,exit         ;error

    ld      a,(rdsek)
    inc     a
    ld      (rdsek),a
    ld      hl,_cpbufsz
    cp      (hl)
    jr      nc,write_start_2

    ld      l,a             ;ready to shift
    xor     a
    rr      l               ;shift left 7, for 128 bytes sectors
    rra
    ld      h,l
    ld      l,a

    ld      de,(_cpbufpt)
    add     hl,de           ;create new destination dma address
    ex      de,hl

    jr      read_loop_2

endoffile_2:
    ld      (endof),a       ;set end of file flag

write_start_2:
    ld      hl,rdsek        ;preincrement so we can decrement before test.
    inc     (hl)

;   set dma address
    ld      de,(_cpbufpt)

write_loop_2:
    call    setdma

    ld      a,(rdsek)
    dec     a
    ld      (rdsek),a
    jr      z,copy_loop_2   ;reached end of file before end of buffers

    ld      de,_fcb_dst     ;destination
    ld      c,CPM_WRIT
    call    BDOS            ;write record
    ld      (_status),a
    or      a               ;0 if write OK
    jr      nz,exit         ;end if so

    ld      a,(wrsek)
    inc     a
    ld      (wrsek),a
    ld      hl,_cpbufsz     ;number of buffers we have
    cp      (hl)
    jr      nc,copy_loop_2

    ld      l,a             ;ready to shift
    xor     a
    rr      l               ;shift left 7, for 128 bytes sectors
    rra
    ld      h,l
    ld      l,a

    ld      de,(_cpbufpt)
    add     hl,de           ;create new source dma address
    ex      de,hl

    jr      write_loop_2      ;loop until buffer


; common exit routines for CP/M 2.2 and CP/M 3
copy_done:
    ld      de,_fcb_dst     ;destination close
    ld      c,CPM_CLS
    call    BDOS            ;255 if error
    ld      (_status),a
    inc     a
    jr      z,exit

    ld      de,_fcb_src     ;source close
    ld      c,CPM_CLS
    call    BDOS

exit:
    ld      de,TBUFF
    call    setdma
    ld      h,0
    ld      a,(_status)
    ld      l,a
    ret


setdma:
    ld      c,CPM_SDMA
    jp      BDOS


ifdef USE_CPM3_MULTI_SECTOR

;   DO NOT USE, CAN CRASH THE SYSTEM!
;   COPY LOOP for CP/M 3 (MULTI SECTOR)
;   source file open, dest file open
;   copy until end of file from source
copy_loop_3:
;   check we're not eof
    xor     a
    ld      hl,endof
    or      (hl)
    jr      nz,copy_done    ;jump to end of file

;   set dma address
    ld      de,(_cpbufpt)
    call    setdma

    ld      c,CPM_MULT
    ld      de,_cpbufsz
    ld      e,(de)
    call    BDOS

    ld      de,_fcb_src     ;source
    ld      c,CPM_READ
    call    BDOS            ;read next records
    ld      (_status),a
    cp      1               ;end of file?
    jr      z,endoffile_3   ;skip to fileend if so
    or      a
    jr      nz,exit      ;error

    ld      de,_cpbufsz
    ld      e,(de)
    jr      write_start_3

endoffile_3:
    ld      (endof),a       ;set end of file flag
    ld      e,h             ;number of sectors read

write_start_3:
    ld      c,CPM_MULT
    call    BDOS

;   set dma address
    ld      de,(_cpbufpt)
    call    setdma

    ld      de,_fcb_dst     ;destination
    ld      c,CPM_WRIT
    call    BDOS            ;write records
    ld      (_status),a
    or      a               ;0 if write OK
    jr      nz,exit      ;end if so

    jr      copy_loop_3

endif
