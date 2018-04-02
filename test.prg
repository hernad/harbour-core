#include "hbtrace.ch"
#include "hbgtinfo.ch"
#include "inkey.ch"


/*

inkeyapi.c

int hb_inkeyLast( int iEventMask )
{
   int iKey = 0;
   PHB_GT pGT;

   HB_TRACE( HB_TR_DEBUG, ( "hb_inkeyLast(%d)", iEventMask ) );

   pGT = hb_gt_Base();
   if( pGT )
   {
      iKey = HB_GTSELF_INKEYLAST( pGT, iEventMask );
      hb_gt_BaseFree( pGT );
   }
   return iKey;
}


#define HB_GTSELF_INKEYLAST(g,m)                (g)->pFuncTable->InkeyLast(g,m)


hbgtcore.h:

int       (* InkeyLast) ( HB_GT_PTR, int iEventMask );


hbgtcore.c:


// Wait for keyboard input 
static int hb_gt_def_InkeyGet( PHB_GT pGT, HB_BOOL fWait, double dSeconds, int iEventMask )
{
   HB_MAXUINT end_timer;
   PHB_ITEM pKey;
   HB_BOOL fPop;

   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_def_InkeyGet(%p,%d,%f,%d)", pGT, ( int ) fWait, dSeconds, iEventMask ) );

   pKey = NULL;

   if( pGT->pInkeyReadBlock )
   {
      int iKey;
      HB_GTSELF_UNLOCK( pGT );
      iKey = hb_itemGetNI( hb_vmEvalBlock( pGT->pInkeyReadBlock ) );
      HB_GTSELF_LOCK( pGT );
      if( iKey != 0 )
         return iKey;
   }

   // Wait forever ?, Use fixed value 100 for strict Clipper compatibility
   if( fWait && dSeconds * 100 >= 1 )
      end_timer = hb_dateMilliSeconds() + ( HB_MAXUINT ) ( dSeconds * 1000 );
   else
      end_timer = 0;

   for( ;; )
   {
      hb_gt_def_InkeyPollDo( pGT );
      fPop = hb_gt_def_InkeyNextCheck( pGT, iEventMask, &pGT->inkeyLast );

      if( fPop )
      {
         hb_gt_def_InkeyPop( pGT );
         if( ! pGT->pInkeyFilterBlock )
            break;
         pKey = hb_itemPutNI( pKey, pGT->inkeyLast );
         HB_GTSELF_UNLOCK( pGT );
         pGT->inkeyLast = hb_itemGetNI( hb_vmEvalBlockV( pGT->pInkeyFilterBlock, 1, pKey ) );
         HB_GTSELF_LOCK( pGT );
         if( pGT->inkeyLast != 0 )
            break;
      }

      // immediately break if a VM request is pending.
      if( ! fWait || hb_vmRequestQuery() != 0 ||
                    ( end_timer != 0 && end_timer <= hb_dateMilliSeconds() ) )
         break;

      HB_GTSELF_UNLOCK( pGT );
      hb_idleState();
      HB_GTSELF_LOCK( pGT );
   }

   if( pKey )
      hb_itemRelease( pKey );

   hb_idleReset();

   return fPop ? pGT->inkeyLast : 0;
}

// Return the value of the last key that was extracted
static int hb_gt_def_InkeyLast( PHB_GT pGT, int iEventMask )
{
   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_def_InkeyLast(%p,%d)", pGT, iEventMask ) );

   HB_GTSELF_INKEYPOLL( pGT );

   return hb_gt_def_InkeyFilter( pGT, pGT->inkeyLast, iEventMask );
}

// Set LastKey() value and return previous value
static int hb_gt_def_InkeySetLast( PHB_GT pGT, int iKey )
{
   int iLast;

   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_def_InkeySetLast(%p,%d)", pGT, iKey ) );

   iLast = pGT->inkeyLast;
   pGT->inkeyLast = iKey;

   return iLast;
}


gtcore.c

static HB_BOOL hb_gt_def_Info( PHB_GT pGT, int iType, PHB_GT_INFO pInfo )
{
...
      case HB_GTI_INKEYREAD:
         hb_gt_def_SetBlock( &pGT->pInkeyReadBlock, pInfo );
         break;


*/

nRows = 40
nCols = 120
cColor := "N/G,R/W,,,N/G"
SetMode( nRows, nCols )
set color to (cColor)
clear screen

? maxrow(), maxcol()

Set( _SET_OSCODEPAGE, hb_cdpOS() )
hb_cdpSelect( "SL852" ) // šŠćĆđĐžŽ

Set( _SET_EVENTMASK, INKEY_ALL )
MSetCursor( .T. )

? "press F1"

//? "export HB_TR_LEVEL=DEBUG"
//? "export HB_TR_OUTPUT=trace.txt"
//? "export HB_USER_CFLAGS=-DHB_TR_LEVEL=5"
inkey(0)

IF LastKey() == K_F1
  ? "Lastkey F1"
ENDIF

//? hb_traceState(HB_TR_DEBUG)
//inkey(0)
//? hb_traceState()
//inkey(0)

cGet := "AB  "

//? hb_gtInfo( HB_GTI_DESKTOPROWS, 40 )
//? hb_gtInfo( HB_GTI_DESKTOPCOLS, 70 )
//inkey(0)

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


