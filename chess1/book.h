#ifndef BOOK_H
#define BOOK_H

#include <windows.h>

extern void GetOpenings(HINSTANCE hInstance);
extern void OpeningBook(WORD *hint);
extern void FreeBook();

class Book
{
public:
    Book();
};

#endif

