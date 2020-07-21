#include "toolbar.h"
#include "mdi_unit.h"
#include <commctrl.h>

Toolbar::Toolbar(HWND parent) : Element(parent)
{
}

void Toolbar::create(HINSTANCE hInst)
{
    _hwnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                    WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                    _parent, HMENU(ID_TOOLBAR), hInst, NULL);

    // Send the TB_BUTTONSTRUCTSIZE message, which is required for
    // backward compatibility.
    sendMsgW(TB_BUTTONSTRUCTSIZE, WPARAM(sizeof(TBBUTTON)), 0);

    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = IDB_STD_SMALL_COLOR;
    sendMsgW(TB_ADDBITMAP, 0, LPARAM(&tbab));

    ZeroMemory(tbb, sizeof(tbb));

    tbb[0].iBitmap = STD_FILENEW;
    tbb[0].fsState = TBSTATE_ENABLED;
    tbb[0].fsStyle = TBSTYLE_BUTTON;
    tbb[0].idCommand = CM_FILE_NEW;

    tbb[1].iBitmap = STD_FILEOPEN;
    tbb[1].fsState = TBSTATE_ENABLED;
    tbb[1].fsStyle = TBSTYLE_BUTTON;
    tbb[1].idCommand = CM_FILE_OPEN;

    tbb[2].iBitmap = STD_FILESAVE;
    tbb[2].fsStyle = TBSTYLE_BUTTON;
    tbb[2].idCommand = CM_FILE_SAVE;

    tbb[3].fsStyle = TBSTYLE_SEP;

    tbb[4].iBitmap = STD_CUT;
    tbb[4].fsStyle = TBSTYLE_BUTTON;
    tbb[4].idCommand = CM_EDIT_CUT;

    tbb[5].iBitmap = STD_COPY;
    tbb[5].fsStyle = TBSTYLE_BUTTON;
    tbb[5].idCommand = CM_EDIT_COPY;

    tbb[6].iBitmap = STD_PASTE;
    tbb[6].fsStyle = TBSTYLE_BUTTON;
    tbb[6].idCommand = CM_EDIT_PASTE;

    tbb[7].fsStyle = TBSTYLE_SEP;

    tbb[8].iBitmap = STD_UNDO;
    tbb[8].fsStyle = TBSTYLE_BUTTON;
    tbb[8].idCommand = CM_EDIT_UNDO;
    sendMsgW(TB_ADDBUTTONS, 9, LPARAM(&tbb));
}
