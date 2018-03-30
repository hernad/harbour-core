/*
 * Video subsystem - electron web desktop GT driver, based on GTELE, 
 * which is based from code of ther GT drivers (GTCRS, GTPCA)
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus /at/ priv.onet.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.txt.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site https://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

/* NOTE: User programs should never call this layer directly! */

#define HB_GT_NAME ELE

#define HB_GT_UNICODE_BUF

#include "hbgtcore.h"
#include "hbinit.h"
#include "hbapicdp.h"
#include "hbapistr.h"
#include "hbapiitm.h"
#include "hbapifs.h"
#include "hbdate.h"
#include "inkey.ch"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#if defined(HB_OS_UNIX)
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#if defined(HB_OS_WIN)
#include <windows.h>
#endif

//#if (defined(_MSC_VER)
//#include <conio.h>
//#endif

#endif

/*
#if defined(HB_HAS_GPM)
#include <gpm.h>
#if defined(HB_OS_LINUX) && 0
#include <linux/keyboard.h>
#else
#define KG_SHIFT 0
#define KG_CTRL 2
#define KG_ALT 3
#endif
#endif
*/

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

static int s_GtId;
static HB_GT_FUNCS SuperTable;
#define HB_GTSUPER (&SuperTable)
#define HB_GTID_PTR (&s_GtId)

#define HB_GTELE_ATTR_CHAR 0x00FF
#define HB_GTELE_ATTR_STD 0x0000

#define HB_GTELE_ATTR_ALT 0x0100
#define HB_GTELE_ATTR_PROT 0x0100
#define HB_GTELE_ATTR_ACSC 0x0100

#define HB_GTELE_ATTR_BOX 0x0800

//#define TERM_ANSI 1
//#define TERM_LINUX 2
#define TERM_XTERM 3

#define HB_GTELE_CLRSTD 0
#define HB_GTELE_CLRX16 1
#define HB_GTELE_CLR256 2
#define HB_GTELE_CLRRGB 3
#define HB_GTELE_CLRAIX 4

#define STDIN_BUFLEN 128

#define ESC_DELAY 0 // vrijeme u kome se ceka ESC sekvenca

#define IS_EVTFDSTAT(x) ((x) >= 0x01 && (x) <= 0x03)
#define EVTFDSTAT_RUN 0x01
#define EVTFDSTAT_STOP 0x02
#define EVTFDSTAT_DEL 0x03

/* mouse button states */
#define M_BUTTON_LEFT 0x0001
#define M_BUTTON_RIGHT 0x0002
#define M_BUTTON_MIDDLE 0x0004
#define M_BUTTON_LDBLCK 0x0010
#define M_BUTTON_RDBLCK 0x0020
#define M_BUTTON_MDBLCK 0x0040
#define M_BUTTON_WHEELUP 0x0100
#define M_BUTTON_WHEELDOWN 0x0200
#define M_CURSOR_MOVE 0x0400
#define M_BUTTON_KEYMASK (M_BUTTON_LEFT | M_BUTTON_RIGHT | M_BUTTON_MIDDLE)
#define M_BUTTON_DBLMASK (M_BUTTON_LDBLCK | M_BUTTON_RDBLCK | M_BUTTON_MDBLCK)

#define MOUSE_NONE 0
#define MOUSE_GPM 1
#define MOUSE_XTERM 2

#define KEY_SHIFTMASK 0x01000000
#define KEY_CTRLMASK 0x02000000
#define KEY_ALTMASK 0x04000000
#define KEY_KPADMASK 0x08000000
#define KEY_EXTDMASK 0x10000000
#define KEY_CLIPMASK 0x20000000
/* 0x40000000 reserved for Harbour extended keys */
#define KEY_MASK 0xFF000000

#define CLR_KEYMASK(x) ((x) & ~KEY_MASK)
#define GET_KEYMASK(x) ((x)&KEY_MASK)

#define IS_CLIPKEY(x) ((((x) & ~0xffff) ^ KEY_CLIPMASK) == 0)
#define SET_CLIPKEY(x) (((x)&0xffff) | KEY_CLIPMASK)
#define GET_CLIPKEY(x) ((((x)&0x8000) ? ~0xffff : 0) | ((x)&0xffff))

#define CTRL_SEQ "\036"
#define ALT_SEQ "\037"
/*#define NATION_SEQ         "\016"*/

#define EXKEY_F1 (HB_KX_F1 | KEY_EXTDMASK)
#define EXKEY_F2 (HB_KX_F2 | KEY_EXTDMASK)
#define EXKEY_F3 (HB_KX_F3 | KEY_EXTDMASK)
#define EXKEY_F4 (HB_KX_F4 | KEY_EXTDMASK)
#define EXKEY_F5 (HB_KX_F5 | KEY_EXTDMASK)
#define EXKEY_F6 (HB_KX_F6 | KEY_EXTDMASK)
#define EXKEY_F7 (HB_KX_F7 | KEY_EXTDMASK)
#define EXKEY_F8 (HB_KX_F8 | KEY_EXTDMASK)
#define EXKEY_F9 (HB_KX_F9 | KEY_EXTDMASK)
#define EXKEY_F10 (HB_KX_F10 | KEY_EXTDMASK)
#define EXKEY_F11 (HB_KX_F11 | KEY_EXTDMASK)
#define EXKEY_F12 (HB_KX_F12 | KEY_EXTDMASK)
#define EXKEY_UP (HB_KX_UP | KEY_EXTDMASK)
#define EXKEY_DOWN (HB_KX_DOWN | KEY_EXTDMASK)
#define EXKEY_LEFT (HB_KX_LEFT | KEY_EXTDMASK)
#define EXKEY_RIGHT (HB_KX_RIGHT | KEY_EXTDMASK)
#define EXKEY_DEL (HB_KX_DEL | KEY_EXTDMASK)
#define EXKEY_HOME (HB_KX_HOME | KEY_EXTDMASK)
#define EXKEY_END (HB_KX_END | KEY_EXTDMASK)
#define EXKEY_PGUP (HB_KX_PGUP | KEY_EXTDMASK)
#define EXKEY_PGDN (HB_KX_PGDN | KEY_EXTDMASK)
#define EXKEY_INS (HB_KX_INS | KEY_EXTDMASK)
#define EXKEY_BS (HB_KX_BS | KEY_EXTDMASK)
#define EXKEY_TAB (HB_KX_TAB | KEY_EXTDMASK)
#define EXKEY_ESC (HB_KX_ESC | KEY_EXTDMASK)
#define EXKEY_ENTER (HB_KX_ENTER | KEY_EXTDMASK)
#define EXKEY_CENTER (HB_KX_CENTER | KEY_EXTDMASK)
#define EXKEY_PRTSCR (HB_KX_PRTSCR | KEY_EXTDMASK)
#define EXKEY_PAUSE (HB_KX_PAUSE | KEY_EXTDMASK)

#define K_UNDEF 0x10000
#define K_METAALT 0x10001
#define K_METACTRL 0x10002
#define K_NATIONAL 0x10003
#define K_MOUSETERM 0x10004
#define K_RESIZE 0x10005


//#if defined(HB_OS_UNIX)

#define TIMEVAL_GET(tv) gettimeofday(&(tv), NULL)
#define TIMEVAL_LESS(tv1, tv2) (((tv1).tv_sec == (tv2).tv_sec) ? ((tv1).tv_usec < (tv2).tv_usec) : ((tv1).tv_sec < (tv2).tv_sec))
#define TIMEVAL_ADD(dst, src, n)                                                  \
      do                                                                          \
      {                                                                           \
            (dst).tv_sec = (src).tv_sec + (n) / 1000;                             \
            if (((dst).tv_usec = (src).tv_usec + ((n) % 1000) * 1000) >= 1000000) \
            {                                                                     \
                  (dst).tv_usec -= 1000000;                                       \
                  (dst).tv_sec++;                                                 \
            }                                                                     \
      } while (0)

//#else

/*

#define TIMEVAL_GET(tv)              \
      do                             \
      {                              \
            (tv) = hb_dateSeconds(); \
      } while (0)
#define TIMEVAL_LESS(tv1, tv2) ((tv1) < (tv2))
#define TIMEVAL_ADD(dst, src, n)      \
      do                              \
      {                               \
            (dst) = (src) + n / 1000; \
      } while (0)

//#endif
*/
typedef struct
{
      int fd;
      int mode;
      int status;
      void *cargo;
      int (*eventFunc)(int, int, void *);
} evtFD;

typedef struct
{
      int row, col;
      int buttonstate;
      int lbuttons;
      int flags;
      int lbup_row, lbup_col;
      int lbdn_row, lbdn_col;
      int rbup_row, rbup_col;
      int rbdn_row, rbdn_col;
      int mbup_row, mbup_col;
      int mbdn_row, mbdn_col;

      /* to analize DBLCLK on xterm */
      struct timeval BL_time;
      struct timeval BR_time;
      struct timeval BM_time;

} mouseEvent;

typedef struct _keyTab
{
      int ch;
      int key;
      struct _keyTab *nextCh;
      struct _keyTab *otherCh;
} keyTab;

typedef struct
{
      int key;
      const char *seq;
} keySeq;

#define HB_GTELE_PTR struct _HB_GTELE *
#define HB_GTELE_GET(p) ((PHB_GTELE)HB_GTLOCAL(p))

typedef struct _HB_GTELE
{
      PHB_GT pGT;

      //HB_FHANDLE hFileno;
      HB_FHANDLE hFilenoStdin;
      HB_FHANDLE hFilenoStdout;
      HB_FHANDLE hFilenoStderr;

      int iMaxRows; // set windows rows
      int iMaxCols; // cols

      int iRow;
      int iCol;
      int iWidth;
      int iHeight;
      HB_SIZE nLineBufSize;
      char *pLineBuf;
      int iCurrentSGR /* current ansi attribute */, iFgColor, iBgColor, iBold, iBlink, iACSC, iExtColor, iAM;
      int iAttrMask;
      int iCursorStyle;
      HB_BOOL fAM;

      //HB_BOOL fOutTTY;
      HB_BOOL fStdinTTY;
      HB_BOOL fStdoutTTY;
      HB_BOOL fStderrTTY;

      //HB_BOOL fPosAnswer;

      HB_BOOL fUTF8;

#ifndef HB_GT_UNICODE_BUF
      PHB_CODEPAGE cdpIn;
      PHB_CODEPAGE cdpHost;
      PHB_CODEPAGE cdpTerm;
      PHB_CODEPAGE cdpBox;

      HB_UCHAR keyTransTbl[256];
#endif

      int charmap[256];

      int chrattr[256];
      int boxattr[256];

      int colors[16];

      char *szTitle;

      int iOutBufSize; // out buffer size 16K / unicode 32K
      int iOutBufIndex;
      char *pOutBuf;

      int terminal_type;
      int terminal_ext;

      //#if defined( HB_OS_UNIX )
      //struct termios saved_TIO /* external environment termios */, curr_TIO;
      //HB_BOOL fRestTTY;
      //#endif

      double dToneSeconds;

      /* input events */
      keyTab *pKeyTab;
      int key_flag;
      int esc_delay;
      int key_counter;
      int nation_mode;

      int mouse_type;
      int mButtons;
      int nTermMouseChars;
      unsigned char cTermMouseBuf[3];
      mouseEvent mLastEvt;
      //#if defined(HB_HAS_GPM)
      //      Gpm_Connect Conn;
      //#endif

      unsigned char stdin_buf[STDIN_BUFLEN];
      int stdin_ptr_l;
      int stdin_ptr_r;
      int stdin_inbuf; // stdin position, max STDIN_BUFLEN

      evtFD **event_fds; // matrica file deskriptora
      int efds_size;     // broj input event file descriptora, ovo je bez veze; vazda je samo 1
      int efds_no;

      /* terminal functions */

      void (*Init)(HB_GTELE_PTR);
      void (*Exit)(HB_GTELE_PTR);
      void (*SetTermMode)(HB_GTELE_PTR, int);
      HB_BOOL(*GetCursorPos)
      (HB_GTELE_PTR, int *, int *, const char *);
      void (*SetCursorPos)(HB_GTELE_PTR, int, int);
      void (*SetCursorStyle)(HB_GTELE_PTR, int);
      void (*SetAttributes)(HB_GTELE_PTR, int);
      HB_BOOL(*SetMode)
      (HB_GTELE_PTR, int *, int *);
      int (*GetAcsc)(HB_GTELE_PTR, unsigned char);
      void (*Tone)(HB_GTELE_PTR, double, double);
      void (*Bell)(HB_GTELE_PTR);
      const char *szAcsc;
} HB_TERM_STATE, HB_GTELE, *PHB_GTELE;

/* static variables use by signal handler */
/*
#if defined(HB_OS_UNIX)
static volatile HB_BOOL s_WinSizeChangeFlag = HB_FALSE;
#endif
*/

//#if defined(HB_OS_UNIX) && defined(SA_NOCLDSTOP)
//static volatile HB_BOOL s_fRestTTY = HB_FALSE;
//#endif

/* save old hilit tracking & enable mouse tracking */
static const char *s_szMouseOn = "\x1b[?1001s\x1b[?1002h";

static const char *s_szInit = "\x1b[?25l\x1b[10;18H\x1b[?25h";

/* disable mouse tracking & restore old hilit tracking */
static const char *s_szMouseOff = "\x1b[?1002l\x1b[?1001r";
static const char s_szBell[] = {HB_CHAR_BEL, 0};

/* conversion table for ANSI color indexes */
static const int s_AnsiColors[] = {0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15};

static int getClipKey(int nKey)
{
      int nRet = 0, nFlag, n;

      if (IS_CLIPKEY(nKey))
            nRet = GET_CLIPKEY(nKey);
      else if (HB_INKEY_ISEXT(nKey))
            nRet = nKey;
      else
      {
            n = GET_KEYMASK(nKey);
            nKey = CLR_KEYMASK(nKey);
            nFlag = 0;
            if (n & KEY_SHIFTMASK)
                  nFlag |= HB_KF_SHIFT;
            if (n & KEY_CTRLMASK)
                  nFlag |= HB_KF_CTRL;
            if (n & KEY_ALTMASK)
                  nFlag |= HB_KF_ALT;
            if (n & KEY_KPADMASK)
                  nFlag |= HB_KF_KEYPAD;

            if (n & KEY_EXTDMASK)
                  nRet = HB_INKEY_NEW_KEY(nKey, nFlag);
            else
            {
                  if (nKey > 0 && nKey < 32)
                  {
                        nFlag |= HB_KF_CTRL;
                        nKey += ('A' - 1);
                  }
                  nRet = HB_INKEY_NEW_KEY(nKey, nFlag);
            }
      }

      return nRet;
}

/* SA_NOCLDSTOP in #if is a hack to detect POSIX compatible environment 
#if defined(HB_OS_UNIX) && defined(SA_NOCLDSTOP)

static void sig_handler(int iSigNo)
{
      int e = errno, status;
      pid_t pid;

      switch (iSigNo)
      {
      case SIGCHLD:
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
                  ;
            break;
      case SIGWINCH:
            s_WinSizeChangeFlag = HB_TRUE;
            break;
      case SIGINT:
            // s_InetrruptFlag = HB_TRUE;
            break;
      case SIGQUIT:
            // s_BreakFlag = HB_TRUE; 
            break;
      case SIGTSTP:
            // s_DebugFlag = HB_TRUE;
            break;
      case SIGTTOU:
            //s_fRestTTY = HB_FALSE;
            break;
      }
      errno = e;
}

static void set_sig_handler(int iSig)
{
      struct sigaction act;

      sigaction(iSig, 0, &act);
      act.sa_handler = sig_handler;
#if defined(SA_RESTART)
      act.sa_flags = SA_RESTART | (iSig == SIGCHLD ? SA_NOCLDSTOP : 0);
#else
      act.sa_flags = (iSig == SIGCHLD ? SA_NOCLDSTOP : 0);
#endif
      sigaction(iSig, &act, 0);
}

static void set_signals(void)
{
      int i, sigs[] = {SIGINT, SIGQUIT, SIGTSTP, SIGWINCH, 0};

      // Ignore SIGPIPEs so they don't kill us.
      signal(SIGPIPE, SIG_IGN);
      for (i = 0; sigs[i]; ++i)
      {
            set_sig_handler(sigs[i]);
      }
}

#endif
*/

static int hb_gt_ele_getKbdState(PHB_GTELE pTerm)
{
      int iFlags = 0;

      if (pTerm->mLastEvt.flags & HB_KF_SHIFT)
            iFlags |= HB_GTI_KBD_SHIFT;
      if (pTerm->mLastEvt.flags & HB_KF_CTRL)
            iFlags |= HB_GTI_KBD_CTRL;
      if (pTerm->mLastEvt.flags & HB_KF_ALT)
            iFlags |= HB_GTI_KBD_ALT;

      return iFlags;
}

static int hb_gt_ele_getSize(PHB_GTELE pTerm, int *piRows, int *piCols)
{

      /*
      *piRows = *piCols = 0;
#if defined(HB_OS_UNIX) && defined(TIOCGWINSZ)
      if (pTerm->fOutTTY)
      {
            struct winsize win;

            if (ioctl(pTerm->hFileno, TIOCGWINSZ, (char *)&win) != -1)
            {
                  *piRows = win.ws_row;
                  *piCols = win.ws_col;
            }
      }
#else
      HB_SYMBOL_UNUSED(pTerm);
#endif

      if (*piRows <= 0 || *piCols <= 0)
      {
            char *env;
            if ((env = getenv("COLUMNS")) != NULL)
                  *piCols = atoi(env);
            if ((env = getenv("LINES")) != NULL)
                  *piRows = atoi(env);
      }

      
*/
      *piRows = pTerm->iMaxRows;
      *piCols = pTerm->iMaxCols;

      return *piRows > 0 && *piCols > 0;
}


