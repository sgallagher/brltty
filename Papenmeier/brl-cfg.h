/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * Support for all Papenmeier Terminal + config file
 *   Heimo.Sch�n <heimo.schoen@gmx.at>
 *   August H�randl <august.hoerandl@gmx.at>
 *
 * Papenmeier/brl-cfg.h
 *  some defines and the big config table
 */

 /* supported terminals:
   id   name                     help  x  y  stat frnt easy
    0, "BRAILLEX Compact 486", 	  '1', 40, 1,  0,  9,  0
    1, "BRAILLEX 2D Lite (plus)", '2', 40, 1, 13,  9,  0
    2, "BRAILLEX Compact/Tiny",	  '3', 40, 1,  0,  9,  0
    3, "BRAILLEX 2D Screen Soft", '4', 80, 1, 22, 13,  0
    6, "BRAILLEX IB 80 cr soft",  '5', 80, 1,  4,  9,  0
   64, "BRAILLEX EL 2D-40",	  '6', 40, 1, 13,  0,  1
   65, "BRAILLEX EL 2D-66",	  '7', 66, 1, 13,  0,  1
   66, "BRAILLEX EL 80 (IB)",	  '8', 80, 1,  2,  0,  1
   67, "BRAILLEX EL 2D-80",	  '8', 80, 1,  20, 0,  1
   68, "BRAILLEX EL 40 P",	  '8', 40, 1,  0,  0,  1
   69, "BRAILLEX Elba 32",	  '8', 32, 1,  0,  0,  1
   70, "BRAILLEX Elba 20",	  '8', 20, 1,  0,  0,  1
 */

#define OFFS_FRONT      0
#define OFFS_STAT    1000
#define OFFS_ROUTE   2000  
#define OFFS_EASY    3000
#define OFFS_SWITCH  4000
#define OFFS_CR      5000

#define ROUTINGKEY      -9999  /* virtual routing key */
#define NOKEY           -1

#define KEYMAX 8

#define  STATMAX   22
#define  FRONTMAX  13
#define  EASYMAX   8
#define  NAMEMAX   80
#define  MODMAX    16
#define  CMDMAX    120
#define  INPUTSPECMAX 20
#define  HELPLEN   80

#define STAT_EMPTY       0
#define OFFS_HORIZ    1000	/* added to status show */
#define OFFS_FLAG     2000
#define OFFS_NUMBER   3000

/* easy bar - code as delivered by the terminals */
#define EASY_LE   1
#define EASY_LE2  2    
#define EASY_UP   3 
#define EASY_UP2  4
#define EASY_RI   5 
#define EASY_RI2  6  
#define EASY_DO   7 
#define EASY_DO2  8  

typedef struct {
  int code;			/* the code to sent */
  int keycode;			/* the key to press */
  int modifiers;	/* modifiers (bitfield) */
} commands;

typedef struct {
  unsigned char ident;		/* identity of terminal */
  char name[NAMEMAX];		/* name of terminal */
  char helpfile[HELPLEN];	/* filename of local helpfile */

  int x, y;			/* size of display */
  int statcells;		/* number of status cells */
  int frontkeys;		/* number of frontkeys */
  int haseasybar;		/* terminal has an easy bar */

  int statshow[STATMAX];	/* status cells: info to show */
  int modifiers[MODMAX];	/* keys used as modifier */
  commands cmds[CMDMAX];

} one_terminal; 

/* some macros for terminals with the same layout -
 * named after there usage
 */

#define CHGONOFF(cmd, offs, on, off) \
      { cmd                , offs, 0  }, \
      { cmd | VAL_SWITCHON , offs, on }, \
      { cmd | VAL_SWITCHOFF, offs, off}


/* modifiers for 9 front keys */
#define MOD_FRONT_9 \
     OFFS_FRONT + 1, \
     OFFS_FRONT + 9, \
     OFFS_FRONT + 2, \
     OFFS_FRONT + 8

