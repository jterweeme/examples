#ifndef RESOURCE_H
#define RESOURCE_H
#include <windows.h>

#define PAWNBASE 0
#define IDB_PAWN   1
#define IDB_KNIGHT 2
#define IDB_BISHOP 3
#define IDB_ROOK   4
#define IDB_QUEEN  5
#define IDB_KING   6

#define IDB_PAWNM   7
#define IDB_KNIGHTM 8
#define IDB_BISHOPM 9
#define IDB_ROOKM   10
#define IDB_QUEENM  11
#define IDB_KINGM   12

#define IDB_PAWNO   13
#define IDB_KNIGHTO 14
#define IDB_BISHOPO 15
#define IDB_ROOKO   16
#define IDB_QUEENO  17
#define IDB_KINGO   18

#define IDD_HEADING  200
#define IDD_OK       201
#define IDD_CANCEL   202

#define IDD_INT 0x10
#define IDD_CHAR 0x11

#define IDD_FNAME 0x10
#define IDD_FPATH 0x11
#define IDD_FLIST 0x12
#define IDD_DLIST 0x13
#define IDD_BROWSE 0x14

#define BSTLINETEXT    103
#define DEPTHTEXT      105
#define NODESECTEXT    112
#define NODETEXT       107
#define POSITIONTEXT   106
#define SCORETEXT      108
#define STATS          268

#define CNT_BLACK    100
#define CNT_BLUE     101
#define CNT_GREEN    102
#define CNT_CYAN     103
#define CNT_RED      104
#define CNT_PINK     105
#define CNT_YELLOW   106
#define CNT_PALEGRAY 107
#define CNT_DARKGRAY 108
#define CNT_DARKBLUE 109
#define CNT_DARKGREEN   110
#define CNT_DARKCYAN    111
#define CNT_DARKRED     112
#define CNT_DARKPINK    113
#define CNT_BROWN       114
#define CNT_WHITE       115

#define TMDLG_MIN 300
#define TMDLG_MOV 100

#define TMDLG_600MIN  (TMDLG_MIN+600)
#define TMDLG_60MIN  (TMDLG_MIN+60)
#define TMDLG_30MIN  (TMDLG_MIN+30)
#define TMDLG_15MIN  (TMDLG_MIN+15)
#define TMDLG_5MIN   (TMDLG_MIN+5)

#define TMDLG_60MOV  (TMDLG_MOV+60)
#define TMDLG_40MOV  (TMDLG_MOV+40)
#define TMDLG_20MOV  (TMDLG_MOV+20)
#define TMDLG_10MOV  (TMDLG_MOV+10)
#define TMDLG_1MOV   (TMDLG_MOV+1)

#if 0
#define FILESAVE 270
#define FILEOPEN 271
#define WILDFILEOPEN 272
#endif

#define IDS_ILLEGALMOVE   1000     /* Illeagal move */
#define IDS_AMBIGUOUSMOVE 1001    /* Ambigous move */
#define IDS_OBAE          1002    /* Opening book allocation error */
#define IDS_OBNF          1003    /* Opening Book not found */
#define IDS_UNABLESAVE    1004    /* Unable to save game*/
#define IDS_UNABLELIST    1005    /* Unable to list game */
#define IDS_CHESS         1006    /* Chess */
#define IDS_DRAWGAME      1007    /* Draw Game */
#define IDS_YOUWIN        1008    /* You win */
#define IDS_COMPUTERWIN   1009    /* Computer wins */
#define IDS_MATESOON      1010    /* You will soon mate */
#define IDS_COMPMATE      1011    /* Computer will soon mate */
#define IDS_TTABLEAF      1012    /* ttable Allocation Failed*/
#define IDS_SQDATAAF      1013    /* sqdata Allocation Failed*/
#define IDS_HISTORYAF     1014    /* History Allocation Failed*/
#define IDS_TREEAF        1016    /* Tree Allocation Failed*/
#define IDS_GAMEAF        1017    /* Game List Allocation Failed*/
#define IDS_LOADFAILED    1018
#define IDS_SETAWIN       1019
#define IDS_SETBWIN       1020
#define IDS_SETCONTEMPT   1021
#define IDS_MAXSEARCH     1022
#define IDS_INITERROR     1023
#define IDS_UNKNOWNERR    1024
#define IDS_ALLOCMEM      1025