static void hb_gt_ele_termFlush(PHB_GTELE pTerm)
{
      if (pTerm->iOutBufIndex > 0)
      {
            hb_fsWriteLarge(pTerm->hFilenoStdout, pTerm->pOutBuf, pTerm->iOutBufIndex);
            HB_TRACE(HB_TR_DEBUG, ("GTELE OUTPUT %s", pTerm->pOutBuf)); 
            pTerm->iOutBufIndex = 0;
      }
}

#if defined(HB_OS_UNIX)


static void hb_gt_ele_termOut(PHB_GTELE pTerm, const char *pStr, int iLen)
{
      if (pTerm->iOutBufSize)
      {
            while (iLen > 0)
            {
                  int i;
                  if (pTerm->iOutBufSize == pTerm->iOutBufIndex)
                        hb_gt_ele_termFlush(pTerm);
                  i = pTerm->iOutBufSize - pTerm->iOutBufIndex;
                  if (i > iLen)
                        i = iLen;
                  memcpy(pTerm->pOutBuf + pTerm->iOutBufIndex, pStr, i);
                  pTerm->iOutBufIndex += i;
                  pStr += i;
                  iLen -= i;
            }
      }
}
#else

static void hb_gt_std_termOut( PHB_GTELE pTerm, const char * szStr, HB_SIZE nLen )
{
   hb_fsWriteLarge( pTerm->hFilenoStdout, szStr, nLen );
}

#endif


/*

//#ifndef HB_GT_UNICODE_BUF
static void hb_gt_ele_termOutTrans(PHB_GTELE pTerm, const char *pStr, int iLen, int iAttr)
{
      if (pTerm->iOutBufSize)
      {
            PHB_CODEPAGE cdp = NULL;

            //if (pTerm->fUTF8)
            //{
            if ((iAttr & (HB_GTELE_ATTR_ACSC | HB_GTELE_ATTR_BOX)) &&
                pTerm->cdpBox)
                  cdp = pTerm->cdpBox;
            else if (pTerm->cdpHost)
                  cdp = pTerm->cdpHost;
            else
                  cdp = hb_vmCDP();
            //}

            if (cdp)
            {
                  while (iLen > 0)
                  {
                        int i = (pTerm->iOutBufSize - pTerm->iOutBufIndex) >> 2;
                        if (i < 4)
                        {
                              hb_gt_ele_termFlush(pTerm);
                              i = pTerm->iOutBufSize >> 2;
                        }
                        if (i > iLen)
                              i = iLen;
                        
                        HB_TRACE(HB_TR_DEBUG, ("GTELE OUTPUT %s", pStr));     
                        pTerm->iOutBufIndex += hb_cdpStrToUTF8Disp(cdp, pStr, i,
                                                                   pTerm->pOutBuf + pTerm->iOutBufIndex,
                                                                   pTerm->iOutBufSize - pTerm->iOutBufIndex);
                        pStr += i;
                        iLen -= i;
                  }
            }
            else
            {
                  hb_gt_ele_termOut(pTerm, pStr, iLen);
            }
      }
}
//#endif
*/

/* ************************************************************************* */

/*
 * KEYBOARD and MOUSE
 */

static int add_efds(PHB_GTELE pTerm, int fd, int mode,
                    int (*eventFunc)(int, int, void *), void *cargo)
{
      evtFD *pefd = NULL;
      int i;

      if (eventFunc == NULL && mode != O_RDONLY)
            return -1;

      /*
#if defined(HB_OS_UNIX)
      {
            int fl;
            if ((fl = fcntl(fd, F_GETFL, 0)) == -1)
                  return -1;

            fl &= O_ACCMODE;
            if ((fl == O_RDONLY && mode == O_WRONLY) ||
                (fl == O_WRONLY && mode == O_RDONLY))
                  return -1;
      }
#endif
*/
      for (i = 0; i < pTerm->efds_no && !pefd; i++)
            if (pTerm->event_fds[i]->fd == fd)
                  pefd = pTerm->event_fds[i];

      if (pefd)
      {
            pefd->mode = mode;
            pefd->cargo = cargo;
            pefd->eventFunc = eventFunc;
            pefd->status = EVTFDSTAT_RUN;
      }
      else
      {
            if (pTerm->efds_size <= pTerm->efds_no)
            {
                  if (pTerm->event_fds == NULL)
                        pTerm->event_fds = (evtFD **)
                            hb_xgrab((pTerm->efds_size += 1) * sizeof(evtFD *));
                  else
                        pTerm->event_fds = (evtFD **)
                            hb_xrealloc(pTerm->event_fds,
                                        (pTerm->efds_size += 1) * sizeof(evtFD *));
            }

            pefd = (evtFD *)hb_xgrab(sizeof(evtFD));
            pefd->fd = fd;
            pefd->mode = mode;
            pefd->cargo = cargo;
            pefd->eventFunc = eventFunc;
            pefd->status = EVTFDSTAT_RUN;
            pTerm->event_fds[pTerm->efds_no++] = pefd;
      }

      return fd;
}

/*
#if defined(HB_HAS_GPM)
static void del_efds(PHB_GTELE pTerm, int fd)
{
      int i, n = -1;

      for (i = 0; i < pTerm->efds_no && n == -1; i++)
            if (pTerm->event_fds[i]->fd == fd)
                  n = i;

      if (n != -1)
      {
            hb_xfree(pTerm->event_fds[n]);
            pTerm->efds_no--;
            for (i = n; i < pTerm->efds_no; i++)
                  pTerm->event_fds[i] = pTerm->event_fds[i + 1];
      }
}
#endif
*/

static void del_all_efds(PHB_GTELE pTerm)
{
      if (pTerm->event_fds != NULL)
      {
            int i;

            for (i = 0; i < pTerm->efds_no; i++)
                  hb_xfree(pTerm->event_fds[i]);

            hb_xfree(pTerm->event_fds);

            pTerm->event_fds = NULL;
            pTerm->efds_no = pTerm->efds_size = 0;
      }
}

static int getMouseKey(mouseEvent *mEvt)
{
      int nKey = 0;

      if (mEvt->lbuttons != mEvt->buttonstate)
      {
            if (mEvt->buttonstate & M_CURSOR_MOVE)
            {
                  nKey = HB_INKEY_NEW_MPOS(mEvt->col, mEvt->row);
                  mEvt->buttonstate &= ~M_CURSOR_MOVE;
            }
            else if (mEvt->buttonstate & M_BUTTON_WHEELUP)
            {
                  nKey = HB_INKEY_NEW_MKEY(K_MWFORWARD, mEvt->flags);
                  mEvt->buttonstate &= ~M_BUTTON_WHEELUP;
            }
            else if (mEvt->buttonstate & M_BUTTON_WHEELDOWN)
            {
                  nKey = HB_INKEY_NEW_MKEY(K_MWBACKWARD, mEvt->flags);
                  mEvt->buttonstate &= ~M_BUTTON_WHEELDOWN;
            }
            else
            {
                  int butt = mEvt->lbuttons ^ mEvt->buttonstate;

                  if (butt & M_BUTTON_LEFT)
                  {
                        if (mEvt->buttonstate & M_BUTTON_LEFT)
                        {
                              mEvt->lbdn_row = mEvt->row;
                              mEvt->lbdn_col = mEvt->col;
                        }
                        else
                        {
                              mEvt->lbup_row = mEvt->row;
                              mEvt->lbup_col = mEvt->col;
                        }
                        nKey = (mEvt->buttonstate & M_BUTTON_LEFT) ? ((mEvt->buttonstate & M_BUTTON_LDBLCK) ? K_LDBLCLK : K_LBUTTONDOWN) : K_LBUTTONUP;
                        nKey = HB_INKEY_NEW_MKEY(nKey, mEvt->flags);
                        mEvt->lbuttons ^= M_BUTTON_LEFT;
                        mEvt->buttonstate &= ~M_BUTTON_LDBLCK;
                  }
                  else if (butt & M_BUTTON_RIGHT)
                  {
                        if (mEvt->buttonstate & M_BUTTON_RIGHT)
                        {
                              mEvt->rbdn_row = mEvt->row;
                              mEvt->rbdn_col = mEvt->col;
                        }
                        else
                        {
                              mEvt->rbup_row = mEvt->row;
                              mEvt->rbup_col = mEvt->col;
                        }
                        nKey = (mEvt->buttonstate & M_BUTTON_RIGHT) ? ((mEvt->buttonstate & M_BUTTON_RDBLCK) ? K_RDBLCLK : K_RBUTTONDOWN) : K_RBUTTONUP;
                        nKey = HB_INKEY_NEW_MKEY(nKey, mEvt->flags);
                        mEvt->lbuttons ^= M_BUTTON_RIGHT;
                        mEvt->buttonstate &= ~M_BUTTON_RDBLCK;
                  }
                  else if (butt & M_BUTTON_MIDDLE)
                  {
                        if (mEvt->buttonstate & M_BUTTON_MIDDLE)
                        {
                              mEvt->mbdn_row = mEvt->row;
                              mEvt->mbdn_col = mEvt->col;
                        }
                        else
                        {
                              mEvt->mbup_row = mEvt->row;
                              mEvt->mbup_col = mEvt->col;
                        }
                        nKey = (mEvt->buttonstate & M_BUTTON_MIDDLE) ? ((mEvt->buttonstate & M_BUTTON_MDBLCK) ? K_MDBLCLK : K_MBUTTONDOWN) : K_MBUTTONUP;
                        nKey = HB_INKEY_NEW_MKEY(nKey, mEvt->flags);
                        mEvt->lbuttons ^= M_BUTTON_MIDDLE;
                        mEvt->buttonstate &= ~M_BUTTON_MDBLCK;
                  }
                  else
                        mEvt->lbuttons = mEvt->buttonstate;
            }
      }

      return nKey;
}

static void chk_mevtdblck(PHB_GTELE pTerm)
{
      int newbuttons = (pTerm->mLastEvt.buttonstate & ~pTerm->mLastEvt.lbuttons) & M_BUTTON_KEYMASK;

      if (newbuttons != 0)
      {
//#if defined(HB_OS_UNIX)
            struct timeval tv;
//#else
//            double tv;
//#endif

            TIMEVAL_GET(tv);
            if (newbuttons & M_BUTTON_LEFT)
            {
                  if (TIMEVAL_LESS(tv, pTerm->mLastEvt.BL_time))
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_LDBLCK;
                  TIMEVAL_ADD(pTerm->mLastEvt.BL_time, tv,
                              HB_GTSELF_MOUSEGETDOUBLECLICKSPEED(pTerm->pGT));
            }
            if (newbuttons & M_BUTTON_MIDDLE)
            {
                  if (TIMEVAL_LESS(tv, pTerm->mLastEvt.BM_time))
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_MDBLCK;
                  TIMEVAL_ADD(pTerm->mLastEvt.BM_time, tv,
                              HB_GTSELF_MOUSEGETDOUBLECLICKSPEED(pTerm->pGT));
            }
            if (newbuttons & M_BUTTON_RIGHT)
            {
                  if (TIMEVAL_LESS(tv, pTerm->mLastEvt.BR_time))
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_RDBLCK;
                  TIMEVAL_ADD(pTerm->mLastEvt.BR_time, tv,
                              HB_GTSELF_MOUSEGETDOUBLECLICKSPEED(pTerm->pGT));
            }
      }
}

static void set_tmevt(PHB_GTELE pTerm, unsigned char *cMBuf, mouseEvent *mEvt)
{
      int row, col;

      mEvt->flags = 0;
      if (cMBuf[0] & 0x04)
            mEvt->flags |= HB_KF_SHIFT;
      if (cMBuf[0] & 0x08)
            mEvt->flags |= HB_KF_ALT;
      if (cMBuf[0] & 0x10)
            mEvt->flags |= HB_KF_CTRL;

      col = cMBuf[1] - 33;
      row = cMBuf[2] - 33;
      if (mEvt->row != row || mEvt->col != col)
      {
            mEvt->buttonstate |= M_CURSOR_MOVE;
            mEvt->row = row;
            mEvt->col = col;
      }

      switch (cMBuf[0] & 0xC3)
      {
      case 0x0:
            mEvt->buttonstate |= M_BUTTON_LEFT;
            break;
      case 0x1:
            mEvt->buttonstate |= M_BUTTON_MIDDLE;
            break;
      case 0x2:
            mEvt->buttonstate |= M_BUTTON_RIGHT;
            break;
      case 0x3:
            mEvt->buttonstate &= ~(M_BUTTON_KEYMASK | M_BUTTON_DBLMASK);
            break;
      case 0x40:
            if (cMBuf[0] & 0x20)
                  mEvt->buttonstate |= M_BUTTON_WHEELUP;
            break;
      case 0x41:
            if (cMBuf[0] & 0x20)
                  mEvt->buttonstate |= M_BUTTON_WHEELDOWN;
            break;
      }
      chk_mevtdblck(pTerm);
      /* printf( "\r\nmouse event: %02x, %02x, %02x\r\n", cMBuf[ 0 ], cMBuf[ 1 ], cMBuf[ 2 ] ); */
}

/*
#if defined(HB_HAS_GPM)
static int set_gpmevt(int fd, int mode, void *cargo)
{
      int nKey = 0;
      PHB_GTELE pTerm;
      Gpm_Event gEvt;

      HB_SYMBOL_UNUSED(fd);
      HB_SYMBOL_UNUSED(mode);

      pTerm = (PHB_GTELE)cargo;

      if (Gpm_GetEvent(&gEvt) > 0)
      {
            pTerm->mLastEvt.flags = 0;
            if (gEvt.modifiers & (1 << KG_SHIFT))
                  pTerm->mLastEvt.flags |= HB_KF_SHIFT;
            if (gEvt.modifiers & (1 << KG_CTRL))
                  pTerm->mLastEvt.flags |= HB_KF_CTRL;
            if (gEvt.modifiers & (1 << KG_ALT))
                  pTerm->mLastEvt.flags |= HB_KF_ALT;

            pTerm->mLastEvt.row = gEvt.y;
            pTerm->mLastEvt.col = gEvt.x;
            if (gEvt.type & (GPM_MOVE | GPM_DRAG))
                  pTerm->mLastEvt.buttonstate |= M_CURSOR_MOVE;
            if (gEvt.type & GPM_DOWN)
            {
                  if (gEvt.buttons & GPM_B_LEFT)
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_LEFT;
                  if (gEvt.buttons & GPM_B_MIDDLE)
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_MIDDLE;
                  if (gEvt.buttons & GPM_B_RIGHT)
                        pTerm->mLastEvt.buttonstate |= M_BUTTON_RIGHT;
            }
            else if (gEvt.type & GPM_UP)
            {
                  if (gEvt.buttons & GPM_B_LEFT)
                        pTerm->mLastEvt.buttonstate &= ~M_BUTTON_LEFT;
                  if (gEvt.buttons & GPM_B_MIDDLE)
                        pTerm->mLastEvt.buttonstate &= ~M_BUTTON_MIDDLE;
                  if (gEvt.buttons & GPM_B_RIGHT)
                        pTerm->mLastEvt.buttonstate &= ~M_BUTTON_RIGHT;
            }
      }
      chk_mevtdblck(pTerm);
      nKey = getMouseKey(&pTerm->mLastEvt);

      return nKey ? (HB_INKEY_ISEXT(nKey) ? nKey : SET_CLIPKEY(nKey)) : 0;
}

static void flush_gpmevt(PHB_GTELE pTerm)
{
      if (gpm_fd >= 0)
      {
            struct timeval tv = {0, 0};
            fd_set rfds;

            FD_ZERO(&rfds);
            FD_SET(gpm_fd, &rfds);

            while (select(gpm_fd + 1, &rfds, NULL, NULL, &tv) > 0)
                  set_gpmevt(gpm_fd, O_RDONLY, (void *)pTerm);

            while (getMouseKey(&pTerm->mLastEvt))
                  ;
      }
}
#endif
*/

static void disp_mousecursor(PHB_GTELE pTerm)
{
      /*
#if defined(HB_HAS_GPM)
      if ((pTerm->mouse_type & MOUSE_GPM) && gpm_visiblepointer)
      {
            Gpm_DrawPointer(pTerm->mLastEvt.col, pTerm->mLastEvt.row,
                            gpm_consolefd);
      }
#else
*/
      HB_SYMBOL_UNUSED(pTerm);
      //#endif
}

static void mouse_init(PHB_GTELE pTerm)
{

      hb_gt_ele_termOut(pTerm, s_szMouseOn, strlen(s_szMouseOn));
      hb_gt_ele_termFlush(pTerm);
      memset((void *)&pTerm->mLastEvt, 0, sizeof(pTerm->mLastEvt));
      pTerm->mouse_type = MOUSE_XTERM;
      pTerm->mButtons = 3;
}

static void mouse_exit(PHB_GTELE pTerm)
{
      if (pTerm->mouse_type & MOUSE_XTERM)
      {
            hb_gt_ele_termOut(pTerm, s_szMouseOff, strlen(s_szMouseOff));
            hb_gt_ele_termFlush(pTerm);
      }
      /*
#if defined(HB_HAS_GPM)
      if ((pTerm->mouse_type & MOUSE_GPM) && gpm_fd >= 0)
      {
            del_efds(pTerm, gpm_fd);
            Gpm_Close();
      }
#endif
*/
}

static int read_bufch(PHB_GTELE pTerm, int fd)
{
      int n = 0;

      //if (STDIN_BUFLEN > pTerm->stdin_inbuf)
      //{
      unsigned char buf[STDIN_BUFLEN];
      int i;

      //#if defined(HB_OS_UNIX)
      //            n = read(fd, buf, STDIN_BUFLEN - pTerm->stdin_inbuf);
      //#else
      // n - number of read bytes
      n = hb_fsRead(fd, buf, STDIN_BUFLEN - pTerm->stdin_inbuf);

      //#endif

      for (i = 0; i < n; i++)
      {
            pTerm->stdin_buf[pTerm->stdin_ptr_r++] = buf[i];
            if (pTerm->stdin_ptr_r == STDIN_BUFLEN)
                  pTerm->stdin_ptr_r = 0;
            pTerm->stdin_inbuf++;
      }
      //}

      return n;
}

/*
  get in char u odredjenom periodu
*/

