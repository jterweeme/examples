#ifndef SIM_H
#define SIM_H

#include <windows.h>

class Sim
{
private:
    GLOBALHANDLE hGameList, hTree, hHistory, hTTable, hHashCode, hdistdata;
    GLOBALHANDLE htaxidata, hnextdir, hnextpos;
    static void Initialize_dist();
    static void Initialize_moves();
    short _maxSearchDepth;
public:
    Sim();
    void init_main();
    void FreeGlobals();
    void NewGame(HWND hWnd, HWND compClr);
    short maxSearchDepth() const;
    void maxSearchDepth(short n);
};

#endif