/* commands for 9 front keys */
#define CMD_FRONT_9 \
     { CMD_FWINLT     , NOKEY         , 0X1 }, \
     { CMD_FWINRT     , NOKEY         , 0X2 }, \
     { CMD_HWINLT     , NOKEY         , 0X4 }, \
     { CMD_HWINRT     , NOKEY         , 0X8 }, \
                                               \
     { CMD_HOME       , OFFS_FRONT + 5, 0X0 }, \
     { CMD_LNBEG      , OFFS_FRONT + 5, 0X1 }, \
     { CMD_LNEND      , OFFS_FRONT + 5, 0X2 }, \
     { CMD_CHRLT      , OFFS_FRONT + 5, 0X4 }, \
     { CMD_CHRRT      , OFFS_FRONT + 5, 0X8 }, \
                                               \
     { CMD_WINUP      , OFFS_FRONT + 4, 0X0 }, \
     { CMD_PRDIFLN    , OFFS_FRONT + 4, 0X1 }, \
     { CMD_ATTRUP     , OFFS_FRONT + 4, 0X2 }, \
     { CMD_PRPGRPH    , OFFS_FRONT + 4, 0X4 }, \
     { CMD_PRSEARCH   , OFFS_FRONT + 4, 0X8 }, \
                                               \
     { CMD_WINDN      , OFFS_FRONT + 6, 0X0 }, \
     { CMD_NXDIFLN    , OFFS_FRONT + 6, 0X1 }, \
     { CMD_ATTRDN     , OFFS_FRONT + 6, 0X2 }, \
     { CMD_NXPGRPH    , OFFS_FRONT + 6, 0X4 }, \
     { CMD_NXSEARCH   , OFFS_FRONT + 6, 0X8 }, \
                                               \
     { CMD_LNUP       , OFFS_FRONT + 3, 0X0 }, \
     { CMD_TOP_LEFT   , OFFS_FRONT + 3, 0X1 }, \
     { CMD_TOP        , OFFS_FRONT + 3, 0X2 }, \
                                               \
     { CMD_LNDN       , OFFS_FRONT + 7, 0X0 }, \
     { CMD_BOT_LEFT   , OFFS_FRONT + 7, 0X1 }, \
     { CMD_BOT        , OFFS_FRONT + 7, 0X2 }, \
                                               \
     { CR_ROUTE       , ROUTINGKEY    , 0X0 }, \
     { CR_CUTBEGIN    , ROUTINGKEY    , 0X1 }, \
     { CR_CUTRECT     , ROUTINGKEY    , 0X2 }, \
     { CR_PRINDENT    , ROUTINGKEY    , 0X4 }, \
     { CR_NXINDENT    , ROUTINGKEY    , 0X8 }, \
     { CMD_PASTE      , NOKEY         , 0X3 }


/* modifiers for 13 front keys */
#define MOD_FRONT_13  \
     OFFS_FRONT +  4, \
     OFFS_FRONT +  3, \
     OFFS_FRONT +  2, \
     OFFS_FRONT + 10, \
     OFFS_FRONT + 11, \
     OFFS_FRONT + 12, \
     OFFS_FRONT +  1, \
     OFFS_FRONT + 13