static int get_inch(PHB_GTELE pTerm, int milisec)
{
      int nRet = 0, nNext = 0, npfd = -1, nchk = pTerm->efds_no, lRead = 0;
      int mode, i, n;
      struct timeval tv, *ptv;
      evtFD *pefd = NULL;
      fd_set rfds, wfds;

      if (milisec == 0)
            ptv = NULL;
            //§tv = 0;
      else
      {
            if (milisec < 0)
                  milisec = 0;
            tv.tv_sec = (milisec / 1000);
            tv.tv_usec = (milisec % 1000) * 1000;
            ptv = &tv;
            //tv = milisec;
      }

      while (nRet == 0 && lRead == 0)
      {
            int counter;

            n = -1;
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            for (i = 0; i < pTerm->efds_no; i++)
            {
                  if (pTerm->event_fds[i]->status == EVTFDSTAT_RUN)
                  {
                        if (pTerm->event_fds[i]->mode == O_RDWR ||
                            pTerm->event_fds[i]->mode == O_RDONLY)
                        {
                              FD_SET(pTerm->event_fds[i]->fd, &rfds);
                              if (n < pTerm->event_fds[i]->fd)
                                    n = pTerm->event_fds[i]->fd;
                        }
                        if (pTerm->event_fds[i]->mode == O_RDWR ||
                            pTerm->event_fds[i]->mode == O_WRONLY)
                        {
                              FD_SET(pTerm->event_fds[i]->fd, &wfds);
                              if (n < pTerm->event_fds[i]->fd)
                                    n = pTerm->event_fds[i]->fd;
                        }
                  }
                  else if (pTerm->event_fds[i]->status == EVTFDSTAT_STOP &&
                           pTerm->event_fds[i]->eventFunc == NULL)
                        nNext = HB_INKEY_NEW_EVENT(HB_K_TERMINATE); // emituje na osnovu inkey event
            }

            counter = pTerm->key_counter;
            // https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141(v=vs.85).aspx

            if (select(n + 1, &rfds, &wfds, NULL, ptv) > 0)
            {
                  for (i = 0; i < pTerm->efds_no; i++)
                  {
                        n = (FD_ISSET(pTerm->event_fds[i]->fd, &rfds) ? 1 : 0) |
                            (FD_ISSET(pTerm->event_fds[i]->fd, &wfds) ? 2 : 0);
                        if (n != 0)
                        {
                              if (pTerm->event_fds[i]->eventFunc == NULL)
                              {
                                    lRead = 1;
                                    n = read_bufch(pTerm, pTerm->event_fds[i]->fd);
                                    if (n == 0)
                                    {
                                          pTerm->event_fds[i]->status = EVTFDSTAT_STOP;
                                          nRet = HB_INKEY_NEW_EVENT(HB_K_CLOSE); // emituje na osnovu inkey event
                                    }
                              }
                              else if (nRet == 0 && counter == pTerm->key_counter)
                              {
                                    if (n == 3)
                                          mode = O_RDWR;
                                    else if (n == 2)
                                          mode = O_WRONLY;
                                    else
                                          mode = O_RDONLY;
                                    pTerm->event_fds[i]->status = EVTFDSTAT_STOP;
                                    n = (pTerm->event_fds[i]->eventFunc)(pTerm->event_fds[i]->fd,
                                                                         mode,
                                                                         pTerm->event_fds[i]->cargo);
                                    if (IS_EVTFDSTAT(n))
                                    {
                                          pTerm->event_fds[i]->status = n;
                                          if (nchk > i)
                                                nchk = i;
                                    }
                                    else
                                    {
                                          pTerm->event_fds[i]->status = EVTFDSTAT_RUN;
                                          if (IS_CLIPKEY(n) || HB_INKEY_ISEXT(n))
                                          {
                                                nRet = n;
                                                npfd = pTerm->event_fds[i]->fd;
                                                if (nchk > i)
                                                      nchk = i;
                                          }
                                    }
                              }
                        }
                  }
            }
            else
                  lRead = 1;
      }

      for (i = n = nchk; i < pTerm->efds_no; i++)
      {
            if (pTerm->event_fds[i]->status == EVTFDSTAT_DEL)
                  hb_xfree(pTerm->event_fds[i]);
            else if (pTerm->event_fds[i]->fd == npfd)
                  pefd = pTerm->event_fds[i];
            else
            {
                  if (i > n)
                        pTerm->event_fds[n] = pTerm->event_fds[i];
                  n++;
            }
      }
      if (pefd)
            pTerm->event_fds[n++] = pefd;
      pTerm->efds_no = n;

      return nRet == 0 ? nNext : nRet;
}

/*
   test buf char, used by wait_key
*/
static int test_bufch(PHB_GTELE pTerm, int n, int delay)
{
      int nKey = 0;

      if (pTerm->stdin_inbuf == n)
      {
            nKey = get_inch(pTerm, delay);
            HB_TRACE(HB_TR_DEBUG, ("GTELE test_bufch key=%d n=%d", nKey, n));
      }

      return (IS_CLIPKEY(nKey) || HB_INKEY_ISEXT(nKey)) ? nKey : (pTerm->stdin_inbuf > n ? pTerm->stdin_buf[(pTerm->stdin_ptr_l + n) % STDIN_BUFLEN] : -1);
}

static void free_bufch(PHB_GTELE pTerm, int n)
{
      if (n > pTerm->stdin_inbuf)
            n = pTerm->stdin_inbuf;
      pTerm->stdin_ptr_l = (pTerm->stdin_ptr_l + n) % STDIN_BUFLEN;
      pTerm->stdin_inbuf -= n;
}

static int wait_key(PHB_GTELE pTerm, int milisec)
{
      int nKey, esc, n, i, ch, counter;
      keyTab *ptr;

      /*
#if defined(HB_OS_UNIX)
      if (s_WinSizeChangeFlag)
      {
            s_WinSizeChangeFlag = HB_FALSE;
            return K_RESIZE;
      }
#endif
*/

restart:
      counter = ++(pTerm->key_counter);
      nKey = esc = n = i = 0;
again:
      if ((nKey = getMouseKey(&pTerm->mLastEvt)) != 0)
            return nKey;

      ch = test_bufch(pTerm, i, pTerm->nTermMouseChars ? pTerm->esc_delay : milisec);
      if (counter != pTerm->key_counter)
            goto restart;

      if (ch >= 0 && ch <= 255)
      {
            ++i;
            if (pTerm->nTermMouseChars)
            {
                  pTerm->cTermMouseBuf[3 - pTerm->nTermMouseChars] = ch;
                  free_bufch(pTerm, i);
                  i = 0;
                  if (--pTerm->nTermMouseChars == 0)
                        set_tmevt(pTerm, pTerm->cTermMouseBuf, &pTerm->mLastEvt);
                  goto again;
            }

            nKey = ch;
            ptr = pTerm->pKeyTab;
            if (i == 1 && nKey == K_ESC && esc == 0)
            {
                  nKey = EXKEY_ESC;
                  esc = 1;
            }
            while (ch >= 0 && ch <= 255 && ptr != NULL)
            {
                  if (ptr->ch == ch)
                  {
                        if (ptr->key != K_UNDEF)
                        {
                              nKey = ptr->key;
                              switch (nKey)
                              {
                              case K_METAALT:
                                    pTerm->key_flag |= KEY_ALTMASK;
                                    break;
                              case K_METACTRL:
                                    pTerm->key_flag |= KEY_CTRLMASK;
                                    break;
                              case K_NATIONAL:
                                    pTerm->nation_mode = !pTerm->nation_mode;
                                    break;
                              case K_MOUSETERM:
                                    pTerm->nTermMouseChars = 3;
                                    break;
                              default:
                                    n = i;
                              }
                              if (n != i)
                              {
                                    free_bufch(pTerm, i);
                                    i = n = nKey = 0;
                                    if (esc == 2)
                                          break;
                                    esc = 0;
                                    goto again;
                              }
                        }
                        ptr = ptr->nextCh;
                        if (ptr)
                              if ((ch = test_bufch(pTerm, i, pTerm->esc_delay)) != -1)
                                    ++i;
                        if (counter != pTerm->key_counter)
                              goto restart;
                  }
                  else
                        ptr = ptr->otherCh;
            }
      }
      if (ch == -1 && pTerm->nTermMouseChars)
            pTerm->nTermMouseChars = 0;

      if (IS_CLIPKEY(ch))
            nKey = GET_CLIPKEY(ch);
      else if (HB_INKEY_ISEXT(ch))
            nKey = ch;
      else
      {
            if (esc == 1 && n == 0 && (ch != -1 || i >= 2))
            {
                  nKey = 0;
                  esc = 2;
                  i = n = 1;
                  goto again;
            }
            if (esc == 2)
            {
                  if (nKey != 0)
                        pTerm->key_flag |= KEY_ALTMASK;
                  else
                        nKey = EXKEY_ESC;
                  if (n == 1 && i > 1)
                        n = 2;
            }
            else
            {
                  if (nKey != 0 && (pTerm->key_flag & KEY_CTRLMASK) != 0 &&
                      (pTerm->key_flag & KEY_ALTMASK) != 0)
                  {
                        pTerm->key_flag &= ~(KEY_CTRLMASK | KEY_ALTMASK);
                        pTerm->key_flag |= KEY_SHIFTMASK;
                  }
                  if (n == 0 && i > 0)
                        n = 1;
            }

            if (n > 0)
                  free_bufch(pTerm, n);

            if (pTerm->key_flag != 0 && nKey != 0)
            {
                  nKey |= pTerm->key_flag;
                  pTerm->key_flag = 0;
            }

#ifdef HB_GT_UNICODE_BUF
            if (!pTerm->fUTF8)
            {
                  if (nKey != 0)
                  {
                        int u = HB_GTSELF_KEYTRANS(pTerm->pGT, nKey);
                        if (u)
                              return HB_INKEY_NEW_UNICODE(u);
                  }
            }
            else if (nKey >= 32 && nKey <= 255)
            {
                  HB_WCHAR wc = 0;
                  n = i = 0;
                  if (hb_cdpUTF8ToU16NextChar((HB_UCHAR)nKey, &n, &wc))
                  {
                        while (n > 0)
                        {
                              ch = test_bufch(pTerm, i++, pTerm->esc_delay);
                              if (ch < 0 || ch > 255)
                                    break;
                              if (!hb_cdpUTF8ToU16NextChar((HB_UCHAR)ch, &n, &wc))
                                    n = -1;
                        }
                        if (n == 0)
                        {
                              free_bufch(pTerm, i);
                              return HB_INKEY_NEW_UNICODE(wc);
                        }
                  }
            }
#else
            if (nKey >= 32 && nKey <= 255 && pTerm->fUTF8 && pTerm->cdpIn)
            {
                  HB_USHORT uc = 0;
                  n = i = 0;
                  if (hb_cdpGetFromUTF8(pTerm->cdpIn, (HB_UCHAR)nKey, &n, &uc))
                  {
                        while (n > 0)
                        {
                              ch = test_bufch(pTerm, i++, pTerm->esc_delay);
                              if (ch < 0 || ch > 255)
                                    break;
                              if (!hb_cdpGetFromUTF8(pTerm->cdpIn, ch, &n, &uc))
                                    n = -1;
                        }
                        if (n == 0)
                        {
                              free_bufch(pTerm, i);
                              nKey = uc;
                        }
                  }
            }

            /*
      if( pTerm->nation_transtbl && pTerm->nation_mode &&
           nKey >= 32 && nKey < 128 && pTerm->nation_transtbl[nKey] )
         nKey = pTerm->nation_transtbl[nKey];
 */
            if (nKey > 0 && nKey <= 255 && pTerm->keyTransTbl[nKey])
                  nKey = pTerm->keyTransTbl[nKey];
#endif
            if (nKey)
                  nKey = getClipKey(nKey);
      }

      //hb_gt_ele_termOut(pTerm, s_szInit, strlen(s_szInit));

      return nKey;
}

/* ************************************************************************* */

/*
 * LINUX terminal operations
 */
static void hb_gt_ele_LinuxSetTermMode(PHB_GTELE pTerm, int iAM)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_LinuxSetTermMode(%p,%d)", pTerm, iAM));

      if (iAM != pTerm->iAM)
      {
            if (iAM == 0)
                  hb_gt_ele_termOut(pTerm, "\x1B[m", 3);

            hb_gt_ele_termOut(pTerm, iAM ? "\x1B[?7h" : "\x1B[?7l", 5);
            pTerm->iAM = iAM;
      }
}

/*
static void hb_gt_ele_LinuxSetPalette(PHB_GTELE pTerm, int iIndexFrom, int iIndexTo)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_LinuxSetPalette(%p,%d,%d)", pTerm, iIndexFrom, iIndexTo));

      if (iIndexFrom < 0)
            iIndexFrom = 0;
      if (iIndexTo > 15)
            iIndexTo = 15;

      if (iIndexFrom <= iIndexTo)
      {
            do
            {
                  char szColor[11];
                  int iAnsiIndex = s_AnsiColors[iIndexFrom & 0x0F];

                  hb_snprintf(szColor, sizeof(szColor), "\x1b]P%X%02X%02X%02X",
                              iAnsiIndex,
                              (pTerm->colors[iIndexFrom]) & 0xff,
                              (pTerm->colors[iIndexFrom] >> 8) & 0xff,
                              (pTerm->colors[iIndexFrom] >> 16) & 0xff);
                  hb_gt_ele_termOut(pTerm, szColor, 10);
            } while (++iIndexFrom <= iIndexTo);

            // ESC ] is Operating System Command (OSC) which by default should
       // be terminated by ESC \ (ST). Some terminals which sets LINUX
       // TERM envvar but do not correctly understand above palette set
       // sequence may hang waiting for ST. We send ST below to avoid such
       // situation.
       // Linux console simply ignore ST terminator so nothing wrong
       // should happen.
       //
            hb_gt_ele_termOut(pTerm, "\x1b\\", 2);
      }
}

static void hb_gt_ele_LinuxResetPalette(PHB_GTELE pTerm)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_LinuxResetPalette(%p)", pTerm));

      hb_gt_ele_termOut(pTerm, "\x1b]R", 3);
}
*/

/*
 * XTERM terminal operations
 */
static HB_BOOL hb_gt_ele_XtermSetMode(PHB_GTELE pTerm, int *piRows, int *piCols)
{
      int iHeight, iWidth;
      char escseq[64];

      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_XtermSetMode(%p,%d,%d)", pTerm, *piRows, *piCols));

      pTerm->iMaxRows = *piRows;
      pTerm->iMaxCols = *piCols;

      HB_GTSELF_GETSIZE(pTerm->pGT, &iHeight, &iWidth);

      // CSI Ps ; Ps ; Ps t - Windows manipulation
      // 8 ; height ; width -> resize text area
      hb_snprintf(escseq, sizeof(escseq), "\x1b[8;%d;%dt", *piRows, *piCols);
      hb_gt_ele_termOut(pTerm, escseq, strlen(escseq));
      hb_gt_ele_termFlush(pTerm);

      /*
#if defined(HB_OS_UNIX)
      // dirty hack - wait for SIGWINCH
      if (*piRows != iHeight || *piCols != iWidth)
            sleep(3);
      if (s_WinSizeChangeFlag)
            s_WinSizeChangeFlag = HB_FALSE;
#endif
*/

      //hb_gt_ele_getSize(pTerm, piRows, piCols);

      return HB_TRUE;
}