#define MENU_ID_FILE      0
#define MENU_ID_EDIT      1
#define MENU_ID_OPTIONS   2
#define MENU_ID_SKILL     3
#define MENU_ID_SIDE      4
#define MENU_ID_HINT      5
#define MENU_ID_ABORT     6

#define ID_ABOUT          (WM_USER+1)
#define MSG_CHESS_NEW     (WM_USER+2)
#define MSG_CHESS_LIST    (WM_USER+3)
#define MSG_CHESS_GET     (WM_USER+4)
#define MSG_CHESS_SAVE    (WM_USER+5)
#define MSG_CHESS_QUIT    (WM_USER+6)
#define MSG_CHESS_HASH    (WM_USER+7)
#define MSG_CHESS_BEEP    (WM_USER+8)
#define MSG_CHESS_BOTH    (WM_USER+9)
#define MSG_CHESS_POST    (WM_USER+10)
#define MSG_CHESS_AWIN    (WM_USER+11)
#define MSG_CHESS_BWIN    (WM_USER+12)
#define MSG_CHESS_CONTEMP (WM_USER+13)
#define MSG_CHESS_UNDO    (WM_USER+14)
#define MSG_CHESS_ABOUT   (WM_USER+15)

#define MSG_CHESS_COORD   (WM_USER+16)
#define MSG_CHESS_REVIEW  (WM_USER+17)
#define MSG_CHESS_TEST    (WM_USER+18)
#define MSG_CHESS_BOOK    (WM_USER+19)

#define MSG_CHESS_RANDOM  (WM_USER+25)
#define MSG_CHESS_EASY    (WM_USER+26)
#define MSG_CHESS_DEPTH   (WM_USER+27)
#define MSG_CHESS_REVERSE (WM_USER+28)
#define MSG_CHESS_SWITCH  (WM_USER+29)
#define MSG_CHESS_BLACK   (WM_USER+30)
#define MSG_CHESS_WHITE   (WM_USER+31)

#define MSG_EDITBOARD     (WM_USER+32)
#define ID_EDITDONE       (WM_USER+33)

#define MSG_CHESS_EDIT    (WM_USER+34)
#define MSG_CHESS_EDITDONE (WM_USER+24)

#define MSG_USER_MOVE      (WM_USER+35)
#define MSG_USER_ENTERED_MOVE (WM_USER+36)
#define MSG_COMPUTER_MOVE  (WM_USER+37)

#define MSG_CHESS_HINT     (WM_USER+38)
#define MSG_CHESS_REMOVE   (WM_USER+39)
#define MSG_CHESS_FORCE    (WM_USER+40)
#define MSG_MANUAL_ENTRY_POINT    (WM_USER+41)

#define MSG_HELP_INDEX     (WM_USER+50)
#define MSG_HELP_HELP      (WM_USER+51)

#define MSG_DESTROY     (WM_USER+60)
#define MSG_WM_COMMAND  (WM_USER+61)

#define IDM_BACKGROUND  300
#define IDM_BLACKSQUARE 301
#define IDM_WHITESQUARE 302
#define IDM_BLACKPIECE  303
#define IDM_WHITEPIECE  304
#define IDM_DEFAULT     305
#define IDM_TEXT        307

#define IDM_TIMECONTROL 306

#define IDD_ABOUT    262
#define REVIEW      263
#define NUMBERDLG   264
#define COLOR       266
#define TIMECONTROL 267
#define TEST        269
#define PAWNPROMOTE 273
#define MANUALDLG   274
#endif