/* commands for 13 front keys */
#define CMD_FRONT_13 \
      { CMD_HOME                    , OFFS_FRONT +  7, 0000 }, \
      { CMD_TOP                     , OFFS_FRONT +  6, 0000 }, \
      { CMD_BOT                     , OFFS_FRONT +  8, 0000 }, \
      { CMD_LNUP                    , OFFS_FRONT +  5, 0000 }, \
      { CMD_LNDN                    , OFFS_FRONT +  9, 0000 }, \
                                                               \
      { CMD_PRDIFLN                 , NOKEY          , 0001 }, \
      { CMD_NXDIFLN                 , NOKEY          , 0010 }, \
      { CMD_WINUP                   , NOKEY          , 0002 }, \
      { CMD_WINDN                   , NOKEY          , 0020 }, \
      { CMD_ATTRUP                  , NOKEY          , 0004 },         \
      { CMD_ATTRDN                  , NOKEY          , 0040 },        \
      { CMD_PRPROMPT                , NOKEY          , 0100 },        \
      { CMD_NXPROMPT                , NOKEY          , 0200 },        \
                                                               \
      { CMD_PRPGRPH                 , NOKEY          , 0003 },        \
      { CMD_NXPGRPH                 , NOKEY          , 0030 },        \
      { CMD_PRSEARCH                , NOKEY          , 0104 },        \
      { CMD_NXSEARCH                , NOKEY          , 0240 },        \
      { CR_PRINDENT                 , ROUTINGKEY     , 0104 },        \
      { CR_NXINDENT                 , ROUTINGKEY     , 0240 },        \
                                                               \
      { CMD_LNBEG                   , OFFS_FRONT +  7, 0001 }, \
      { CMD_TOP_LEFT                , OFFS_FRONT +  6, 0001 }, \
      { CMD_BOT_LEFT                , OFFS_FRONT +  8, 0001 }, \
      { CMD_FWINLT                  , OFFS_FRONT +  5, 0001 }, \
      { CMD_FWINRT                  , OFFS_FRONT +  9, 0001 }, \
      { CR_DESCCHAR                 , ROUTINGKEY     , 0001 }, \
                                                               \
      { CMD_LNEND                   , OFFS_FRONT +  7, 0010 }, \
      { CMD_CHRLT                   , OFFS_FRONT +  6, 0010 }, \
      { CMD_CHRRT                   , OFFS_FRONT +  8, 0010 }, \
      { CMD_HWINLT                  , OFFS_FRONT +  5, 0010 }, \
      { CMD_HWINRT                  , OFFS_FRONT +  9, 0010 }, \
      { CR_SETLEFT                  , ROUTINGKEY     , 0010 }, \
                                                               \
      { VAL_PASSKEY+VPK_INSERT      , OFFS_FRONT +  7, 0002 }, \
      { VAL_PASSKEY+VPK_PAGE_UP     , OFFS_FRONT +  6, 0002 }, \
      { VAL_PASSKEY+VPK_PAGE_DOWN   , OFFS_FRONT +  8, 0002 }, \
      { VAL_PASSKEY+VPK_CURSOR_UP   , OFFS_FRONT +  5, 0002 }, \
      { VAL_PASSKEY+VPK_CURSOR_DOWN , OFFS_FRONT +  9, 0002 }, \
      { CR_SWITCHVT                 , ROUTINGKEY     , 0002 }, \
                                                               \
      { VAL_PASSKEY+VPK_DELETE      , OFFS_FRONT +  7, 0020 }, \
      { VAL_PASSKEY+VPK_HOME        , OFFS_FRONT +  6, 0020 }, \
      { VAL_PASSKEY+VPK_END         , OFFS_FRONT +  8, 0020 }, \
      { VAL_PASSKEY+VPK_CURSOR_LEFT , OFFS_FRONT +  5, 0020 }, \
      { VAL_PASSKEY+VPK_CURSOR_RIGHT, OFFS_FRONT +  9, 0020 }, \
      { VAL_PASSKEY+VPK_FUNCTION    , ROUTINGKEY     , 0020 }, \
                                                               \
      { CMD_NODOTS                  , OFFS_FRONT +  7, 0004 }, \
      { VAL_PASSKEY+VPK_ESCAPE      , OFFS_FRONT +  6, 0004 }, \
      { VAL_PASSKEY+VPK_TAB         , OFFS_FRONT +  8, 0004 }, \
      { VAL_PASSKEY+VPK_BACKSPACE   , OFFS_FRONT +  5, 0004 }, \
      { VAL_PASSKEY+VPK_RETURN      , OFFS_FRONT +  9, 0004 }, \
                                                               \
      { CMD_SPKHOME                 , OFFS_FRONT +  7, 0040 }, \
      { CMD_MUTE                    , OFFS_FRONT +  6, 0040 }, \
      { CMD_RESTARTSPEECH           , OFFS_FRONT +  8, 0040 }, \
      { CMD_SAY                     , OFFS_FRONT +  5, 0040 }, \
      { CMD_SAYALL                  , OFFS_FRONT +  9, 0040 }, \
                                                               \
      { CR_CUTBEGIN                 , ROUTINGKEY     , 0100 }, \
      { CR_CUTAPPEND                , ROUTINGKEY     , 0004 }, \
      { CR_CUTLINE                  , ROUTINGKEY     , 0040 }, \
      { CR_CUTRECT                  , ROUTINGKEY     , 0200 }, \
                                                               \
      { CR_ROUTE                    , ROUTINGKEY     , 0000 }
	