static void hb_gt_ele_XtermSetAttributes(PHB_GTELE pTerm, int iAttr)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_XtermSetAttributes(%p,%d)", pTerm, iAttr));

      if (pTerm->iCurrentSGR != iAttr)
      {
            int i, acsc, bg, fg, bold, blink, rgb;
            char buff[64];

            i = 2;
            buff[0] = 0x1b;
            buff[1] = '[';

            acsc = (iAttr & HB_GTELE_ATTR_ACSC) && !pTerm->fUTF8 ? 1 : 0;
            if (pTerm->iExtColor == HB_GTELE_CLRSTD)
            {
                  bg = s_AnsiColors[(iAttr >> 4) & 0x07];
                  fg = s_AnsiColors[iAttr & 0x07];
                  bold = (iAttr & 0x08) ? 1 : 0;
                  blink = (iAttr & 0x80) ? 1 : 0;
            }
            else
            {
                  bg = s_AnsiColors[(iAttr >> 4) & 0x0F];
                  fg = s_AnsiColors[iAttr & 0x0F];
                  bold = blink = 0;
            }

            if (pTerm->iCurrentSGR == -1)
            {
                  buff[i++] = 'm';
                  buff[i++] = 0x1b;
                  buff[i++] = '(';
                  buff[i++] = acsc ? '0' : 'B';

                  buff[i++] = 0x1b;
                  buff[i++] = '[';

                  if (pTerm->iExtColor == HB_GTELE_CLRSTD)
                  {
                        if (bold)
                        {
                              buff[i++] = '1';
                              buff[i++] = ';';
                        }
                        if (blink)
                        {
                              buff[i++] = '5';
                              buff[i++] = ';';
                        }
                        buff[i++] = '3';
                        buff[i++] = '0' + fg;
                        buff[i++] = ';';
                        buff[i++] = '4';
                        buff[i++] = '0' + bg;
                  }
                  else if (pTerm->iExtColor == HB_GTELE_CLRX16)
                  {
                        /* ESC [ 38 ; 5 ; <fg> m */
                        buff[i++] = '3';
                        buff[i++] = '8';
                        buff[i++] = ';';
                        buff[i++] = '5';
                        buff[i++] = ';';
                        if (fg >= 10)
                              buff[i++] = '1';
                        buff[i++] = '0' + fg % 10;
                        buff[i++] = ';';
                        /* ESC [ 48 ; 5 ; <bg> m */
                        buff[i++] = '4';
                        buff[i++] = '8';
                        buff[i++] = ';';
                        buff[i++] = '5';
                        buff[i++] = ';';
                        if (bg >= 10)
                              buff[i++] = '1';
                        buff[i++] = '0' + bg % 10;
                  }
                  else if (pTerm->iExtColor == HB_GTELE_CLR256)
                  {
                        /* ESC [ 38 ; 5 ; <16 + 36 * r + 6 * g + b> m   0 <= r,g,b <= 5 */
                        rgb = pTerm->colors[iAttr & 0x0F];
                        rgb = 16 + 36 * ((rgb & 0xFF) / 43) +
                              6 * (((rgb >> 8) & 0xFF) / 43) +
                              (((rgb >> 16) & 0xFF) / 43);
                        i += hb_snprintf(buff + i, sizeof(buff) - i, "38;5;%d", rgb);
                        /* ESC [ 48 ; 5 ; <16 + 36 * r + 6 * g + b> m   0 <= r,g,b <= 5 */
                        rgb = pTerm->colors[(iAttr >> 4) & 0x0F];
                        rgb = 16 + 36 * ((rgb & 0xFF) / 43) +
                              6 * (((rgb >> 8) & 0xFF) / 43) +
                              (((rgb >> 16) & 0xFF) / 43);
                        i += hb_snprintf(buff + i, sizeof(buff) - i, ";48;5;%d", rgb);
                  }
                  else if (pTerm->iExtColor == HB_GTELE_CLRRGB)
                  {
                        /* ESC [ 38 ; 2 ; <r> ; <g> ; <b> m */
                        rgb = pTerm->colors[iAttr & 0x0F];
                        i += hb_snprintf(buff + i, sizeof(buff) - i, "38;2;%d;%d;%d",
                                         rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
                        /* ESC [ 48 ; 2 ; <r> ; <g> ; <b> m */
                        rgb = pTerm->colors[(iAttr >> 4) & 0x0F];
                        i += hb_snprintf(buff + i, sizeof(buff) - i, ";48;2;%d;%d;%d",
                                         rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
                  }
                  else if (pTerm->iExtColor == HB_GTELE_CLRAIX)
                  {
                        if (fg < 8)
                        {
                              buff[i++] = '3';
                              buff[i++] = '0' + fg;
                        }
                        else
                        {
                              buff[i++] = '9';
                              buff[i++] = '0' - 8 + fg;
                        }
                        buff[i++] = ';';
                        if (bg < 8)
                        {
                              buff[i++] = '4';
                              buff[i++] = '0' + bg;
                        }
                        else
                        {
                              buff[i++] = '1';
                              buff[i++] = '0';
                              buff[i++] = '0' - 8 + bg;
                        }
                  }
                  buff[i++] = 'm';
                  pTerm->iACSC = acsc;
                  pTerm->iBold = bold;
                  pTerm->iBlink = blink;
                  pTerm->iFgColor = fg;
                  pTerm->iBgColor = bg;
            }
            else
            {
                  if (pTerm->iBold != bold)
                  {
                        if (bold)
                              buff[i++] = '1';
                        else
                        {
                              buff[i++] = '2';
                              buff[i++] = '2';
                        }
                        buff[i++] = ';';
                        pTerm->iBold = bold;
                  }
                  if (pTerm->iBlink != blink)
                  {
                        if (!blink)
                              buff[i++] = '2';
                        buff[i++] = '5';
                        buff[i++] = ';';
                        pTerm->iBlink = blink;
                  }
                  if (pTerm->iFgColor != fg)
                  {
                        if (pTerm->iExtColor == HB_GTELE_CLRSTD)
                        {
                              buff[i++] = '3';
                              buff[i++] = '0' + fg;
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRX16)
                        {
                              /* ESC [ 38 ; 5 ; <fg> m */
                              buff[i++] = '3';
                              buff[i++] = '8';
                              buff[i++] = ';';
                              buff[i++] = '5';
                              buff[i++] = ';';
                              if (fg >= 10)
                                    buff[i++] = '1';
                              buff[i++] = '0' + fg % 10;
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLR256)
                        {
                              /* ESC [ 38 ; 5 ; <16 + 36 * r + 6 * g + b> m   0 <= r,g,b <= 5 */
                              rgb = pTerm->colors[iAttr & 0x0F];
                              rgb = 16 + 36 * ((rgb & 0xFF) / 43) +
                                    6 * (((rgb >> 8) & 0xFF) / 43) +
                                    (((rgb >> 16) & 0xFF) / 43);
                              i += hb_snprintf(buff + i, sizeof(buff) - i, "38;5;%d", rgb);
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRRGB)
                        {
                              /* ESC [ 38 ; 2 ; <r> ; <g> ; <b> m */
                              rgb = pTerm->colors[iAttr & 0x0F];
                              i += hb_snprintf(buff + i, sizeof(buff) - i, "38;2;%d;%d;%d",
                                               rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRAIX)
                        {
                              if (fg < 8)
                              {
                                    buff[i++] = '3';
                                    buff[i++] = '0' + fg;
                              }
                              else
                              {
                                    buff[i++] = '9';
                                    buff[i++] = '0' - 8 + fg;
                              }
                        }
                        buff[i++] = ';';
                        pTerm->iFgColor = fg;
                  }
                  if (pTerm->iBgColor != bg)
                  {
                        if (pTerm->iExtColor == HB_GTELE_CLRSTD)
                        {
                              buff[i++] = '4';
                              buff[i++] = '0' + bg;
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRX16)
                        {
                              /* ESC [ 48 ; 5 ; <fg> m */
                              buff[i++] = '4';
                              buff[i++] = '8';
                              buff[i++] = ';';
                              buff[i++] = '5';
                              buff[i++] = ';';
                              if (bg >= 10)
                                    buff[i++] = '1';
                              buff[i++] = '0' + bg % 10;
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLR256)
                        {
                              /* ESC [ 48 ; 5 ; <16 + 36 * r + 6 * g + b> m   0 <= r,g,b <= 5 */
                              rgb = pTerm->colors[(iAttr >> 4) & 0x0F];
                              rgb = 16 + 36 * ((rgb & 0xFF) / 43) +
                                    6 * (((rgb >> 8) & 0xFF) / 43) +
                                    (((rgb >> 16) & 0xFF) / 43);
                              i += hb_snprintf(buff + i, sizeof(buff) - i, "48;5;%d", rgb);
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRRGB)
                        {
                              /* ESC [ 48 ; 2 ; <r> ; <g> ; <b> m */
                              rgb = pTerm->colors[(iAttr >> 4) & 0x0F];
                              i += hb_snprintf(buff + i, sizeof(buff) - i, "48;2;%d;%d;%d",
                                               rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
                        }
                        else if (pTerm->iExtColor == HB_GTELE_CLRAIX)
                        {
                              if (bg < 8)
                              {
                                    buff[i++] = '4';
                                    buff[i++] = '0' + bg;
                              }
                              else
                              {
                                    buff[i++] = '1';
                                    buff[i++] = '0';
                                    buff[i++] = '0' - 8 + bg;
                              }
                        }
                        buff[i++] = ';';
                        pTerm->iBgColor = bg;
                  }
                  buff[i - 1] = 'm';
                  if (pTerm->iACSC != acsc)
                  {
                        if (i <= 2)
                              i = 0;
                        buff[i++] = 0x1b;
                        buff[i++] = '(';
                        buff[i++] = acsc ? '0' : 'B';
                        pTerm->iACSC = acsc;
                  }
            }
            pTerm->iCurrentSGR = iAttr;
            if (i > 2)
            {
                  hb_gt_ele_termOut(pTerm, buff, i);
            }
      }
}

static void hb_gt_ele_XtermSetTitle(PHB_GTELE pTerm, const char *szTitle)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_XtermSetTitle(%p,%s)", pTerm, szTitle));

      hb_gt_ele_termOut(pTerm, "\x1b]0;", 4);
      if (szTitle)
            hb_gt_ele_termOut(pTerm, szTitle, strlen(szTitle));
      hb_gt_ele_termOut(pTerm, "\x07", 1);
}

static HB_BOOL hb_gt_ele_AnsiGetCursorPos(PHB_GTELE pTerm, int *iRow, int *iCol,
                                          const char *szPost)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiGetCursorPos(%p,%p,%p,%s)", pTerm, iRow, iCol, szPost));

      //if (pTerm->fPosAnswer)
      //{
      char rdbuf[64];
      int i, j, n, d, y, x, z, q;
      HB_MAXUINT end_timer, cur_time;

      // CSI Ps n - device status report
      // Ps = 6 -> Report Cursor Position
      hb_gt_ele_termOut(pTerm, "\x1B[6n", 4);
      if (szPost)
            hb_gt_ele_termOut(pTerm, szPost, strlen(szPost));
      hb_gt_ele_termFlush(pTerm);

      n = j = x = y = 0;
      for (z = 0; z < 64; z++)
            rdbuf[z] = 0;
      //pTerm->fPosAnswer = HB_FALSE;

      /* wait up to 2 seconds for answer */
      end_timer = hb_dateMilliSeconds() + 2000;
      for (q=1; q++; q<10)
      {
            /* loking for cursor position in "\x1b[%d;%dR" */
            while (j < n && rdbuf[j] != '\x1b')
            {
                  HB_TRACE(HB_TR_DEBUG, ("GTELE AnsiGetCursorPos while rdbuf: j=%d  char=%d", j, rdbuf[j]));
                  ++j;
            }
            HB_TRACE(HB_TR_DEBUG, ("GTELE AnsiGetCursorPos after while rdbuf: j=%d  char=%d", j, rdbuf[j]));

            for (z = j; z < 64; z++)
            {
                  HB_TRACE(HB_TR_DEBUG, ("char %d=0x%02X '%c'", z, rdbuf[z], rdbuf[z]));
            }

            if (n - j >= 6)
            {
                  i = j + 1;
                  if (rdbuf[i] == '[')
                  {
                        y = 0;
                        d = ++i;
                        while (i < n && rdbuf[i] >= '0' && rdbuf[i] <= '9')
                              y = y * 10 + (rdbuf[i++] - '0');
                        if (i < n && i > d && rdbuf[i] == ';')
                        {
                              x = 0;
                              d = ++i;
                              while (i < n && rdbuf[i] >= '0' && rdbuf[i] <= '9')
                                    x = x * 10 + (rdbuf[i++] - '0');
                              if (i < n && i > d && rdbuf[i] == 'R')
                              {
                                    if (szPost)
                                    {
                                          while (j >= 5)
                                          {
                                                --j;
                                          }
                                    }
                                    //pTerm->fPosAnswer = HB_TRUE;
                                    break;
                              }
                        }
                  }
                  if (i < n)
                  {
                        j = i;
                        continue;
                  }
            }
            if (n == sizeof(rdbuf))
                  break;
            cur_time = hb_dateMilliSeconds();
            if (cur_time > end_timer)
                  break;
            else
            {
                  /*
                        struct timeval tv;
                        fd_set rdfds;
                        int iMilliSec;

                        FD_ZERO(&rdfds);
                        FD_SET(pTerm->hFilenoStdin, &rdfds);
                        iMilliSec = (int)(end_timer - cur_time);
                        tv.tv_sec = iMilliSec / 1000;
                        tv.tv_usec = (iMilliSec % 1000) * 1000;

                        if (select(pTerm->hFilenoStdin + 1, &rdfds, NULL, NULL, &tv) <= 0)
                              break;
                        */
                  // unix read out
                  //i = read(pTerm->hFilenoStdin, rdbuf + n, sizeof(rdbuf) - n);

                  i = hb_fsRead(pTerm->hFilenoStdin, rdbuf + n, sizeof(rdbuf) - n);

                  HB_TRACE(HB_TR_DEBUG, ("GTELE AnsiGetCursorPos feedback rdbuf: size=%d, i=%d", sizeof(rdbuf), i));

                  if (i <= 0)
                        break;
                  n += i;
            }
      }

      //if (pTerm->fPosAnswer)
      //{
      *iRow = y - 1;
      *iCol = x - 1;
      HB_TRACE(HB_TR_DEBUG, ("HERNAD GTELE ROWPOS: %d, %d", *iRow, *iCol));
      //}
      //else
      //{
      //      *iRow = *iCol = -1;
      //      HB_TRACE(HB_TR_DEBUG, ("GTELE ROWPOS ERROR!", *iRow, *iCol);
      //}
      //}
      
      //return pTerm->fPosAnswer;
      return HB_TRUE;
}

static void hb_gt_ele_AnsiSetCursorPos(PHB_GTELE pTerm, int iRow, int iCol)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiSetCursorPos(%p,%d,%d)", pTerm, iRow, iCol));

      if (pTerm->iRow != iRow || pTerm->iCol != iCol) // current position changed
      {
            char buff[16];

            //CSI Ps ; Ps H
            // Cursor Position [row;column] (default = [1,1]) (CUP).
            hb_snprintf(buff, sizeof(buff), "\x1B[%d;%dH", iRow + 1, iCol + 1);
            hb_gt_ele_termOut(pTerm, buff, strlen(buff));
            pTerm->iRow = iRow;
            pTerm->iCol = iCol;
      }
}

static void hb_gt_ele_AnsiSetCursorStyle(PHB_GTELE pTerm, int iStyle)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiSetCursorStyle(%p,%d)", pTerm, iStyle));

      if (pTerm->iCursorStyle == iStyle)
            return;

      // CSI ? Pm h
      //    Ps = 2 5  -> Show Cursor (DECTCEM).

      // CSI ? Pm l
      //    Ps = 2 5  -> Hide Cursor (DECTCEM).
      hb_gt_ele_termOut(pTerm, iStyle == SC_NONE ? "\x1B[?25l" : "\x1B[?25h", 6);
      pTerm->iCursorStyle = iStyle;
}

static void hb_gt_ele_AnsiSetAttributes(PHB_GTELE pTerm, int iAttr)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiSetAttributes(%p,%d)", pTerm, iAttr));

      if (pTerm->iCurrentSGR == iAttr) // ansi attribute - no change
            return;

      int i, acsc, bg, fg, bold, blink;
      char buff[32];

      i = 2;
      buff[0] = 0x1b;
      buff[1] = '[';

      acsc = (iAttr & HB_GTELE_ATTR_ACSC) ? 1 : 0;
      bg = s_AnsiColors[(iAttr >> 4) & 0x07];
      fg = s_AnsiColors[iAttr & 0x07];
      bold = (iAttr & 0x08) ? 1 : 0;
      blink = (iAttr & 0x80) ? 1 : 0;

      if (pTerm->iCurrentSGR == -1)
      {
            buff[i++] = '0';
            buff[i++] = ';';
            buff[i++] = '1';
            buff[i++] = acsc ? '1' : '0';
            buff[i++] = ';';
            if (bold)
            {
                  buff[i++] = '1';
                  buff[i++] = ';';
            }
            if (blink)
            {
                  buff[i++] = '5';
                  buff[i++] = ';';
            }
            buff[i++] = '3';
            buff[i++] = '0' + fg;
            buff[i++] = ';';
            buff[i++] = '4';
            buff[i++] = '0' + bg;
            buff[i++] = 'm';
            pTerm->iACSC = acsc;
            pTerm->iBold = bold;
            pTerm->iBlink = blink;
            pTerm->iFgColor = fg;
            pTerm->iBgColor = bg;
      }
      else
      {
            if (pTerm->iACSC != acsc)
            {
                  buff[i++] = '1';
                  buff[i++] = acsc ? '1' : '0';
                  buff[i++] = ';';
                  pTerm->iACSC = acsc;
            }
            if (pTerm->iBold != bold)
            {
                  if (bold)
                        buff[i++] = '1';
                  else
                  {
                        buff[i++] = '2';
                        buff[i++] = '2';
                  }
                  buff[i++] = ';';
                  pTerm->iBold = bold;
            }
            if (pTerm->iBlink != blink)
            {
                  if (!blink)
                        buff[i++] = '2';
                  buff[i++] = '5';
                  buff[i++] = ';';
                  pTerm->iBlink = blink;
            }
            if (pTerm->iFgColor != fg)
            {
                  buff[i++] = '3';
                  buff[i++] = '0' + fg;
                  buff[i++] = ';';
                  pTerm->iFgColor = fg;
            }
            if (pTerm->iBgColor != bg)
            {
                  buff[i++] = '4';
                  buff[i++] = '0' + bg;
                  buff[i++] = ';';
                  pTerm->iBgColor = bg;
            }
            buff[i - 1] = 'm';
      }
      pTerm->iCurrentSGR = iAttr;
      if (i > 2)
      {
            hb_gt_ele_termOut(pTerm, buff, i);
      }
}

static int hb_gt_ele_AnsiGetAcsc(PHB_GTELE pTerm, unsigned char c)
{
      const unsigned char *ptr;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiGetAcsc(%p,%d)", pTerm, c));

      for (ptr = (const unsigned char *)pTerm->szAcsc; *ptr && *(ptr + 1); ptr += 2)
      {
            if (*ptr == c)
                  return *(ptr + 1) | HB_GTELE_ATTR_ACSC;
      }

      switch (c)
      {
      case '.':
            return 'v' | HB_GTELE_ATTR_STD;
      case ',':
            return '<' | HB_GTELE_ATTR_STD;
      case '+':
            return '>' | HB_GTELE_ATTR_STD;
      case '-':
            return '^' | HB_GTELE_ATTR_STD;
      case 'a':
            return '#' | HB_GTELE_ATTR_STD;
      case '0':
      case 'h':
            return hb_gt_ele_AnsiGetAcsc(pTerm, 'a');
      }

      return c | HB_GTELE_ATTR_ALT;
}

static void hb_gt_ele_AnsiBell(PHB_GTELE pTerm)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiBell(%p)", pTerm));

      hb_gt_ele_termOut(pTerm, s_szBell, 1);
      hb_gt_ele_termFlush(pTerm);
}

