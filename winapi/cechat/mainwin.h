#ifndef MAINWIN_H
#define MAINWIN_H
#include <windows.h>

class AbstractMenuBar;
class CommandBar;

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd, _hC1, _hC2, _hBtn;
    AbstractMenuBar *_menu;
    CommandBar *_cmdBar;
    static void DoSizeMain(HWND hWnd);
    void _createProc(HWND hWnd, UINT, WPARAM, LPARAM lParam);
    static void _comPortComboProc(HWND hWnd, WORD, HWND hwndCtl, WORD wNotifyCode);
    LRESULT _commandProc(HWND hWnd, UINT, WPARAM wParam, LPARAM lParam);
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    HWND hwnd() const;
    void show(int nCmdShow) const;
    void update() const;
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