/* modifiers for switches */
#define MOD_EASY \
     OFFS_EASY   + EASY_UP, \
     OFFS_EASY   + EASY_DO, \
     OFFS_EASY   + EASY_LE, \
     OFFS_EASY   + EASY_RI, \
     OFFS_SWITCH + 1      , \
     OFFS_SWITCH + 2      , \
     OFFS_SWITCH + 7      , \
     OFFS_SWITCH + 8

/* commands for easy bar + switches */
#define CMD_EASY \
     { CMD_LNUP                    , NOKEY               , 0X01 }, \
     { CMD_TOP                     , OFFS_EASY + EASY_UP2, 0X01 }, \
     { CMD_LNDN                    , NOKEY               , 0X02 }, \
     { CMD_BOT                     , OFFS_EASY + EASY_DO2, 0X02 }, \
     { CMD_FWINLT                  , NOKEY               , 0X04 }, \
     { CMD_LNBEG                   , OFFS_EASY + EASY_LE2, 0X04 }, \
     { CMD_FWINRT                  , NOKEY               , 0X08 }, \
     { CMD_LNEND                   , OFFS_EASY + EASY_RI2, 0X08 }, \
     { CMD_BACK                    , OFFS_SWITCH + 3     , 0X00 }, \
     { CMD_HOME                    , OFFS_SWITCH + 4     , 0X00 }, \
     { CMD_CSRJMP_VERT             , OFFS_SWITCH + 5     , 0X00 }, \
     { CMD_PASTE                   , OFFS_SWITCH + 6     , 0X00 }, \
                                                                   \
     { VAL_PASSKEY+VPK_CURSOR_UP   , NOKEY               , 0X11 }, \
     { VAL_PASSKEY+VPK_PAGE_UP     , OFFS_EASY + EASY_UP2, 0X11 }, \
     { VAL_PASSKEY+VPK_CURSOR_DOWN , NOKEY               , 0X12 }, \
     { VAL_PASSKEY+VPK_PAGE_DOWN   , OFFS_EASY + EASY_DO2, 0X12 }, \
                                                                   \
     { VAL_PASSKEY+VPK_CURSOR_UP   , NOKEY               , 0X11 }, \
     { VAL_PASSKEY+VPK_PAGE_UP     , OFFS_EASY + EASY_UP2, 0X11 }, \
     { VAL_PASSKEY+VPK_CURSOR_DOWN , NOKEY               , 0X12 }, \
     { VAL_PASSKEY+VPK_PAGE_DOWN   , OFFS_EASY + EASY_DO2, 0X12 }, \
     { VAL_PASSKEY+VPK_CURSOR_LEFT , NOKEY               , 0X14 }, \
     { VAL_PASSKEY+VPK_HOME        , OFFS_EASY + EASY_LE2, 0X14 }, \
     { VAL_PASSKEY+VPK_CURSOR_RIGHT, NOKEY               , 0X18 }, \
     { VAL_PASSKEY+VPK_END         , OFFS_EASY + EASY_RI2, 0X18 }, \
                                                                   \
     { CMD_PRDIFLN                 , NOKEY               , 0X21 }, \
     { CMD_ATTRUP                  , OFFS_EASY + EASY_UP2, 0X21 }, \
     { CMD_NXDIFLN                 , NOKEY               , 0X22 }, \
     { CMD_ATTRDN                  , OFFS_EASY + EASY_DO2, 0X22 }, \
     { CMD_CHRLT                   , NOKEY               , 0X24 }, \
     { CMD_HWINLT                  , OFFS_EASY + EASY_LE2, 0X24 }, \
     { CMD_CHRRT                   , NOKEY               , 0X28 }, \
     { CMD_HWINRT                  , OFFS_EASY + EASY_RI2, 0X28 }, \
                                                                   \
     { CR_ROUTE                    , ROUTINGKEY          , 0X00 }, \
     { CR_PRINDENT                 , ROUTINGKEY          , 0X01 }, \
     { CR_NXINDENT                 , ROUTINGKEY          , 0X02 }, \
     { CR_CUTBEGIN                 , ROUTINGKEY          , 0X04 }, \
     { CR_CUTRECT                  , ROUTINGKEY          , 0X08 }


/* what to show for 2 status cells */
#define SHOW_STAT_2 \
      OFFS_NUMBER + STAT_BRLROW, \
      OFFS_NUMBER + STAT_CSRROW