static void hb_gt_ele_AnsiTone(PHB_GTELE pTerm, double dFrequency, double dDuration)
{
      double dCurrentSeconds;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiTone(%p,%lf,%lf)", pTerm, dFrequency, dDuration));

      // but throttle to max once per second, in case of sound
      // effects prgs calling lots of short tone sequences in
      // succession leading to BEL hell on the terminal
      // Output an ASCII BEL character to cause a sound

      dCurrentSeconds = hb_dateSeconds();
      if (dCurrentSeconds < pTerm->dToneSeconds ||
          dCurrentSeconds - pTerm->dToneSeconds > 0.5)
      {
            hb_gt_ele_AnsiBell(pTerm);
            pTerm->dToneSeconds = dCurrentSeconds;
      }

      HB_SYMBOL_UNUSED(dFrequency);

      //convert Clipper (DOS) timer tick units to seconds
      hb_idleSleep(dDuration / 18.2);
}

static void hb_gt_ele_AnsiInit(PHB_GTELE pTerm)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiInit(%p)", pTerm));

      pTerm->iCurrentSGR = pTerm->iRow = pTerm->iCol =
          pTerm->iCursorStyle = pTerm->iACSC = pTerm->iAM = -1;
}

static void hb_gt_ele_AnsiExit(PHB_GTELE pTerm)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_AnsiExit(%p)", pTerm));

      /* set default color */
      pTerm->SetAttributes(pTerm, 0x07 & pTerm->iAttrMask);
      pTerm->SetCursorStyle(pTerm, SC_NORMAL);
      pTerm->SetTermMode(pTerm, 1);
      hb_gt_ele_termOut(pTerm, "\x1B[m", 3);
}

/* ************************************************************************* */

/*
 * common functions
 */
static HB_BOOL hb_ele_Param(const char *pszParam, int *piValue)
{
      HB_BOOL fResult = HB_FALSE;
      char *pszGtEleParams = hb_cmdargString("GTELE");

      if (pszGtEleParams)
      {
            const char *pszAt = strstr(hb_strupr(pszGtEleParams), pszParam);

            if (pszAt != NULL)
            {
                  fResult = HB_TRUE;
                  if (piValue)
                  {
                        int iOverflow;

                        pszAt += strlen(pszParam);
                        if (*pszAt == '=' || *pszAt == ':')
                              ++pszAt;
                        *piValue = HB_ISDIGIT(*pszAt) ? hb_strValInt(pszAt, &iOverflow) : 1;
                  }
            }
            hb_xfree(pszGtEleParams);
      }

      return fResult;
}

static HB_BOOL hb_trm_isUTF8(PHB_GTELE pTerm)
{
      HB_BOOL fUTF8 = HB_FALSE;
      char *szLang;

      //if (pTerm->fPosAnswer)
      //{
      hb_gt_ele_termOut(pTerm, "\005\r\303\255", 4);
      if (pTerm->GetCursorPos(pTerm, &pTerm->iRow, &pTerm->iCol, "\r   \r"))
      {
            fUTF8 = pTerm->iCol == 1;
            pTerm->iCol = 0;
            return fUTF8;
      }
      //}

/*
      if (hb_ele_Param("UTF8", NULL) || hb_ele_Param("UTF-8", NULL))
      {
            HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_utf8 %d", HB_TRUE));
            return HB_TRUE;
      }
      else if (hb_ele_Param("ISO", NULL))
      {
            HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_utf8 %d", HB_FALSE));
            return HB_FALSE;
      }
      else if (pTerm->fPosAnswer)
      {
            HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_utf8 %d", fUTF8));
            return fUTF8;
      }
*/
      szLang = getenv("LANG");
      if (szLang && strstr(szLang, "UTF-8") != NULL)
      {
            HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_utf8 %d", HB_TRUE));
            return HB_TRUE;
      }

      //#ifdef IUTF8
      //   if( ( pTerm->curr_TIO.c_iflag & IUTF8 ) != 0 )
      //      return HB_TRUE;
      //#endif

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_utf8 %d", HB_FALSE));
      return HB_FALSE;
}

static void hb_gt_ele_PutStr(PHB_GTELE pTerm, int iRow, int iCol, int iAttr, const char *pStr, int iLen, int iChars)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_PutStr(%p,%d,%d,%d,%p,%d,%d)", pTerm, iRow, iCol, iAttr, pStr, iLen, iChars));

      //if (pTerm->iOutBufSize)
      //{
      pTerm->SetCursorPos(pTerm, iRow, iCol);
      pTerm->SetAttributes(pTerm, iAttr & pTerm->iAttrMask);
      //#ifdef HB_GT_UNICODE_BUF
            hb_gt_ele_termOut(pTerm, pStr, iLen);
      //#else
      //hb_gt_ele_termOutTrans(pTerm, pStr, iLen, iAttr);
      //#endif
      //}

      pTerm->iCol += iChars;
}

static void hb_gt_ele_SetPalette(PHB_GTELE pTerm, int iIndexFrom, int iIndexTo)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetPalette(%p,%d,%d)", pTerm, iIndexFrom, iIndexTo));
}

static void hb_gt_ele_ResetPalette(PHB_GTELE pTerm)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_ResetPalette(%p)", pTerm));
}

static void hb_gt_ele_SetTitle(PHB_GTELE pTerm, const char *szTitle)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetTitle(%p,%s)", pTerm, szTitle));

      hb_gt_ele_XtermSetTitle(pTerm, szTitle);
}

#ifndef HB_GT_UNICODE_BUF
static void hb_gt_ele_SetKeyTrans(PHB_GTELE pTerm)
{
      PHB_CODEPAGE cdpTerm = HB_GTSELF_INCP(pTerm->pGT),
                   cdpHost = HB_GTSELF_HOSTCP(pTerm->pGT);
      int i;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetKeyTrans(%p,%p,%p)", pTerm, cdpTerm, cdpHost));

      for (i = 0; i < 256; ++i)
            pTerm->keyTransTbl[i] = (unsigned char)
                hb_cdpTranslateChar(i, cdpTerm, cdpHost);
}
#endif

static void hb_gt_ele_SetDispTrans(PHB_GTELE pTerm, int box)
{
      PHB_CODEPAGE cdpTerm = HB_GTSELF_TERMCP(pTerm->pGT),
                   cdpHost = HB_GTSELF_HOSTCP(pTerm->pGT);
      int i;

      memset(pTerm->chrattr, 0, sizeof(pTerm->chrattr));
      memset(pTerm->boxattr, 0, sizeof(pTerm->boxattr));

      for (i = 0; i < 256; i++)
      {
            int ch = pTerm->charmap[i] & 0xffff;
            int mode = !pTerm->fUTF8 ? (pTerm->charmap[i] >> 16) & 0xff : 1;

            switch (mode)
            {
            case 1:
                  pTerm->chrattr[i] = pTerm->boxattr[i] = HB_GTELE_ATTR_STD;
                  break;
            case 2:
                  pTerm->chrattr[i] = pTerm->boxattr[i] = HB_GTELE_ATTR_ALT;
                  break;
            case 3:
                  pTerm->chrattr[i] = pTerm->boxattr[i] = HB_GTELE_ATTR_PROT;
                  break;
            case 4:
                  pTerm->chrattr[i] = pTerm->boxattr[i] = HB_GTELE_ATTR_ALT | HB_GTELE_ATTR_PROT;
                  break;
            case 5:
                  ch = pTerm->GetAcsc(pTerm, ch & 0xff);
                  pTerm->chrattr[i] = pTerm->boxattr[i] = ch & ~HB_GTELE_ATTR_CHAR;
                  break;
            case 0:
            default:
                  pTerm->chrattr[i] = HB_GTELE_ATTR_STD;
                  pTerm->boxattr[i] = HB_GTELE_ATTR_ALT;
                  break;
            }
            pTerm->chrattr[i] |= ch;
            pTerm->boxattr[i] |= ch;
      }

      if (cdpHost && cdpTerm)
      {
            for (i = 0; i < 256; ++i)
            {
                  if (hb_cdpIsAlpha(cdpHost, i))
                  {
                        unsigned char uc = (unsigned char)
                            hb_cdpTranslateDispChar(i, cdpHost, cdpTerm);

                        pTerm->chrattr[i] = uc | HB_GTELE_ATTR_STD;
                        if (box)
                              pTerm->boxattr[i] = uc | HB_GTELE_ATTR_STD;
                  }
            }
      }
}

static int addKeyMap(PHB_GTELE pTerm, int nKey, const char *cdesc)
{
      int ret = K_UNDEF, i = 0, c;
      keyTab **ptr;

      if (cdesc == NULL)
            return ret;

      c = (unsigned char)cdesc[i++];
      ptr = &pTerm->pKeyTab;

      while (c)
      {
            if (*ptr == NULL)
            {
                  *ptr = (keyTab *)hb_xgrab(sizeof(keyTab));
                  (*ptr)->ch = c;
                  (*ptr)->key = K_UNDEF;
                  (*ptr)->nextCh = NULL;
                  (*ptr)->otherCh = NULL;
            }
            if ((*ptr)->ch == c)
            {
                  c = (unsigned char)cdesc[i++];
                  if (c)
                        ptr = &((*ptr)->nextCh);
                  else
                  {
                        ret = (*ptr)->key;
                        (*ptr)->key = nKey;
                  }
            }
            else
                  ptr = &((*ptr)->otherCh);
      }
      return ret;
}

static int removeKeyMap(PHB_GTELE pTerm, const char *cdesc)
{
      int ret = K_UNDEF, i = 0, c;
      keyTab **ptr;

      c = (unsigned char)cdesc[i++];
      ptr = &pTerm->pKeyTab;

      while (c && *ptr != NULL)
      {
            if ((*ptr)->ch == c)
            {
                  c = (unsigned char)cdesc[i++];
                  if (!c)
                  {
                        ret = (*ptr)->key;
                        (*ptr)->key = K_UNDEF;
                        if ((*ptr)->nextCh == NULL && (*ptr)->otherCh == NULL)
                        {
                              hb_xfree(*ptr);
                              *ptr = NULL;
                        }
                  }
                  else
                        ptr = &((*ptr)->nextCh);
            }
            else
                  ptr = &((*ptr)->otherCh);
      }
      return ret;
}

static void removeAllKeyMap(PHB_GTELE pTerm, keyTab **ptr)
{
      if ((*ptr)->nextCh != NULL)
            removeAllKeyMap(pTerm, &((*ptr)->nextCh));
      if ((*ptr)->otherCh != NULL)
            removeAllKeyMap(pTerm, &((*ptr)->otherCh));

      hb_xfree(*ptr);
      *ptr = NULL;
}

static void addKeyTab(PHB_GTELE pTerm, const keySeq *keys)
{
      while (keys->key)
      {
            addKeyMap(pTerm, keys->key, keys->seq);
            ++keys;
      }
}

