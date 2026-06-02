; NZTCAP:  VT100.Z80
; Author:  Joe Wright
; Date:    12 October 87
; Version: 1.0


; New Z3 Termcap for the DEC VT-100 (ANSI STANDARD 7-bit)
;
; Version 2.3D (11/6/90 ) - added hashed block using lower case "a"
; as suggested by Gene Pizzetta, and also used it for solid block.  Now
; works with Gene's DIRBAR and GREET10, which require a full extended
; TCAP. 			 		        Bob Dean
;
; Version 2.2D (6/02/90 - "D" is for VLIB4D, and doesn't mean dim)
; Used Terry Hazen's TCSRC13.COM as the basis, very thoroughly commented.
; Changed Curtis' assembly-time approach for implmenting reverse video
; to commented out SO/SE, which can be implemented to taste.  However, new
; programs using VLIB4D can use the attribute bit to have BOTH bright
; and reverse simultaneously.  Also picked up changes from 2.1.  We should
; have a VT-200 version to add buttons and buzzers under VT200 mode (both
; 7 and 8 bit...extended CSI).  Also need a VT-52 version. Graphics tested
; with the latest BOX.COM and SETATR.COM on a VT-220 in VT-100 mode.
; 				Bob Dean, DHN* Znode # 6 215-623-4040
;
; Version 2.0/2.1 - 5/25/90 Curtis Anderson, with comments by Terry Hazen
; Versions 1.5-1.9 skipped by Curtis Anderson - no reason given.
; Version 1.4D 6 May 90 (BETA 5/7/90) - Bob Dean
; Version 1.3  3 March 90 - Bob Dean
;
; Version 1.2  3 Feb 90
; Corrected standout characters by reversing, corrected clear screen, and
; cursor positioning.  Tested on my DEC VT220, which is likely credibly
; DEC ANSI standard in VT100 mode than Wyse or Heath. I think SE/SO are
; reversed in T3TCAP27 version too.  Can only believe that the WYSE terms,
; which portend to be a VT100 compatible would react the same (??). BD
;
; Version 1.1  19 Dec 89
; Expands Type to 16 bits
;
; Z3TCAP file:  NZDECTH.Z80
;
ESC	EQU	27		; Escape character
;
; The first character in the terminal name must not be a space.  For
; Z3TCAP.TCP library purposes only, the name terminates with a space
; and must be unique in the first eight characters.
;
TNAME:	DB	"VT-100R      "	; Name of terminal (13 chars)
;
GOFF:	DB	GOELD-TNAME	; Graphics offset from Z3TCAP start
;
; Terminal configuration bytes B14 and B15 are defined and bits assigned
; as follows.  The remaining bits are not currently assigned.  Set these
; bits according to your terminal configuration.
;
;	B14 b7: Z3TCAP Type.... 0 = Standard TCAP  1 = Extended TCAP
;
;	bit:	76543210
B14:	DB	10000000B	; Configuration byte B14
;
;	B15 b0: Standout....... 0 = Half-Intensity 1 = Reverse Video
;	B15 b1: Power Up Delay. 0 = None           1 = Ten-second delay
;	B15 b2: No Auto Wrap... 0 = Auto Wrap      1 = No Auto Wrap
;	B15 b3: No Auto Scroll. 0 = Auto Scroll    1 = No Auto Scroll
;	B15 b4: ANSI........... 0 = ASCII          1 = ANSI
;
;	bit:	76543210
B15:	DB	00010001B	; Configuration byte B15
;
; Single character arrow keys or WordStar diamond
;
	DB	'E'-40H		; Cursor up
	DB	'X'-40H		; Cursor down
	DB	'D'-40H		; Cursor right
	DB	'S'-40H		; Cursor left
;
; Delays (in ms) after sending terminal control strings
;
	DB	0		; CL delay
	DB	0		; CM delay
	DB	0		; CE delay
;
; Strings start here
;
CL:	DB	ESC,"[;H",ESC,"[J",0 ; Home cursor and clear screen
CM:	DB	ESC,"[%i%d;%dH",0 ; Cursor motion macro
CE:	DB	ESC,"[K",0      ; Erase from cursor to end-of-line
SO:	DB	ESC,"[7m",0     ; Start standout mode - reverse
SE:	DB	ESC,"[m",0      ; End standout mode - reverse
TI:	DB	ESC,"[m",0      ; Terminal initialization (using SE:)
TE:	DB	ESC,"[m",0      ; Terminal deinitialization (using SE:)
;
; Extensions to standard Z3TCAP
;
LD:	DB	ESC,"[M",0      ; Delete line at cursor position
LI:	DB	ESC,"[L",0      ; Insert line at cursor position
CD:	DB	ESC,"[J",0      ; Erase from cursor to end-of-screen
;
; The attribute string contains the four command characters to set
; the following four attributes for this terminal in the following
; order:  	Normal, Blink, Reverse, Underscore
;
SA:	DB	ESC,"[%Dm",0    ; Set screen attributes macro
AT:	DB	"0574",0        ; Attribute string
RC:	DB	0               ; Read current cursor position
;RC:	DB	ESC,'5',0       ; Read current cursor pos.- WYSE 85 (ca)
RL:	DB	0               ; Read line until cursor
;
; Graphics TCAP area
;
GOELD:	DB	0		; Graphics On/Off delay in ms
;
; Graphics strings
;
GO:	DB	ESC,"(0",0      ; Graphics mode On
GE:	DB	ESC,"(B",0      ; Graphics mode Off
CDO:	DB	ESC,"[?25l",0    ; Cursor Off  (ca)
CDE:	DB	ESC,"[?25h",0    ; Cursor On   (ca)
;
; Graphics characters
;
GULC:	DB	'l'		; Upper left corner		5
GURC:	DB	'k'		; Upper right corner		6
GLLC:	DB	'm'		; Lower left corner		7
GLRC:	DB	'j'		; Lower right corner		8
GHL:	DB	'q'		; Horizontal line		9
GVL:	DB	'x'		; Vertical line		       10
GFB:	DB	'a'		; Full block (Used hashed)     11
GHB:	DB	'a'		; Hashed block 		       12
GUI:	DB	'w'		; Upper intersect	       13
GLI:	DB	'v'		; Lower intersect              14
GIS:	DB	'n'		; Mid intersect		       15
GRTI:	DB	'u'		; Right intersect	       16
GLTI:	DB	't'		; Left intersect	       17
;
; End of Z3TCAP
;

ALIGN	128