/* commands for 2 status keys */
#define CMD_STAT_2 \
      { CMD_HELP,       OFFS_STAT + 1, 0 }, \
      { CMD_RESTARTBRL, OFFS_STAT + 2, 0 }


/* what to show for 4 status cells */
#define SHOW_STAT_4 \
      OFFS_NUMBER + STAT_BRLROW, \
      OFFS_NUMBER + STAT_CSRROW, \
      OFFS_NUMBER + STAT_CSRCOL, \
      OFFS_FLAG   + STAT_DISPMD

/* commands for 4 status keys */
#define CMD_STAT_4 \
      { CMD_HELP       , OFFS_STAT + 1, 0 }, \
      { CMD_RESTARTBRL , OFFS_STAT + 2, 0 }, \
      { CMD_CSRJMP_VERT, OFFS_STAT + 3, 0 }, \
      { CMD_DISPMD     , OFFS_STAT + 4, 0 }


/* what to show for 13 status cells */
#define SHOW_STAT_13 \
      OFFS_HORIZ + STAT_BRLROW  , \
      STAT_EMPTY                , \
      OFFS_HORIZ + STAT_CSRROW  , \
      OFFS_HORIZ + STAT_CSRCOL  , \
      STAT_EMPTY                , \
      OFFS_FLAG  + STAT_CSRTRK  , \
      OFFS_FLAG  + STAT_DISPMD  , \
      OFFS_FLAG  + STAT_FREEZE  , \
      STAT_EMPTY                , \
      STAT_EMPTY                , \
      OFFS_FLAG  + STAT_CSRVIS  , \
      OFFS_FLAG  + STAT_ATTRVIS , \
      STAT_EMPTY

/* commands for 13 status keys */
#define CMD_STAT_13 \
      CHGONOFF( CMD_HELP       , OFFS_STAT +  1, 2, 1), \
              { CMD_RESTARTBRL , OFFS_STAT +  2, 0   }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0   }, \
              { CMD_BACK       , OFFS_STAT +  4, 0   }, \
      CHGONOFF( CMD_INFO       , OFFS_STAT +  5, 2, 1), \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, 2, 1), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, 2, 1), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  8, 2, 1), \
              { CMD_PREFMENU   , OFFS_STAT +  9, 0   }, \
              { CMD_PREFSAVE   , OFFS_STAT + 10, 0   }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 11, 2, 1), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 12, 2, 1), \
              { CMD_PASTE      , OFFS_STAT + 13, 0   }


/* what to show for 20 status cells */
#define SHOW_STAT_20 \
      OFFS_HORIZ + STAT_BRLROW   , \
      STAT_EMPTY                 , \
      OFFS_HORIZ + STAT_CSRROW   , \
      OFFS_HORIZ + STAT_CSRCOL   , \
      STAT_EMPTY                 , \
      OFFS_FLAG  + STAT_CSRTRK   , \
      OFFS_FLAG  + STAT_DISPMD   , \
      OFFS_FLAG  + STAT_FREEZE   , \
      STAT_EMPTY                 , \
      OFFS_HORIZ + STAT_SCRNUM   , \
      STAT_EMPTY                 , \
      OFFS_FLAG  + STAT_CSRVIS   , \
      OFFS_FLAG  + STAT_CSRSIZE  , \
      OFFS_FLAG  + STAT_CSRBLINK , \
      OFFS_FLAG  + STAT_CAPBLINK , \
      OFFS_FLAG  + STAT_SIXDOTS  , \
      OFFS_FLAG  + STAT_TUNES    , \
      OFFS_FLAG  + STAT_SKPIDLNS , \
      OFFS_FLAG  + STAT_ATTRVIS  , \
      OFFS_FLAG  + STAT_ATTRBLINK

