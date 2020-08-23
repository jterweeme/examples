#ifndef SIM_H
#define SIM_H

#include <windows.h>

class Sim
{
private:
    GLOBALHANDLE hGameList, hTree, hHistory, hTTable, hHashCode, hdistdata;
    GLOBALHANDLE htaxidata, hnextdir, hnextpos;

    static short const max_steps[8];
    static short const direc[8][8];
    static short const nunmap[120];
    static short const Stcolor[64];
    short _maxSearchDepth;

    static void Initialize_dist();
    static void Initialize_moves();
public:
    Sim();
    void init_main();
    void FreeGlobals();
    void NewGame(HWND hWnd, HWND compClr);
    short maxSearchDepth() const;
    void maxSearchDepth(short n);
};

#endif

