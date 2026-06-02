
SECTION code_user

PUBLIC _z3vinit, _gz3init, _gxymsg, _stndout, _stndend, _gotoxy, _cls, _clreos, _ereol
PUBLIC _vprint, _vpstr, _dellin, _inslin, _curoff, _curon, _grxon, _grxoff
PUBLIC _uleft, _uright, _lleft, _lright, _ltisec, _rtisec, _uisec, _lisec
PUBLIC GETCRT, _gbox

EXTERN Z3VINIT, GZ3INIT, GXYMSG, STNDOUT, STNDEND, GOTOXY, CLS, CLREOS, EREOL
EXTERN VPRINT, VPSTR, DELLIN, INSLIN, CUROFF, CURON, GRXON, GRXOFF
EXTERN ULEFT, URIGHT, LLEFT, LRIGHT, LTISEC, RTISEC, UISEC, LISEC
EXTERN GBOX, _COLUMNS

_z3vinit:
    call Z3VINIT
    ret

_gz3init:
    call GZ3INIT
    jr   aret

_stndout:
    call STNDOUT
    jr   aret

_stndend:
    call STNDEND
    jr   aret

_gotoxy:
    call GOTOXY
    jr   aret

_gxymsg:
    push hl
    ld  hl,GXYMSG
    jp  (hl)

_vprint:
    push hl
    ld  hl,VPRINT
    jp  (hl)

_vpstr:
    call VPSTR
    jr  aret

_dellin:
    call DELLIN
    jr  aret

_inslin:
    call INSLIN
    jr  aret

_cls:
    call CLS
    jr  aret

_clreos:
    call CLREOS
    jr  aret

_ereol:
    call EREOL
    jr  aret

aret:
    ld   h,0
    ld   l,a
    or   a
    ret

_curoff:
    call CUROFF
    jr  aret

_curon:
    call CURON
    jr  aret

_grxon:
    call GRXON
    jr  aret

_grxoff:
    call GRXOFF
    jr  aret

_uleft:
    call ULEFT
    jr  aret

_uright:
    call URIGHT
    jr  aret

_lleft:
    call LLEFT
    jr  aret

_lright:
    call URIGHT
    jr  aret

_ltisec:
    call LTISEC
    jr  aret

_rtisec:
    call RTISEC
    jr  aret

_uisec:
    call UISEC
    jr  aret

_lisec:
    call LISEC
    jr  aret

GETCRT:
    ld      hl, _COLUMNS
    ret

_gbox:
    pop     de      ; ret addr
    pop     bc      ; size = 256 * width + height (!)
    pop     hl      ; pos  = 256 * row + col
    call    GBOX
    push    de      ; ret addr
    ret

