#include "hbtrace.ch"
#include "hbgtinfo.ch"
#include "inkey.ch"


Set( _SET_OSCODEPAGE, hb_cdpOS() )
hb_cdpSelect( "SL852" ) // šŠćĆđĐžŽ

Set( _SET_EVENTMASK, INKEY_ALL )
MSetCursor( .T. )


? "export HB_TR_LEVEL=DEBUG"
? "export HB_TR_OUTPUT=trace.txt"
? "export HB_USER_CFLAGS=-DHB_TR_LEVEL=5"
inkey(0)

//? hb_traceState(HB_TR_DEBUG)
//inkey(0)
//? hb_traceState()
//inkey(0)

cGet := "AB  "

? hb_gtInfo( HB_GTI_DESKTOPROWS, 40 )
? hb_gtInfo( HB_GTI_DESKTOPCOLS, 70 )
inkey(0)

hb_gtInfo( HB_GTI_WINTITLE, "TTTTTITLE XTERMJS" )

Alert( "hello world" )

CLEAR SCREEN
Tone( 300, 2)
SET BELL ON

@ 10, 10 SAY "Hello" GET cGet

READ

? "cGet=", cGet


//? hb_gtInfo( HB_GTI_DESKTOPROWS, 50 )
//? hb_gtInfo( HB_GTI_DESKTOPCOLS, 120 )
//inkey(0)


