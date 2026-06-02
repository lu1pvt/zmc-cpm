; LIBRARY:  SYSENV.LIB
; AUTHOR:  RICHARD CONN
; VERSION:  1.0
; DATE:  22 FEB 84
; PREVIOUS VERSIONS:  NONE
;**************************************************************************
;
; AUTHOR: JAY SAGE (with actual modification by Steven Gold)
; VERSION: 2.0 for NZCOM/ZCPR34
; DATE: May 21, 1988
; PREVIOUS VERSIONS: 1.0
;*************************************************************************
;	SYSENV IS THE DEFINITION FOR THE ZCPR3 ENVIRONMENT.
;
; SYSENV	MACRO

ALIGN 128

	PUBLIC	_INTENV, _INTCOLUMNS, _INTLINES, _INTLINES2

;  ENVIRONMENT DESCRIPTOR
;	IF INLINE, THERE IS A LEADING JMP JUST BEFORE THIS
;
_INTENV:
        JP      0000            ; jp BIOS_WBOOT
	DB	"Z3ENV"		; Environment id
	DB	80H             ; Class 4 environment (internal)

EXPATH: DW	0000		; External path address
EXPATHS: DB	0		; Number of 2-byte elements in path

RCP:    DW	0000		; Rcp address
RCPS:   DB	0		; Number of 128-byte blocks in rcp

IOP:    DW	0000		; Iop address
IOPS:   DB	0		; Number of 128-byte blocks in iop

FCP:    DW	0000		; Fcp address
FCPS:   DB	0000		; Number of 128-byte blocks in fcp

Z3NDIR: DW	0000		; Ndr address
Z3NDIRS: DB	0000		; Number of 18-byte entries in ndr

Z3CL:   DW	0000		; Zcpr3 command line
Z3CLS:  DB	0		; Number of bytes in command line

Z3ENV:  DW	0000		; Zcpr3 environment descriptor
Z3ENVS: DB	0		; Number of 128-byte blocks in descriptor

SHSTK:  DW	0000		; Shell stack address
SHSTKS: DB	0		; Number of shsize-byte entires in shell stack
SHSIZE: DB	0		; Size of a shell stack entry

Z3MSG:  DW	0000		; Zcpr3 message buffer

EXTFCB: DW	0000		; Zcpr3 external fcb

EXTSTK: DW	0000		; Zcpr3 external stack

QUIET:  DB	0		; Quiet flag (1=quiet, 0=not quiet)

Z3WHL:  DW	0000		; Address of wheel byte

MHZ:    DB	0 		; Processor speed in Mhz

MAXDSK: DB	'P'-'@'		; Maximum disk
MAXUSR: DB	31		; Maximum user
DUOK:   DB	1		; 1=ok to accept du, 0=not ok

	DB	0		; Crt selection (0=crt 0, 1=crt 1)
	DB	0		; Printer selection (n=printer n)

_INTCOLUMNS:
	DB	80		; Width of crt 0
_INTLINES:
	DB	24		; Number of lines on crt 0
_INTLINES2:
	DB	22		; Number of lines of text on crt 0

DRVEC:  DW	0000		; Drive Vector *sg*

	DB	00		; Space

 	DB	80		; Width of printer 0
	DB	66		; Number of lines on printer 0
	DB	58		; Number of lines of text on printer 0
	DB	1		; Form feed flag (0=can't formfeed, 1=can)

	DB	00		; Space
	DB	00		; Space
	DB	00		; Space
	DB	00		; Space

CCP:    DW	0000		; CCP Address	*sg*
CCPS:   DB	0		; CCP length in records	*sg*
DOS:    DW	0000		; DOS Address 	*sg*
DOSS:   DB	0		; DOS length in records	*sg*
BIOS:   DW	0000		; BIOS Address 	*sg*

	DB	"SH      "	; Shell variable filename
	DB	"VAR"		; Shell variable filetype

	DB	"        "	; Filename 1
	DB	"   "		; Filetype 1

	DB	"        "	; Filename 2
	DB	"   "		; Filetype 2

	DB	"        "	; Filename 3
	DB	"   "		; Filetype 3

	DB	"        "	; Filename 4
	DB	"   "		; Filetype 4

ALIGN 128

