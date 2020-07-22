#ifndef RESOURCE_H
#define RESOURCE_H
#define IDD_HEADING  200
#define IDD_OK       201
#define IDD_CANCEL   202

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

#define FILESAVE 270
#define FILEOPEN 271
#define WILDFILEOPEN 272

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

#define MENU_ID_FILE      0
#define MENU_ID_EDIT      1
#define MENU_ID_OPTIONS   2
#define MENU_ID_SKILL     3
#define MENU_ID_SIDE      4
#define MENU_ID_HINT      5
#define MENU_ID_ABORT     6
#endif