static void init_keys(PHB_GTELE pTerm)
{

      static const keySeq stdKeySeq[] = {
          /* virual CTRL/ALT sequences */
          {K_METACTRL, CTRL_SEQ},
          {K_METAALT, ALT_SEQ},
#ifdef NATION_SEQ
          /* national mode key sequences */
          {K_NATIONAL, NATION_SEQ},
#endif
          {EXKEY_ENTER, "\r"},
          /* terminal mouse event */
          {K_MOUSETERM, "\x1b[M"},
          {0, NULL}};

      static const keySeq stdFnKeySeq[] = {

          {EXKEY_F1, "\x1b[11~"}, /* kf1  */
          {EXKEY_F2, "\x1b[12~"}, /* kf2  */
          {EXKEY_F3, "\x1b[13~"}, /* kf3  */
          {EXKEY_F4, "\x1b[14~"}, /* kf4  */
          {EXKEY_F5, "\x1b[15~"}, /* kf5  */

          {EXKEY_F6, "\x1b[17~"},  /* kf6  */
          {EXKEY_F7, "\x1b[18~"},  /* kf7  */
          {EXKEY_F8, "\x1b[19~"},  /* kf8  */
          {EXKEY_F9, "\x1b[20~"},  /* kf9  */
          {EXKEY_F10, "\x1b[21~"}, /* kf10 */
          {EXKEY_F11, "\x1b[23~"}, /* kf11 */
          {EXKEY_F12, "\x1b[24~"}, /* kf12 */

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1b[25~"},  /* kf13 */
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1b[26~"},  /* kf14 */
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1b[28~"},  /* kf15 */
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1b[29~"},  /* kf16 */
          {EXKEY_F5 | KEY_SHIFTMASK, "\x1b[31~"},  /* kf17 */
          {EXKEY_F6 | KEY_SHIFTMASK, "\x1b[32~"},  /* kf18 */
          {EXKEY_F7 | KEY_SHIFTMASK, "\x1b[33~"},  /* kf19 */
          {EXKEY_F8 | KEY_SHIFTMASK, "\x1b[34~"},  /* kf20 */
          {EXKEY_F9 | KEY_SHIFTMASK, "\x1b[35~"},  /* kf21 */
          {EXKEY_F10 | KEY_SHIFTMASK, "\x1b[36~"}, /* kf22 */
          {EXKEY_F11 | KEY_SHIFTMASK, "\x1b[37~"}, /* kf23 */
          {EXKEY_F12 | KEY_SHIFTMASK, "\x1b[38~"}, /* kf24 */

          {EXKEY_F1 | KEY_CTRLMASK, "\x1b[39~"},  /* kf25 */
          {EXKEY_F2 | KEY_CTRLMASK, "\x1b[40~"},  /* kf26 */
          {EXKEY_F3 | KEY_CTRLMASK, "\x1b[41~"},  /* kf27 */
          {EXKEY_F4 | KEY_CTRLMASK, "\x1b[42~"},  /* kf28 */
          {EXKEY_F5 | KEY_CTRLMASK, "\x1b[43~"},  /* kf29 */
          {EXKEY_F6 | KEY_CTRLMASK, "\x1b[44~"},  /* kf30 */
          {EXKEY_F7 | KEY_CTRLMASK, "\x1b[45~"},  /* kf31 */
          {EXKEY_F8 | KEY_CTRLMASK, "\x1b[46~"},  /* kf32 */
          {EXKEY_F9 | KEY_CTRLMASK, "\x1b[47~"},  /* kf33 */
          {EXKEY_F10 | KEY_CTRLMASK, "\x1b[48~"}, /* kf34 */
          {EXKEY_F11 | KEY_CTRLMASK, "\x1b[49~"}, /* kf35 */
          {EXKEY_F12 | KEY_CTRLMASK, "\x1b[50~"}, /* kf36 */

          {EXKEY_F1 | KEY_ALTMASK, "\x1b[51~"},  /* kf37 */
          {EXKEY_F2 | KEY_ALTMASK, "\x1b[52~"},  /* kf38 */
          {EXKEY_F3 | KEY_ALTMASK, "\x1b[53~"},  /* kf39 */
          {EXKEY_F4 | KEY_ALTMASK, "\x1b[54~"},  /* kf40 */
          {EXKEY_F5 | KEY_ALTMASK, "\x1b[55~"},  /* kf41 */
          {EXKEY_F6 | KEY_ALTMASK, "\x1b[56~"},  /* kf42 */
          {EXKEY_F7 | KEY_ALTMASK, "\x1b[57~"},  /* kf43 */
          {EXKEY_F8 | KEY_ALTMASK, "\x1b[58~"},  /* kf44 */
          {EXKEY_F9 | KEY_ALTMASK, "\x1b[59~"},  /* kf45 */
          {EXKEY_F10 | KEY_ALTMASK, "\x1b[70~"}, /* kf46 */
          {EXKEY_F11 | KEY_ALTMASK, "\x1b[71~"}, /* kf47 */
          {EXKEY_F12 | KEY_ALTMASK, "\x1b[72~"}, /* kf48 */

          {0, NULL}};

      static const keySeq stdCursorKeySeq[] = {
          {EXKEY_HOME, "\x1b[1~"}, /* khome */
          {EXKEY_INS, "\x1b[2~"},  /* kich1 */
          {EXKEY_DEL, "\x1b[3~"},  /* kdch1 */
          {EXKEY_END, "\x1b[4~"},  /* kend  */
          {EXKEY_PGUP, "\x1b[5~"}, /* kpp   */
          {EXKEY_PGDN, "\x1b[6~"}, /* knp   */

          {0, NULL}};

      static const keySeq xtermModKeySeq[] = {
          /* XTerm  with modifiers */
          {EXKEY_F1 | KEY_CTRLMASK, "\x1bO5P"},
          {EXKEY_F2 | KEY_CTRLMASK, "\x1bO5Q"},
          {EXKEY_F3 | KEY_CTRLMASK, "\x1bO5R"},
          {EXKEY_F4 | KEY_CTRLMASK, "\x1bO5S"},

          {EXKEY_F1 | KEY_CTRLMASK, "\x1b[11;5~"},
          {EXKEY_F2 | KEY_CTRLMASK, "\x1b[12;5~"},
          {EXKEY_F3 | KEY_CTRLMASK, "\x1b[13;5~"},
          {EXKEY_F4 | KEY_CTRLMASK, "\x1b[14;5~"},
          {EXKEY_F5 | KEY_CTRLMASK, "\x1b[15;5~"},
          {EXKEY_F6 | KEY_CTRLMASK, "\x1b[17;5~"},
          {EXKEY_F7 | KEY_CTRLMASK, "\x1b[18;5~"},
          {EXKEY_F8 | KEY_CTRLMASK, "\x1b[19;5~"},
          {EXKEY_F9 | KEY_CTRLMASK, "\x1b[20;5~"},
          {EXKEY_F10 | KEY_CTRLMASK, "\x1b[21;5~"},
          {EXKEY_F11 | KEY_CTRLMASK, "\x1b[23;5~"},
          {EXKEY_F12 | KEY_CTRLMASK, "\x1b[24;5~"},

          {EXKEY_HOME | KEY_CTRLMASK, "\x1b[1;5~"},
          {EXKEY_INS | KEY_CTRLMASK, "\x1b[2;5~"},
          {EXKEY_DEL | KEY_CTRLMASK, "\x1b[3;5~"},
          {EXKEY_END | KEY_CTRLMASK, "\x1b[4;5~"},
          {EXKEY_PGUP | KEY_CTRLMASK, "\x1b[5;5~"},
          {EXKEY_PGDN | KEY_CTRLMASK, "\x1b[6;5~"},

          {EXKEY_UP | KEY_CTRLMASK, "\x1b[1;5A"},
          {EXKEY_DOWN | KEY_CTRLMASK, "\x1b[1;5B"},
          {EXKEY_RIGHT | KEY_CTRLMASK, "\x1b[1;5C"},
          {EXKEY_LEFT | KEY_CTRLMASK, "\x1b[1;5D"},
          {EXKEY_CENTER | KEY_CTRLMASK, "\x1b[1;5E"},
          {EXKEY_END | KEY_CTRLMASK, "\x1b[1;5F"},
          {EXKEY_CENTER | KEY_CTRLMASK, "\x1b[1;5G"},
          {EXKEY_HOME | KEY_CTRLMASK, "\x1b[1;5H"},

          {EXKEY_UP | KEY_CTRLMASK, "\x1b[5A"},
          {EXKEY_DOWN | KEY_CTRLMASK, "\x1b[5B"},
          {EXKEY_RIGHT | KEY_CTRLMASK, "\x1b[5C"},
          {EXKEY_LEFT | KEY_CTRLMASK, "\x1b[5D"},
          {EXKEY_CENTER | KEY_CTRLMASK, "\x1b[5E"}, /* --- */
          {EXKEY_END | KEY_CTRLMASK, "\x1b[5F"},    /* --- */
          {EXKEY_CENTER | KEY_CTRLMASK, "\x1b[5G"}, /* --- */
          {EXKEY_HOME | KEY_CTRLMASK, "\x1b[5H"},   /* --- */

          {EXKEY_F1 | KEY_ALTMASK, "\x1bO3P"},
          {EXKEY_F2 | KEY_ALTMASK, "\x1bO3Q"},
          {EXKEY_F3 | KEY_ALTMASK, "\x1bO3R"},
          {EXKEY_F4 | KEY_ALTMASK, "\x1bO3S"},

          {EXKEY_F1 | KEY_ALTMASK, "\x1b[11;3~"},
          {EXKEY_F2 | KEY_ALTMASK, "\x1b[12;3~"},
          {EXKEY_F3 | KEY_ALTMASK, "\x1b[13;3~"},
          {EXKEY_F4 | KEY_ALTMASK, "\x1b[14;3~"},
          {EXKEY_F5 | KEY_ALTMASK, "\x1b[15;3~"},
          {EXKEY_F6 | KEY_ALTMASK, "\x1b[17;3~"},
          {EXKEY_F7 | KEY_ALTMASK, "\x1b[18;3~"},
          {EXKEY_F8 | KEY_ALTMASK, "\x1b[19;3~"},
          {EXKEY_F9 | KEY_ALTMASK, "\x1b[20;3~"},
          {EXKEY_F10 | KEY_ALTMASK, "\x1b[21;3~"},
          {EXKEY_F11 | KEY_ALTMASK, "\x1b[23;3~"},
          {EXKEY_F12 | KEY_ALTMASK, "\x1b[24;3~"},

          {EXKEY_HOME | KEY_ALTMASK, "\x1b[1;3~"},
          {EXKEY_INS | KEY_ALTMASK, "\x1b[2;3~"},
          {EXKEY_DEL | KEY_ALTMASK, "\x1b[3;3~"},
          {EXKEY_END | KEY_ALTMASK, "\x1b[4;3~"},
          {EXKEY_PGUP | KEY_ALTMASK, "\x1b[5;3~"},
          {EXKEY_PGDN | KEY_ALTMASK, "\x1b[6;3~"},

          {EXKEY_UP | KEY_ALTMASK, "\x1b[1;3A"},
          {EXKEY_DOWN | KEY_ALTMASK, "\x1b[1;3B"},
          {EXKEY_RIGHT | KEY_ALTMASK, "\x1b[1;3C"},
          {EXKEY_LEFT | KEY_ALTMASK, "\x1b[1;3D"},
          {EXKEY_CENTER | KEY_ALTMASK, "\x1b[1;3E"},
          {EXKEY_END | KEY_ALTMASK, "\x1b[1;3F"},
          {EXKEY_CENTER | KEY_ALTMASK, "\x1b[1;3G"},
          {EXKEY_HOME | KEY_ALTMASK, "\x1b[1;3H"},

          {EXKEY_UP | KEY_ALTMASK, "\x1b[3A"},
          {EXKEY_DOWN | KEY_ALTMASK, "\x1b[3B"},
          {EXKEY_RIGHT | KEY_ALTMASK, "\x1b[3C"},
          {EXKEY_LEFT | KEY_ALTMASK, "\x1b[3D"},
          {EXKEY_CENTER | KEY_ALTMASK, "\x1b[3E"}, /* --- */
          {EXKEY_END | KEY_ALTMASK, "\x1b[3F"},    /* --- */
          {EXKEY_CENTER | KEY_ALTMASK, "\x1b[3G"}, /* --- */
          {EXKEY_HOME | KEY_ALTMASK, "\x1b[3H"},   /* --- */

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1bO2P"},
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1bO2Q"},
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1bO2R"},
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1bO2S"},

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1bO1;2P"},
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1bO1;2Q"},
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1bO1;2R"},
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1bO1;2S"},

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1b[1;2P"},
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1b[1;2Q"},
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1b[1;2R"},
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1b[1;2S"},

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1b[11;2~"},
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1b[12;2~"},
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1b[13;2~"},
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1b[14;2~"},
          {EXKEY_F5 | KEY_SHIFTMASK, "\x1b[15;2~"},
          {EXKEY_F6 | KEY_SHIFTMASK, "\x1b[17;2~"},
          {EXKEY_F7 | KEY_SHIFTMASK, "\x1b[18;2~"},
          {EXKEY_F8 | KEY_SHIFTMASK, "\x1b[19;2~"},
          {EXKEY_F9 | KEY_SHIFTMASK, "\x1b[20;2~"},
          {EXKEY_F10 | KEY_SHIFTMASK, "\x1b[21;2~"},
          {EXKEY_F11 | KEY_SHIFTMASK, "\x1b[23;2~"},
          {EXKEY_F12 | KEY_SHIFTMASK, "\x1b[24;2~"},

          {EXKEY_HOME | KEY_SHIFTMASK, "\x1b[1;2~"},
          {EXKEY_INS | KEY_SHIFTMASK, "\x1b[2;2~"},
          {EXKEY_DEL | KEY_SHIFTMASK, "\x1b[3;2~"},
          {EXKEY_END | KEY_SHIFTMASK, "\x1b[4;2~"},
          {EXKEY_PGUP | KEY_SHIFTMASK, "\x1b[5;2~"},
          {EXKEY_PGDN | KEY_SHIFTMASK, "\x1b[6;2~"},

          {EXKEY_UP | KEY_SHIFTMASK, "\x1b[1;2A"},
          {EXKEY_DOWN | KEY_SHIFTMASK, "\x1b[1;2B"},
          {EXKEY_RIGHT | KEY_SHIFTMASK, "\x1b[1;2C"},
          {EXKEY_LEFT | KEY_SHIFTMASK, "\x1b[1;2D"},
          {EXKEY_CENTER | KEY_SHIFTMASK, "\x1b[1;2E"},
          {EXKEY_END | KEY_SHIFTMASK, "\x1b[1;2F"},
          {EXKEY_CENTER | KEY_SHIFTMASK, "\x1b[1;2G"},
          {EXKEY_HOME | KEY_SHIFTMASK, "\x1b[1;2H"},

          {EXKEY_UP | KEY_SHIFTMASK, "\x1b[2A"},
          {EXKEY_DOWN | KEY_SHIFTMASK, "\x1b[2B"},
          {EXKEY_RIGHT | KEY_SHIFTMASK, "\x1b[2C"},
          {EXKEY_LEFT | KEY_SHIFTMASK, "\x1b[2D"},
          {EXKEY_CENTER | KEY_SHIFTMASK, "\x1b[2E"}, /* --- */
          {EXKEY_END | KEY_SHIFTMASK, "\x1b[2F"},    /* --- */
          {EXKEY_CENTER | KEY_SHIFTMASK, "\x1b[2G"}, /* --- */
          {EXKEY_HOME | KEY_SHIFTMASK, "\x1b[2H"},   /* --- */

          {EXKEY_BS | KEY_ALTMASK, "\x1b\010"},

          {0, NULL}};

      static const keySeq xtermFnKeySeq[] = {

          {EXKEY_F1, "\x1bOP"}, /* kf1  - ok */
          {EXKEY_F2, "\x1bOQ"}, /* kf2  */
          {EXKEY_F3, "\x1bOR"}, /* kf3  */
          {EXKEY_F4, "\x1bOS"}, /* kf4  */

          {0, NULL}};

      static const keySeq xtermKeySeq[] = {

          {EXKEY_BS, "\010"},  /* kbs   */
          {EXKEY_TAB, "\011"}, /* ht    */
          {EXKEY_BS, "\177"},

          /* cursor keys */
          {EXKEY_UP, "\x1b[A"},
          {EXKEY_DOWN, "\x1b[B"},
          {EXKEY_RIGHT, "\x1b[C"},
          {EXKEY_LEFT, "\x1b[D"},

          {EXKEY_CENTER, "\x1b[E"}, /* XTerm */
          {EXKEY_END, "\x1b[F"},    /* XTerm */
          {EXKEY_HOME, "\x1b[H"},   /* XTerm */

          {EXKEY_TAB | KEY_SHIFTMASK, "\x1b[Z"}, /* kcbt, XTerm */

          /* Konsole */
          {EXKEY_ENTER | KEY_SHIFTMASK, "\x1bOM"},

          /* gnome-terminal */
          {EXKEY_END, "\x1bOF"},  /* kend  */
          {EXKEY_HOME, "\x1bOH"}, /* khome */
          {EXKEY_ENTER | KEY_ALTMASK, "\x1b\012"},

          {0, NULL}};

      static const keySeq ansiKeySeq[] = {

          {EXKEY_BS, "\010"},  /* kbs   */
          {EXKEY_TAB, "\011"}, /* ht    */
          {EXKEY_DEL, "\177"}, /* kdch1 */
          /* cursor keys */
          {EXKEY_UP, "\x1b[A"},     /* kcuu1 */
          {EXKEY_DOWN, "\x1b[B"},   /* kcud1 */
          {EXKEY_RIGHT, "\x1b[C"},  /* kcuf1 */
          {EXKEY_LEFT, "\x1b[D"},   /* kcub1 */
          {EXKEY_CENTER, "\x1b[E"}, /* kb2   */
          {EXKEY_END, "\x1b[F"},    /* kend  */
          {EXKEY_PGDN, "\x1b[G"},   /* knp   */
          {EXKEY_HOME, "\x1b[H"},   /* khome */
          {EXKEY_PGUP, "\x1b[I"},   /* kpp   */
          {EXKEY_INS, "\x1b[L"},    /* kich1 */

          {EXKEY_F1, "\x1b[M"},  /* kf1  */
          {EXKEY_F2, "\x1b[N"},  /* kf2  */
          {EXKEY_F3, "\x1b[O"},  /* kf3  */
          {EXKEY_F4, "\x1b[P"},  /* kf4  */
          {EXKEY_F5, "\x1b[Q"},  /* kf5  */
          {EXKEY_F6, "\x1b[R"},  /* kf6  */
          {EXKEY_F7, "\x1b[S"},  /* kf7  */
          {EXKEY_F8, "\x1b[T"},  /* kf8  */
          {EXKEY_F9, "\x1b[U"},  /* kf9  */
          {EXKEY_F10, "\x1b[V"}, /* kf10 */
          {EXKEY_F11, "\x1b[W"}, /* kf11 */
          {EXKEY_F12, "\x1b[X"}, /* kf12 */

          {EXKEY_F1 | KEY_SHIFTMASK, "\x1b[Y"},  /* kf13 */
          {EXKEY_F2 | KEY_SHIFTMASK, "\x1b[Z"},  /* kf14 */
          {EXKEY_F3 | KEY_SHIFTMASK, "\x1b[a"},  /* kf15 */
          {EXKEY_F4 | KEY_SHIFTMASK, "\x1b[b"},  /* kf16 */
          {EXKEY_F5 | KEY_SHIFTMASK, "\x1b[c"},  /* kf17 */
          {EXKEY_F6 | KEY_SHIFTMASK, "\x1b[d"},  /* kf18 */
          {EXKEY_F7 | KEY_SHIFTMASK, "\x1b[e"},  /* kf19 */
          {EXKEY_F8 | KEY_SHIFTMASK, "\x1b[f"},  /* kf20 */
          {EXKEY_F9 | KEY_SHIFTMASK, "\x1b[g"},  /* kf21 */
          {EXKEY_F10 | KEY_SHIFTMASK, "\x1b[h"}, /* kf22 */
          {EXKEY_F11 | KEY_SHIFTMASK, "\x1b[i"}, /* kf23 */
          {EXKEY_F12 | KEY_SHIFTMASK, "\x1b[j"}, /* kf24 */

          {EXKEY_F1 | KEY_CTRLMASK, "\x1b[k"},  /* kf25 */
          {EXKEY_F2 | KEY_CTRLMASK, "\x1b[l"},  /* kf26 */
          {EXKEY_F3 | KEY_CTRLMASK, "\x1b[m"},  /* kf27 */
          {EXKEY_F4 | KEY_CTRLMASK, "\x1b[n"},  /* kf28 */
          {EXKEY_F5 | KEY_CTRLMASK, "\x1b[o"},  /* kf29 */
          {EXKEY_F6 | KEY_CTRLMASK, "\x1b[p"},  /* kf30 */
          {EXKEY_F7 | KEY_CTRLMASK, "\x1b[q"},  /* kf31 */
          {EXKEY_F8 | KEY_CTRLMASK, "\x1b[r"},  /* kf32 */
          {EXKEY_F9 | KEY_CTRLMASK, "\x1b[s"},  /* kf33 */
          {EXKEY_F10 | KEY_CTRLMASK, "\x1b[t"}, /* kf34 */
          {EXKEY_F11 | KEY_CTRLMASK, "\x1b[u"}, /* kf35 */
          {EXKEY_F12 | KEY_CTRLMASK, "\x1b[v"}, /* kf36 */

          {EXKEY_F1 | KEY_ALTMASK, "\x1b[w"},  /* kf37 */
          {EXKEY_F2 | KEY_ALTMASK, "\x1b[x"},  /* kf38 */
          {EXKEY_F3 | KEY_ALTMASK, "\x1b[y"},  /* kf39 */
          {EXKEY_F4 | KEY_ALTMASK, "\x1b[z"},  /* kf40 */
          {EXKEY_F5 | KEY_ALTMASK, "\x1b[@"},  /* kf41 */
          {EXKEY_F6 | KEY_ALTMASK, "\x1b[["},  /* kf42 */
          {EXKEY_F7 | KEY_ALTMASK, "\x1b[\\"}, /* kf43 */
          {EXKEY_F8 | KEY_ALTMASK, "\x1b[]"},  /* kf44 */
          {EXKEY_F9 | KEY_ALTMASK, "\x1b[^"},  /* kf45 */
          {EXKEY_F10 | KEY_ALTMASK, "\x1b[_"}, /* kf46 */
          {EXKEY_F11 | KEY_ALTMASK, "\x1b[`"}, /* kf47 */
          {EXKEY_F12 | KEY_ALTMASK, "\x1b[{"}, /* kf48 */

          {0, NULL}};

      //static const keySeq bsdConsKeySeq[] = {
      //    {EXKEY_TAB | KEY_SHIFTMASK, "\x1b[Z"}, /* SHIFT+TAB */
      //    {0, NULL}};

      addKeyTab(pTerm, stdKeySeq);
      addKeyTab(pTerm, xtermKeySeq);
      addKeyTab(pTerm, xtermFnKeySeq);
      addKeyTab(pTerm, stdFnKeySeq);
      addKeyTab(pTerm, stdCursorKeySeq);
      addKeyTab(pTerm, xtermModKeySeq);
}

static void hb_gt_ele_SetTerm(PHB_GTELE pTerm)
{
      static const char *szAcsc = "``aaffggiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz{{||}}~~";
      static const char *szExtAcsc = "+\020,\021-\030.\0310\333`\004a\261f\370g\361h\260i\316j\331k\277l\332m\300n\305o~p\304q\304r\304s_t\303u\264v\301w\302x\263y\363z\362{\343|\330}\234~\376";
      const char *szTerm;
      int iValue;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetTerm(%p)", pTerm));

      if (pTerm->iOutBufSize == 0)
      {
            pTerm->iOutBufIndex = 0;
            pTerm->iOutBufSize = 16384;
            pTerm->pOutBuf = (char *)hb_xgrab(pTerm->iOutBufSize);
      }
      pTerm->mouse_type = MOUSE_XTERM;
      pTerm->esc_delay = ESC_DELAY;
      pTerm->iAttrMask = ~HB_GTELE_ATTR_BOX;
      pTerm->iExtColor = HB_GTELE_CLRSTD;
      pTerm->terminal_ext = 0;
      pTerm->fAM = HB_FALSE;

      /* standard VGA colors */
      pTerm->colors[0x00] = 0x000000;
      pTerm->colors[0x01] = 0xAA0000;
      pTerm->colors[0x02] = 0x00AA00;
      pTerm->colors[0x03] = 0xAAAA00;
      pTerm->colors[0x04] = 0x0000AA;
      pTerm->colors[0x05] = 0xAA00AA;
      pTerm->colors[0x06] = 0x0055AA;
      pTerm->colors[0x07] = 0xAAAAAA;
      pTerm->colors[0x08] = 0x555555;
      pTerm->colors[0x09] = 0xFF5555;
      pTerm->colors[0x0A] = 0x55FF55;
      pTerm->colors[0x0B] = 0xFFFF55;
      pTerm->colors[0x0C] = 0x5555FF;
      pTerm->colors[0x0D] = 0xFF55FF;
      pTerm->colors[0x0E] = 0x55FFFF;
      pTerm->colors[0x0F] = 0xFFFFFF;

      szTerm = "xterm";

      pTerm->Init = hb_gt_ele_AnsiInit;
      pTerm->Exit = hb_gt_ele_AnsiExit;
      pTerm->SetTermMode = hb_gt_ele_LinuxSetTermMode;
      pTerm->GetCursorPos = hb_gt_ele_AnsiGetCursorPos;
      pTerm->SetCursorPos = hb_gt_ele_AnsiSetCursorPos;
      pTerm->SetCursorStyle = hb_gt_ele_AnsiSetCursorStyle;
      pTerm->SetAttributes = hb_gt_ele_XtermSetAttributes;
      pTerm->SetMode = hb_gt_ele_XtermSetMode;
      pTerm->GetAcsc = hb_gt_ele_AnsiGetAcsc;
      pTerm->Tone = hb_gt_ele_AnsiTone;
      pTerm->Bell = hb_gt_ele_AnsiBell;
      pTerm->szAcsc = szAcsc;
      pTerm->terminal_type = TERM_XTERM;

      pTerm->fStdinTTY = hb_fsIsDevice(pTerm->hFilenoStdin);   // -> BOOL true if hFilenoStdin exists
      pTerm->fStdoutTTY = hb_fsIsDevice(pTerm->hFilenoStdout);
      pTerm->fStderrTTY = hb_fsIsDevice(pTerm->hFilenoStderr);

      //pTerm->hFileno = pTerm->hFilenoStdout;

      //pTerm->fOutTTY = pTerm->fStdoutTTY;
      //if (!pTerm->fOutTTY && pTerm->fStdinTTY)
      //{
        //    pTerm->hFileno = pTerm->hFilenoStdin;
            //pTerm->fOutTTY = HB_TRUE;
      //}

      // gdje se cita odgovor na upit
      //pTerm->fPosAnswer =  pTerm->fStdoutTTY; //pTerm->fOutTTY; //&& !hb_ele_Param("NOPOS", NULL);

      pTerm->fUTF8 = HB_FALSE;

      hb_fsSetDevMode(pTerm->hFilenoStdin, FD_BINARY);

#if defined( HB_OS_UNIX )
      hb_gt_chrmapinit(pTerm->charmap, szTerm, pTerm->terminal_type == TERM_XTERM);
#endif

#ifndef HB_GT_UNICODE_BUF
      pTerm->cdpHost = pTerm->cdpIn = NULL;
      pTerm->cdpBox = hb_cdpFind("EN");
      HB_TRACE(HB_TR_DEBUG, ("UNICODE_BUF ON"));
#else
      HB_TRACE(HB_TR_DEBUG, ("UNICODE_BUF OFF"));
#endif

      // add stdin u listu input file descriptora
      add_efds(pTerm, pTerm->hFilenoStdin, O_RDONLY, NULL, NULL);
      init_keys(pTerm);
      mouse_init(pTerm);
      HB_TRACE(HB_TR_DEBUG, ("GTELE broj file descriptora=%d", pTerm->efds_size));
}

static void hb_gt_ele_Init(PHB_GT pGT, HB_FHANDLE hFilenoStdin, HB_FHANDLE hFilenoStdout, HB_FHANDLE hFilenoStderr)
{
      int iRows = 24, iCols = 80;
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Init(%p,%p,%p,%p)", pGT, (void *)(HB_PTRUINT)hFilenoStdin, (void *)(HB_PTRUINT)hFilenoStdout, (void *)(HB_PTRUINT)hFilenoStderr));

      HB_GTLOCAL(pGT) = pTerm = (PHB_GTELE)hb_xgrabz(sizeof(HB_GTELE));

      //process();

      pTerm->pGT = pGT;
      pTerm->hFilenoStdin = hFilenoStdin;
      pTerm->hFilenoStdout = hFilenoStdout;
      pTerm->hFilenoStderr = hFilenoStderr;

      hb_gt_ele_SetTerm(pTerm);

      /* SA_NOCLDSTOP in #if is a hack to detect POSIX compatible environment */
      //#if defined(HB_OS_UNIX) && defined(SA_NOCLDSTOP)

      //if (pTerm->fStdinTTY)
      //
      //struct sigaction act, old;

      //s_fRestTTY = HB_TRUE;

      /* if( pTerm->saved_TIO.c_lflag & TOSTOP ) != 0 */

      /* signali out
      sigaction(SIGTTOU, NULL, &old);
      memcpy(&act, &old, sizeof(struct sigaction));
      act.sa_handler = sig_handler;
      // do not use SA_RESTART - new Linux kernels will repeat the operation 
#if defined(SA_ONESHOT)
      act.sa_flags = SA_ONESHOT;
#elif defined(SA_RESETHAND)
      act.sa_flags = SA_RESETHAND;
#else
      act.sa_flags = 0;
#endif
      sigaction(SIGTTOU, &act, 0);
*/

      //hernad tcgetattr( pTerm->hFilenoStdin, &pTerm->saved_TIO );
      //hernad memcpy( &pTerm->curr_TIO, &pTerm->saved_TIO, sizeof( struct termios ) );

      /* atexit( restore_input_mode ); */

      /* hernad
      pTerm->curr_TIO.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
      pTerm->curr_TIO.c_lflag |= NOFLSH;
      pTerm->curr_TIO.c_cflag &= ~( CSIZE | PARENB );
      pTerm->curr_TIO.c_cflag |= CS8 | CREAD;
      pTerm->curr_TIO.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
      pTerm->curr_TIO.c_oflag &= ~OPOST;
     */

      /* Enable LF->CR+LF translation */

      // pTerm->curr_TIO.c_oflag = ONLCR | OPOST;
      // memset( pTerm->curr_TIO.c_cc, 0, NCCS );

      /* workaround for bug in some Linux kernels (i.e. 3.13.0-64-generic
         *buntu) in which select() unconditionally accepts stdin for
         reading if c_cc[ VMIN ] = 0 [druzus] */

      // control characters for the terminal session
      // line discipline code
      // VMIN - character count 0 - 255 chars; read() is satisfied when VMIN characters have been transfered to the callers buffer
      //pTerm->curr_TIO.c_cc[VMIN] = 1;
      //pTerm->curr_TIO.c_cc[VTIME] = 0;

      /* pTerm->curr_TIO.c_cc[ VMIN ] = 0; */
      /* pTerm->curr_TIO.c_cc[ VTIME ] = 0; */

      /*
       http://unixwiz.net/techtips/termios-vmin-vtime.html
      Function-key processing
      On a regular keyboard, most keys send just one byte each, but almost all keyboards have special keys 
      that send a sequence of characters at a time. Examples (from an ANSI keyboard)
      ESC [ A       up arrow
      ESC [ 5 ~     page up
      ESC [ 18 ~    F7
      and so on 
      
      From a strictly "string recognition" point of view, it's easy enough to translate "ESC [ A" into "up arrow" 
      inside a program, but how does it tell the difference between "user typed up-arrow" 
      and "user typed the ESCAPE key"? >>>>>>>> The difference is timing <<<<<<<<<<<<<<<<<<<< 
      If the ESCAPE is immediately followed by the rest of the expected sequence, then it's a function key: otherwise it's just a plain ESCAPE.
      */

      //tcsetattr(pTerm->hFilenoStdin, TCSAFLUSH, &pTerm->curr_TIO);

      // ovo ne treba
      //act.sa_handler = SIG_DFL;
      //sigaction(SIGTTOU, &old, NULL);

      //pTerm->fRestTTY = s_fRestTTY;
      //}
      //set_signals();
      if (!hb_gt_ele_getSize(pTerm, &iRows, &iCols))
      {
            iRows = 24;
            iCols = 80;
      }
      //#endif

      HB_GTSUPER_INIT(pGT, hFilenoStdin, hFilenoStdout, hFilenoStderr);
      HB_GTSELF_RESIZE(pGT, iRows, iCols);
      HB_GTSELF_SETFLAG(pGT, HB_GTI_COMPATBUFFER, HB_FALSE);
      HB_GTSELF_SETFLAG(pGT, HB_GTI_REDRAWMAX, 8);
      HB_GTSELF_SETFLAG(pGT, HB_GTI_STDOUTCON, pTerm->fStdoutTTY);
      HB_GTSELF_SETFLAG(pGT, HB_GTI_STDERRCON, pTerm->fStderrTTY);

#if defined( HB_OS_WIN )
      SetConsoleMode( ( HANDLE ) hb_fsGetOsHandle( pTerm->hFilenoStdin ), 0x0000 );
#endif


      pTerm->Init(pTerm);
      pTerm->SetTermMode(pTerm, 0);
#ifdef HB_GTELE_CHK_EXACT_POS
      if (pTerm->GetCursorPos(pTerm, &pTerm->iRow, &pTerm->iCol, NULL))
            HB_GTSELF_SETPOS(pGT, pTerm->iRow, pTerm->iCol);
      pTerm->fUTF8 = hb_trm_isUTF8(pTerm);
#else
      pTerm->fUTF8 = hb_trm_isUTF8(pTerm);
      //if (pTerm->fPosAnswer)
      HB_GTSELF_SETPOS(pGT, pTerm->iRow, pTerm->iCol);
#endif
      if (!pTerm->fUTF8)
      {
#ifndef HB_GT_UNICODE_BUF
            hb_gt_ele_SetKeyTrans(pTerm);
#endif
            hb_gt_ele_SetDispTrans(pTerm, 0);
      }
      HB_GTSELF_SETBLINK(pGT, HB_TRUE);
      //if (pTerm->fOutTTY)
      HB_GTSELF_SEMICOLD(pGT);
}

static void hb_gt_ele_Exit(PHB_GT pGT)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Exit(%p)", pGT));

      HB_GTSELF_REFRESH(pGT);

      pTerm = HB_GTELE_GET(pGT);
      if (pTerm)
      {
            mouse_exit(pTerm);
            del_all_efds(pTerm);
            if (pTerm->pKeyTab)
                  removeAllKeyMap(pTerm, &pTerm->pKeyTab);

            pTerm->Exit(pTerm);
            hb_gt_ele_ResetPalette(pTerm);
            if ( pTerm->iCol > 0)
                  hb_gt_ele_termOut(pTerm, "\r\n", 2);
            hb_gt_ele_termFlush(pTerm);
      }

      HB_GTSUPER_EXIT(pGT);

      if (pTerm)
      {
            /* hernad

      //if( pTerm->fRestTTY )
         tcsetattr( pTerm->hFilenoStdin, TCSANOW, &pTerm->saved_TIO );
*/

            if (pTerm->nLineBufSize > 0)
                  hb_xfree(pTerm->pLineBuf);
            if (pTerm->iOutBufSize > 0)
                  hb_xfree(pTerm->pOutBuf);
            hb_xfree(pTerm);
      }
}

static HB_BOOL hb_gt_ele_mouse_IsPresent(PHB_GT pGT)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_IsPresent(%p)", pGT));

      return HB_GTELE_GET(pGT)->mouse_type != MOUSE_NONE;
}

static void hb_gt_ele_mouse_Show(PHB_GT pGT)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_Show(%p)", pGT));

      pTerm = HB_GTELE_GET(pGT);
      /*      
#if defined(HB_HAS_GPM)
      if (pTerm->mouse_type & MOUSE_GPM)
            gpm_visiblepointer = 1;
#endif
*/
      disp_mousecursor(pTerm);
}

static void hb_gt_ele_mouse_Hide(PHB_GT pGT)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_Hide(%p)", pGT));

      /*
#if defined(HB_HAS_GPM)
      if (HB_GTELE_GET(pGT)->mouse_type & MOUSE_GPM)
      {
            gpm_visiblepointer = 0;
      }
#else
*/
      HB_SYMBOL_UNUSED(pGT);
      //#endif
}

static void hb_gt_ele_mouse_GetPos(PHB_GT pGT, int *piRow, int *piCol)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_Col(%p,%p,%p)", pGT, piRow, piCol));

      pTerm = HB_GTELE_GET(pGT);
      *piRow = pTerm->mLastEvt.row;
      *piCol = pTerm->mLastEvt.col;
}

static void hb_gt_ele_mouse_SetPos(PHB_GT pGT, int iRow, int iCol)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_SetPos(%p,%i,%i)", pGT, iRow, iCol));

      pTerm = HB_GTELE_GET(pGT);
      /* it does really nothing */
      pTerm->mLastEvt.col = iCol;
      pTerm->mLastEvt.row = iRow;
      disp_mousecursor(pTerm);
}

static HB_BOOL hb_gt_ele_mouse_ButtonState(PHB_GT pGT, int iButton)
{
      PHB_GTELE pTerm;
      HB_BOOL ret = HB_FALSE;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_ButtonState(%p,%i)", pGT, iButton));

      pTerm = HB_GTELE_GET(pGT);
      if (pTerm->mouse_type != MOUSE_NONE)
      {
            int mask;

            if (iButton == 0)
                  mask = M_BUTTON_LEFT;
            else if (iButton == 1)
                  mask = M_BUTTON_RIGHT;
            else if (iButton == 2)
                  mask = M_BUTTON_MIDDLE;
            else
                  mask = 0;

            ret = (pTerm->mLastEvt.buttonstate & mask) != 0;
      }

      return ret;
}

static int hb_gt_ele_mouse_CountButton(PHB_GT pGT)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_mouse_CountButton(%p)", pGT));

      return HB_GTELE_GET(pGT)->mButtons;
}

static int hb_gt_ele_ReadKey(PHB_GT pGT, int iEventMask)
{

#if defined( HB_OS_UNIX )
      int iKey;

      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_ReadKey(%p,%d)", pGT, iEventMask));

      HB_SYMBOL_UNUSED(iEventMask);

      iKey = wait_key(HB_GTELE_GET(pGT), -1);

      if (iKey == K_RESIZE)
      {
            int iRows, iCols;

            if (hb_gt_ele_getSize(HB_GTELE_GET(pGT), &iRows, &iCols))
            {
                  HB_GTSELF_RESIZE(pGT, iRows, iCols);
                  iKey = HB_INKEY_NEW_EVENT(HB_K_RESIZE); // emituje na osnovu inkey event
            }
            else
                  iKey = 0;
      }

      if (iKey)
      {
            HB_TRACE(HB_TR_DEBUG, ("GTELE read_key: %c", iKey));
      }

      return iKey;


#elif defined( HB_OS_WIN )

   PHB_GTELE pTerm;
   int ch = 0;

   HB_SYMBOL_UNUSED( iEventMask );

   pTerm = HB_GTELE_GET( pGT );

   HB_BYTE bChar;
   if( hb_fsRead( pTerm->hFilenoStdin, &bChar, 1 ) == 1 )
      ch = bChar;


/*
   if( WaitForSingleObject( ( HANDLE ) hb_fsGetOsHandle( pTerm->hFilenoStdin ), 0 ) == WAIT_OBJECT_0 )
   {
      INPUT_RECORD  ir;
      DWORD         dwEvents;
      while( PeekConsoleInput( ( HANDLE ) hb_fsGetOsHandle( pTerm->hFilenoStdin ), &ir, 1, &dwEvents ) && dwEvents == 1 )
      {
         if( ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown )
         {
            HB_BYTE bChar;
            if( hb_fsRead( pTerm->hFilenoStdin, &bChar, 1 ) == 1 )
               ch = bChar;
         }
         else // Remove from the input queue
            ReadConsoleInput( ( HANDLE ) hb_fsGetOsHandle( pTerm->hFilenoStdin ), &ir, 1, &dwEvents );
      }
 
   }
*/

   if( ch )
   {
      int u = HB_GTSELF_KEYTRANS( pGT, ch );
      if( u )
         ch = HB_INKEY_NEW_UNICODE( u );
   }

   if ( ch )
      HB_TRACE( HB_TR_DEBUG, ( "windows read_key %d %c", ch, ch) );

   return ch;
#endif

}

static void hb_gt_ele_Tone(PHB_GT pGT, double dFrequency, double dDuration)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Tone(%p,%lf,%lf)", pGT, dFrequency, dDuration));

      pTerm = HB_GTELE_GET(pGT);
      pTerm->Tone(pTerm, dFrequency, dDuration);
}

static void hb_gt_ele_Bell(PHB_GT pGT)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Bell(%p)", pGT));

      pTerm = HB_GTELE_GET(pGT);
      pTerm->Bell(pTerm);
}

static const char *hb_gt_ele_Version(PHB_GT pGT, int iType)
{
      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Version(%p,%d)", pGT, iType));

      HB_SYMBOL_UNUSED(pGT);

      if (iType == 0)
            return HB_GT_DRVNAME(HB_GT_NAME);

      return "Terminal: electron desktop";
}

static HB_BOOL hb_gt_ele_Suspend(PHB_GT pGT)
{
      PHB_GTELE pTerm;

      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Suspend(%p)", pGT));

      pTerm = HB_GTELE_GET(pGT);
      if (pTerm->mouse_type & MOUSE_XTERM)
            hb_gt_ele_termOut(pTerm, s_szMouseOff, strlen(s_szMouseOff));
      /* hernad
#if defined( HB_OS_UNIX )
   if( pTerm->fRestTTY )
      tcsetattr( pTerm->hFilenoStdin, TCSANOW, &pTerm->saved_TIO );
#endif
*/
      /* Enable line wrap when cursor set after last column */
      pTerm->SetTermMode(pTerm, 1);
      return HB_TRUE;
}

static HB_BOOL hb_gt_ele_Resume(PHB_GT pGT)
{
      PHB_GTELE pTerm;
      int iHeight, iWidth;

      //HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Resume(%p)", pGT));

      pTerm = HB_GTELE_GET(pGT);

      /* hernad
#if defined( HB_OS_UNIX )
   if( pTerm->fRestTTY )
      tcsetattr( pTerm->hFilenoStdin, TCSANOW, &pTerm->curr_TIO );
#endif
*/
      if (pTerm->mouse_type & MOUSE_XTERM)
            hb_gt_ele_termOut(pTerm, s_szMouseOn, strlen(s_szMouseOn));

      pTerm->Init(pTerm);

      HB_GTSELF_GETSIZE(pGT, &iHeight, &iWidth);
      HB_GTSELF_EXPOSEAREA(pGT, 0, 0, iHeight, iWidth);

      HB_GTSELF_REFRESH(pGT);

      return HB_TRUE;
}

static void hb_gt_ele_Scroll(PHB_GT pGT, int iTop, int iLeft, int iBottom, int iRight,
                             int iColor, HB_USHORT usChar, int iRows, int iCols)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Scroll(%p,%d,%d,%d,%d,%d,%d,%d,%d)", pGT, iTop, iLeft, iBottom, iRight, iColor, usChar, iRows, iCols));

      /* Provide some basic scroll support for full screen */
      if (iCols == 0 && iRows > 0 && iTop == 0 && iLeft == 0)
      {
            PHB_GTELE pTerm = HB_GTELE_GET(pGT);
            int iHeight, iWidth;

            HB_GTSELF_GETSIZE(pGT, &iHeight, &iWidth);
            if (iBottom >= iHeight - 1 && iRight >= iWidth - 1 &&
                pTerm->iRow == iHeight - 1)
            {
                  /* scroll up the internal screen buffer */
                  HB_GTSELF_SCROLLUP(pGT, iRows, iColor, usChar);
                  /* set default color for terminals which use it to erase
          * scrolled area */
                  pTerm->SetAttributes(pTerm, iColor & pTerm->iAttrMask);
                  /* update our internal row position */
                  do
                  {
                        hb_gt_ele_termOut(pTerm, "\r\n", 2);
                  } while (--iRows > 0);
                  pTerm->iCol = 0;
                  return;
            }
      }

      HB_GTSUPER_SCROLL(pGT, iTop, iLeft, iBottom, iRight, iColor, usChar, iRows, iCols);
}

static HB_BOOL hb_gt_ele_SetMode(PHB_GT pGT, int iRows, int iCols)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetMode(%d,%d)", iRows, iCols));

      if (iRows > 0 && iCols > 0)
      {
            PHB_GTELE pTerm = HB_GTELE_GET(pGT);
            if (pTerm->SetMode(pTerm, &iRows, &iCols))
            {
                  HB_GTSELF_RESIZE(pGT, iRows, iCols);
                  return HB_TRUE;
            }
      }
      return HB_FALSE;
}

static void hb_gt_ele_SetBlink(PHB_GT pGT, HB_BOOL fBlink)
{
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetBlink(%p,%d)", pGT, (int)fBlink));

      pTerm = HB_GTELE_GET(pGT);

      {
            if (fBlink)
                  pTerm->iAttrMask |= 0x0080;
            else
                  pTerm->iAttrMask &= ~0x0080;
      }

      HB_GTSUPER_SETBLINK(pGT, fBlink);
}

static HB_BOOL hb_gt_ele_SetDispCP(PHB_GT pGT, const char *pszTermCDP, const char *pszHostCDP, HB_BOOL fBox)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetDispCP(%p,%s,%s,%d)", pGT, pszTermCDP, pszHostCDP, (int)fBox));

      if (HB_GTSUPER_SETDISPCP(pGT, pszTermCDP, pszHostCDP, fBox))
      {
            if (!HB_GTELE_GET(pGT)->fUTF8)
                  hb_gt_ele_SetDispTrans(HB_GTELE_GET(pGT), fBox ? 1 : 0);
            return HB_TRUE;
      }
      return HB_FALSE;
}