/* commands for 20 status keys */
#define CMD_STAT_20 \
              { CMD_HELP       , OFFS_STAT +  1, 0000       }, \
              { CMD_RESTARTBRL , OFFS_STAT +  2, 0000       }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0000       }, \
              { CMD_BACK       , OFFS_STAT +  4, 0000       }, \
              { CMD_INFO       , OFFS_STAT +  5, 0000       }, \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, 0200, 0100 ), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, 0200, 0100 ), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  8, 0200, 0100 ), \
              { CMD_PREFMENU   , OFFS_STAT +  9, 0000       }, \
              { CMD_PREFSAVE   , OFFS_STAT + 10, 0000       }, \
              { CMD_PREFLOAD   , OFFS_STAT + 11, 0000       }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 12, 0200, 0100 ), \
      CHGONOFF( CMD_CSRSIZE    , OFFS_STAT + 13, 0200, 0100 ), \
      CHGONOFF( CMD_CSRBLINK   , OFFS_STAT + 14, 0200, 0100 ), \
      CHGONOFF( CMD_CAPBLINK   , OFFS_STAT + 15, 0200, 0100 ), \
      CHGONOFF( CMD_SIXDOTS    , OFFS_STAT + 16, 0200, 0100 ), \
      CHGONOFF( CMD_TUNES      , OFFS_STAT + 17, 0200, 0100 ), \
      CHGONOFF( CMD_SKPIDLNS   , OFFS_STAT + 18, 0200, 0100 ), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 19, 0200, 0100 ), \
      CHGONOFF( CMD_ATTRBLINK  , OFFS_STAT + 20, 0200, 0100 )


/* what to show for 22 status cells */
#define SHOW_STAT_22 \
      OFFS_HORIZ + STAT_BRLROW   , \
      STAT_EMPTY                 , \
      OFFS_HORIZ + STAT_CSRROW   , \
      OFFS_HORIZ + STAT_CSRCOL   , \
      STAT_EMPTY                 , \
      OFFS_FLAG  + STAT_CSRTRK   , \
      OFFS_FLAG  + STAT_DISPMD   , \
      OFFS_FLAG  + STAT_INPUT    , \
      OFFS_FLAG  + STAT_FREEZE   , \
      STAT_EMPTY                 , \
      OFFS_HORIZ + STAT_SCRNUM   , \
      STAT_EMPTY                 , \
      OFFS_FLAG  + STAT_CSRVIS   , \
      OFFS_FLAG  + STAT_CSRSIZE  , \
      OFFS_FLAG  + STAT_CSRBLINK , \
      OFFS_FLAG  + STAT_CAPBLINK , \
      OFFS_FLAG  + STAT_SIXDOTS  , \
      OFFS_FLAG  + STAT_TUNES    , \
      OFFS_FLAG  + STAT_SKPIDLNS , \
      OFFS_FLAG  + STAT_ATTRVIS  , \
      OFFS_FLAG  + STAT_ATTRBLINK, \
      STAT_EMPTY

/* commands for 22 status keys */
#define CMD_STAT_22 \
      CHGONOFF( CMD_HELP       , OFFS_STAT +  1, 0200, 0100 ), \
              { CMD_RESTARTBRL , OFFS_STAT +  2, 0000       }, \
              { CMD_CSRJMP_VERT, OFFS_STAT +  3, 0000       }, \
              { CMD_BACK       , OFFS_STAT +  4, 0000       }, \
      CHGONOFF( CMD_INFO       , OFFS_STAT +  5, 0200, 0100 ), \
      CHGONOFF( CMD_CSRTRK     , OFFS_STAT +  6, 0200, 0100 ), \
      CHGONOFF( CMD_DISPMD     , OFFS_STAT +  7, 0200, 0100 ), \
      CHGONOFF( CMD_INPUT      , OFFS_STAT +  8, 0200, 0100 ), \
      CHGONOFF( CMD_FREEZE     , OFFS_STAT +  9, 0200, 0100 ), \
              { CMD_PREFMENU   , OFFS_STAT + 10, 0000       }, \
              { CMD_PREFSAVE   , OFFS_STAT + 11, 0000       }, \
              { CMD_PREFLOAD   , OFFS_STAT + 12, 0000       }, \
      CHGONOFF( CMD_CSRVIS     , OFFS_STAT + 13, 0200, 0100 ), \
      CHGONOFF( CMD_CSRSIZE    , OFFS_STAT + 14, 0200, 0100 ), \
      CHGONOFF( CMD_CSRBLINK   , OFFS_STAT + 15, 0200, 0100 ), \
      CHGONOFF( CMD_CAPBLINK   , OFFS_STAT + 16, 0200, 0100 ), \
      CHGONOFF( CMD_SIXDOTS    , OFFS_STAT + 17, 0200, 0100 ), \
      CHGONOFF( CMD_TUNES      , OFFS_STAT + 18, 0200, 0100 ), \
      CHGONOFF( CMD_SKPIDLNS   , OFFS_STAT + 19, 0200, 0100 ), \
      CHGONOFF( CMD_ATTRVIS    , OFFS_STAT + 20, 0200, 0100 ), \
      CHGONOFF( CMD_ATTRBLINK  , OFFS_STAT + 21, 0200, 0100 ), \
              { CMD_PASTE      , OFFS_STAT + 22, 0000       }


