#ifndef __VLIB_H
#define __VLIB_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void z3vinit( uint8_t *z3env ) __z88dk_fastcall;

extern uint8_t gz3init( uint8_t *z3env ) __z88dk_fastcall;

extern int stndout( void ) __z88dk_callee;

extern int stndend( void ) __z88dk_callee;

extern int gotoxy( int xy ) __z88dk_fastcall;

extern int cls( void ) __z88dk_callee;

extern int clreos( void ) __z88dk_callee;

extern int ereol( void ) __z88dk_callee;

extern void gxymsg( char *str ) __z88dk_fastcall;

extern void vprint( char *str ) __z88dk_fastcall;

extern void vpstr( char *str ) __z88dk_fastcall;

extern void dellin( void ) __z88dk_callee;

extern void inslin( void ) __z88dk_callee;

extern uint8_t curon( void ) __z88dk_callee;

extern uint8_t curoff( void ) __z88dk_callee;

extern uint8_t grxon( void ) __z88dk_callee;

extern uint8_t grxoff( void ) __z88dk_callee;

extern uint8_t uleft( void ) __z88dk_callee;

extern uint8_t uright( void ) __z88dk_callee;

extern uint8_t lleft( void ) __z88dk_callee;

extern uint8_t lright( void ) __z88dk_callee;

extern uint8_t uisec( void ) __z88dk_callee;

extern uint8_t ltisec( void ) __z88dk_callee;

extern uint8_t rtisec( void ) __z88dk_callee;

extern uint8_t lisec( void ) __z88dk_callee;

extern void gbox( uint16_t pos, uint16_t size ) __z88dk_callee;

#endif