#ifndef HB_GT_UNICODE_BUF
static HB_BOOL hb_gt_ele_SetKeyCP(PHB_GT pGT, const char *pszTermCDP, const char *pszHostCDP)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_SetKeyCP(%p,%s,%s)", pGT, pszTermCDP, pszHostCDP));

      if (HB_GTSUPER_SETKEYCP(pGT, pszTermCDP, pszHostCDP))
      {
            if (!HB_GTELE_GET(pGT)->fUTF8)
                  hb_gt_ele_SetKeyTrans(HB_GTELE_GET(pGT));
            return HB_TRUE;
      }
      return HB_FALSE;
}
#endif

static void hb_gt_ele_Redraw(PHB_GT pGT, int iRow, int iCol, int iSize)
{
      PHB_GTELE pTerm;
      HB_BYTE bAttr;
      HB_USHORT usChar;
      int iLen, iChars, iAttribute, iColor;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Redraw(%p,%d,%d,%d)", pGT, iRow, iCol, iSize));

      iLen = iChars = iAttribute = 0;
      pTerm = HB_GTELE_GET(pGT);
      pTerm->SetTermMode(pTerm, 0);
      if (iRow < pTerm->iRow)
            pTerm->SetCursorStyle(pTerm, SC_NONE);
      if (pTerm->fAM && iRow == pTerm->iHeight - 1 && iCol + iSize == pTerm->iWidth)
            iSize--;
      while (iSize--)
      {
#ifdef HB_GT_UNICODE_BUF
            if (pTerm->fUTF8)
            {
                  if (!HB_GTSELF_GETSCRCHAR(pGT, iRow, iCol + iChars, &iColor, &bAttr, &usChar))
                        break;
                  if (bAttr & HB_GT_ATTR_BOX)
                        iColor |= HB_GTELE_ATTR_BOX;
                  usChar = hb_cdpGetU16Ctrl(usChar);
            }
            else
            {
                  HB_UCHAR uc;
                  if (!HB_GTSELF_GETSCRUC(pGT, iRow, iCol + iChars, &iColor, &bAttr, &uc, HB_FALSE))
                        break;
                  if (bAttr & HB_GT_ATTR_BOX)
                  {
                        iColor |= (pTerm->boxattr[uc] & ~HB_GTELE_ATTR_CHAR);
                        usChar = pTerm->boxattr[uc] & HB_GTELE_ATTR_CHAR;
                  }
                  else
                  {
                        iColor |= (pTerm->chrattr[uc] & ~HB_GTELE_ATTR_CHAR);
                        usChar = pTerm->chrattr[uc] & HB_GTELE_ATTR_CHAR;
                  }
            }

            if (iLen == 0)
                  iAttribute = iColor;
            else if (iColor != iAttribute)
            {
                  hb_gt_ele_PutStr(pTerm, iRow, iCol, iAttribute, pTerm->pLineBuf, iLen, iChars);
                  iCol += iChars;
                  iLen = iChars = 0;
                  iAttribute = iColor;
            }
            if (pTerm->fUTF8)
                  iLen += hb_cdpU16CharToUTF8(pTerm->pLineBuf + iLen, usChar);
            else
                  pTerm->pLineBuf[iLen++] = (char)usChar;
            ++iChars;
#else
            if (!HB_GTSELF_GETSCRCHAR(pGT, iRow, iCol + iChars, &iColor, &bAttr, &usChar))
                  break;
            usChar &= 0xff;
            if (bAttr & HB_GT_ATTR_BOX)
            {
                  iColor |= (pTerm->boxattr[usChar] & ~HB_GTELE_ATTR_CHAR);
                  if (!pTerm->fUTF8)
                        usChar = pTerm->boxattr[usChar] & HB_GTELE_ATTR_CHAR;
                  else
                        iColor |= HB_GTELE_ATTR_BOX;
            }
            else
            {
                  iColor |= (pTerm->chrattr[usChar] & ~HB_GTELE_ATTR_CHAR);
                  if (!pTerm->fUTF8)
                        usChar = pTerm->chrattr[usChar] & HB_GTELE_ATTR_CHAR;
            }
            if (iLen == 0)
                  iAttribute = iColor;
            else if (iColor != iAttribute)
            {
                  hb_gt_ele_PutStr(pTerm, iRow, iCol, iAttribute, pTerm->pLineBuf, iLen, iChars);
                  iCol += iChars;
                  iLen = iChars = 0;
                  iAttribute = iColor;
            }
            pTerm->pLineBuf[iLen++] = (char)usChar;
            ++iChars;
#endif
      }
      if (iLen)
            hb_gt_ele_PutStr(pTerm, iRow, iCol, iAttribute, pTerm->pLineBuf, iLen, iChars);
}