static one_terminal pm_terminals[] =
{
  {
    0,				/* identity */
    "BrailleX Compact 486",	/* name of terminal */
    "brltty-pm-c-486.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    0,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9
    },
  },

  {
    1,				/* identity */
    "BrailleX 2D Lite (plus)",	/* name of terminal */
    "brltty-pm-2d-l.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    13,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_13,
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9,
      CMD_STAT_13
    },
  },

  {
    2,				/* identity */
    "BrailleX Compact/Tiny",	/* name of terminal */
    "brltty-pm-c.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    0,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9
    },
  },

  {
    3,				/* identity */
    "BrailleX 2D Screen Soft", /* name of terminal */
    "brltty-pm-2d-s.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    22,				/* number of status cells */
    13,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {
      SHOW_STAT_22
    },
    {				/* modifiers */
      MOD_FRONT_13
    },
    {				/* commands + keys */
      CMD_FRONT_13,
      CMD_STAT_22
    },
  },

  {
    6,				/* identity */
    "BrailleX IB 80 CR Soft",	/* name of terminal */
    "brltty-pm-ib-80.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    4,				/* number of status cells */
    9,				/* number of frontkeys */
    0,				/* terminal has an easy bar */
    {
      SHOW_STAT_4
    },
    {				/* modifiers */
      MOD_FRONT_9
    },
    {				/* commands + keys */
      CMD_FRONT_9,
      CMD_STAT_4
    },
  },

  {
    64,				/* identity */
    "BrailleX EL 2D-40",	/* name of terminal */
    "brltty-pm-el-2d-40.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    13,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {
      SHOW_STAT_13
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_STAT_13,
      CMD_EASY
    },
  },

  {
    65,				/* identity */
    "BrailleX EL 2D-66",	/* name of terminal */
    "brltty-pm-el-2d-66.hlp",		/* filename of local helpfile */
    66, 1,			/* size of display */
    13,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_13
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_STAT_13,
      CMD_EASY
    },
  },

  {
    66,				/* identity */
    "BrailleX EL 80",		/* name of terminal */
    "brltty-pm-el-80.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    2,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_2
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_EASY,
      CMD_STAT_2
    },
  },

  {
    67,				/* identity */
    "BrailleX EL 2D-80",		/* name of terminal */
    "brltty-pm-el-2d-80.hlp",		/* filename of local helpfile */
    80, 1,			/* size of display */
    20,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
      SHOW_STAT_20
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_EASY,
      CMD_STAT_20
    },
  },

  {
    68,				/* identity */
    "BrailleX EL 40 P",		/* name of terminal */
    "brltty-pm-el-40-p.hlp",		/* filename of local helpfile */
    40, 1,			/* size of display */
    0,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_EASY
    },
  },

  {
    69,				/* identity */
    "BrailleX Elba 32",		/* name of terminal */
    "brltty-pm-elba-32.hlp",		/* filename of local helpfile */
    32, 1,			/* size of display */
    0,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_EASY
    },
  },

  {
    70,				/* identity */
    "BrailleX Elba 20",		/* name of terminal */
    "brltty-pm-elba-20.hlp",		/* filename of local helpfile */
    20, 1,			/* size of display */
    0,				/* number of status cells */
    0,				/* number of frontkeys */
    1,				/* terminal has an easy bar */
    {				/* status cells: info to show */
    },
    {				/* modifiers */
      MOD_EASY
    },
    {				/* commands + keys */
      CMD_EASY
    },
  },
};

static const int num_terminals = sizeof(pm_terminals)/sizeof(pm_terminals[0]);