static void hb_gt_ele_Refresh(PHB_GT pGT)
{
      int iRow, iCol, iStyle;
      HB_SIZE nLineBufSize;
      PHB_GTELE pTerm;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Refresh(%p)", pGT));

      pTerm = HB_GTELE_GET(pGT);

      HB_GTSELF_GETSIZE(pGT, &pTerm->iHeight, &pTerm->iWidth);

#ifdef HB_GT_UNICODE_BUF
      nLineBufSize = pTerm->iWidth * (pTerm->fUTF8 ? 3 : 1);
#else
      nLineBufSize = pTerm->iWidth;
#endif
      if (pTerm->nLineBufSize != nLineBufSize)
      {
            pTerm->pLineBuf = (char *)hb_xrealloc(pTerm->pLineBuf, nLineBufSize);
            pTerm->nLineBufSize = nLineBufSize;
      }

      HB_GTSUPER_REFRESH(pGT);

      HB_GTSELF_GETSCRCURSOR(pGT, &iRow, &iCol, &iStyle);
      if (iStyle != SC_NONE)
      {
            if (iRow >= 0 && iCol >= 0 &&
                iRow < pTerm->iHeight && iCol < pTerm->iWidth)
                  pTerm->SetCursorPos(pTerm, iRow, iCol);
            else
                  iStyle = SC_NONE;
      }
      pTerm->SetCursorStyle(pTerm, iStyle);
      hb_gt_ele_termFlush(pTerm);
      disp_mousecursor(pTerm);
}

static HB_BOOL hb_gt_ele_Info(PHB_GT pGT, int iType, PHB_GT_INFO pInfo)
{
      PHB_GTELE pTerm;
      const char *szVal;
      void *hVal;
      int iVal;

      HB_TRACE(HB_TR_DEBUG, ("hb_gt_ele_Info(%p,%d,%p)", pGT, iType, pInfo));

      pTerm = HB_GTELE_GET(pGT);
      switch (iType)
      {
      case HB_GTI_ISSCREENPOS:
      case HB_GTI_KBDSUPPORT:
            pInfo->pResult = hb_itemPutL(pInfo->pResult, HB_TRUE);
            break;

      case HB_GTI_ISUNICODE:
            pInfo->pResult = hb_itemPutL(pInfo->pResult, pTerm->fUTF8);
            break;

#ifndef HB_GT_UNICODE_BUF
      case HB_GTI_BOXCP:
            pInfo->pResult = hb_itemPutC(pInfo->pResult,
                                         pTerm->cdpBox ? pTerm->cdpBox->id : NULL);
            szVal = hb_itemGetCPtr(pInfo->pNewVal);
            if (szVal && *szVal)
            {
                  PHB_CODEPAGE cdpBox = hb_cdpFind(szVal);
                  if (cdpBox)
                        pTerm->cdpBox = cdpBox;
            }
            break;
#endif

      case HB_GTI_ESCDELAY:
            pInfo->pResult = hb_itemPutNI(pInfo->pResult, pTerm->esc_delay);
            if (hb_itemType(pInfo->pNewVal) & HB_IT_NUMERIC)
                  pTerm->esc_delay = hb_itemGetNI(pInfo->pNewVal);
            break;

      case HB_GTI_KBDSHIFTS:
            pInfo->pResult = hb_itemPutNI(pInfo->pResult,
                                          hb_gt_ele_getKbdState(pTerm));
            break;

      case HB_GTI_DELKEYMAP:
            szVal = hb_itemGetCPtr(pInfo->pNewVal);
            if (szVal && *szVal)
                  removeKeyMap(pTerm, hb_itemGetCPtr(pInfo->pNewVal));
            break;

      case HB_GTI_ADDKEYMAP:
            if (hb_itemType(pInfo->pNewVal) & HB_IT_ARRAY)
            {
                  iVal = hb_arrayGetNI(pInfo->pNewVal, 1);
                  szVal = hb_arrayGetCPtr(pInfo->pNewVal, 2);
                  if (iVal && szVal && *szVal)
                        addKeyMap(pTerm, HB_INKEY_ISEXT(iVal) ? iVal : SET_CLIPKEY(iVal), szVal);
            }
            break;

      case HB_GTI_WINTITLE:
            if (pTerm->fUTF8)
                  pInfo->pResult = hb_itemPutStrUTF8(pInfo->pResult, pTerm->szTitle);
            else
#ifdef HB_GT_UNICODE_BUF
                  pInfo->pResult = hb_itemPutStr(pInfo->pResult, HB_GTSELF_TERMCP(pGT), pTerm->szTitle);
#else
                  pInfo->pResult = hb_itemPutStr(pInfo->pResult, pTerm->cdpTerm, pTerm->szTitle);
#endif
            if (hb_itemType(pInfo->pNewVal) & HB_IT_STRING)
            {
                  if (pTerm->fUTF8)
                        szVal = hb_itemGetStrUTF8(pInfo->pNewVal, &hVal, NULL);
                  else
#ifdef HB_GT_UNICODE_BUF
                        szVal = hb_itemGetStr(pInfo->pNewVal, HB_GTSELF_TERMCP(pGT), &hVal, NULL);
#else
                        szVal = hb_itemGetStr(pInfo->pNewVal, pTerm->cdpTerm, &hVal, NULL);
#endif

                  if (pTerm->szTitle)
                        hb_xfree(pTerm->szTitle);
                  pTerm->szTitle = (szVal && *szVal) ? hb_strdup(szVal) : NULL;
                  hb_gt_ele_SetTitle(pTerm, pTerm->szTitle);
                  hb_gt_ele_termFlush(pTerm);
                  hb_strfree(hVal);
            }
            break;

      case HB_GTI_PALETTE:
            if (hb_itemType(pInfo->pNewVal) & HB_IT_NUMERIC)
            {
                  iVal = hb_itemGetNI(pInfo->pNewVal);
                  if (iVal >= 0 && iVal < 16)
                  {
                        pInfo->pResult = hb_itemPutNI(pInfo->pResult, pTerm->colors[iVal]);
                        if (hb_itemType(pInfo->pNewVal2) & HB_IT_NUMERIC)
                        {
                              pTerm->colors[iVal] = hb_itemGetNI(pInfo->pNewVal2);
                              hb_gt_ele_SetPalette(pTerm, iVal, iVal);
                              hb_gt_ele_termFlush(pTerm);
                        }
                  }
            }
            else
            {
                  if (!pInfo->pResult)
                        pInfo->pResult = hb_itemNew(NULL);
                  hb_arrayNew(pInfo->pResult, 16);
                  for (iVal = 0; iVal < 16; iVal++)
                        hb_arraySetNI(pInfo->pResult, iVal + 1, pTerm->colors[iVal]);
                  if (hb_itemType(pInfo->pNewVal) & HB_IT_ARRAY &&
                      hb_arrayLen(pInfo->pNewVal) == 16)
                  {
                        for (iVal = 0; iVal < 16; iVal++)
                              pTerm->colors[iVal] = hb_arrayGetNI(pInfo->pNewVal, iVal + 1);
                        hb_gt_ele_SetPalette(pTerm, 0, 15);
                        hb_gt_ele_termFlush(pTerm);
                  }
            }
            break;

      case HB_GTI_RESIZABLE:
            pInfo->pResult = hb_itemPutL(pInfo->pResult, HB_TRUE);
            break;

      case HB_GTI_DESKTOPROWS:
      {

            pTerm->iMaxRows = hb_itemGetNI(pInfo->pNewVal);
            pInfo->pResult = hb_itemPutNI(pInfo->pResult, pTerm->iMaxRows);
            break;
      }

      case HB_GTI_DESKTOPCOLS:
      {

            pTerm->iMaxCols = hb_itemGetNI(pInfo->pNewVal);
            pInfo->pResult = hb_itemPutNI(pInfo->pResult, pTerm->iMaxCols);

            HB_GTSELF_RESIZE(pGT, pTerm->iMaxRows, pTerm->iMaxCols);
            break;
      }

      case HB_GTI_CLOSABLE:
            pInfo->pResult = hb_itemPutL(pInfo->pResult, HB_TRUE);
            break;

      default:
            return HB_GTSUPER_INFO(pGT, iType, pInfo);
      }

      return HB_TRUE;
}

static HB_BOOL hb_gt_FuncInit(PHB_GT_FUNCS pFuncTable)
{
      HB_TRACE(HB_TR_DEBUG, ("hb_gt_FuncInit(%p)", pFuncTable));

      pFuncTable->Init = hb_gt_ele_Init;
      pFuncTable->Exit = hb_gt_ele_Exit;
      pFuncTable->Redraw = hb_gt_ele_Redraw;
      pFuncTable->Refresh = hb_gt_ele_Refresh;
      pFuncTable->Scroll = hb_gt_ele_Scroll;
      pFuncTable->Version = hb_gt_ele_Version;
      pFuncTable->Suspend = hb_gt_ele_Suspend;
      pFuncTable->Resume = hb_gt_ele_Resume;
      pFuncTable->SetMode = hb_gt_ele_SetMode;
      pFuncTable->SetBlink = hb_gt_ele_SetBlink;
      pFuncTable->SetDispCP = hb_gt_ele_SetDispCP;
#ifndef HB_GT_UNICODE_BUF
      HB_TRACE( HB_TR_DEBUG, ("UNICODE_BUF YES") );
      pFuncTable->SetKeyCP = hb_gt_ele_SetKeyCP;
#endif
      pFuncTable->Tone = hb_gt_ele_Tone;
      pFuncTable->Bell = hb_gt_ele_Bell;
      pFuncTable->Info = hb_gt_ele_Info;

      pFuncTable->ReadKey = hb_gt_ele_ReadKey;

      pFuncTable->MouseIsPresent = hb_gt_ele_mouse_IsPresent;
      pFuncTable->MouseShow = hb_gt_ele_mouse_Show;
      pFuncTable->MouseHide = hb_gt_ele_mouse_Hide;
      pFuncTable->MouseGetPos = hb_gt_ele_mouse_GetPos;
      pFuncTable->MouseSetPos = hb_gt_ele_mouse_SetPos;
      pFuncTable->MouseButtonState = hb_gt_ele_mouse_ButtonState;
      pFuncTable->MouseCountButton = hb_gt_ele_mouse_CountButton;

      return HB_TRUE;
}

      /* *********************************************************************** */

#include "hbgtreg.h"

/* *********************************************************************** */
