/*
  C source for GNU CHESS

  Revision: 1990-09-30

  Modified by Daryl Baker for use in MS WINDOWS environment

  Copyright (C) 1986, 1987, 1988, 1989, 1990 Free Software Foundation, Inc.
  Copyright (c) 1988, 1989, 1990  John Stanback

  This file is part of CHESS.

  CHESS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY.  No author or distributor accepts responsibility to anyone for
  the consequences of using it or for whether it serves any particular
  purpose or works at all, unless he says so in writing.  Refer to the CHESS
  General Public License for full details.

  Everyone is granted permission to copy, modify and redistribute CHESS, but
  only under the conditions described in the CHESS General Public License.
  A copy of this license is supposed to have been given to you along with
  CHESS so you can know your rights and responsibilities.  It should be in a
  file named COPYING.  Among other things, the copyright notice and this
  notice must be preserved on all copies.
*/

#include "resource.h"
#include "protos.h"
#include "globals.h"
#include "book.h"

#define MAX_BOOK_SIZE (32*1024)

static DWORD book_used = 0;
static char *xBook;

Book::Book()
{

}

void FreeBook()
{
    GlobalUnlock(hBook);
    GlobalFree(hBook);
    hBook = 0;
    book_used = 0;
}

static void *Book_alloc(DWORD size)
{
    if (book_used + size >= MAX_BOOK_SIZE)
        return (void *)0;

    char *temp = xBook + book_used;
    book_used += size;
    return temp;
}

/*
   Read in the Opening Book file and parse the algebraic notation for a move
   into an unsigned integer format indicating the from and to square. Create
   a linked list of opening lines of play, with entry->next pointing to the
   next line and entry->move pointing to a chunk of memory containing the
   moves. More Opening lines of up to 256 half moves may be added to
   gnuchess.book.
*/
void GetOpenings(HINSTANCE hInstance)
{
#ifndef WINCE
    FILE *fd;
    int c;
    struct BookEntry *entry;
    WORD mv, *mp, tmp[100];
    CHAR lpFile[_MAX_FNAME + _MAX_EXT + _MAX_DRIVE + _MAX_DIR + 1];
    ::GetModuleFileNameA(hInstance, lpFile, sizeof(lpFile));
    CHAR sFname[_MAX_FNAME], sExt[_MAX_EXT], sDrive[_MAX_DRIVE], sDir[_MAX_DIR];
    _splitpath(lpFile, sDrive, sDir, sFname, sExt);
    _makepath(lpFile, sDrive, sDir, "gnuchess", "boo");
    fd = fopen(lpFile, "r");

    if (fd == NULL)
        throw UINT(IDS_OBNF);

    hBook = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, MAX_BOOK_SIZE * sizeof(char));

    if (hBook == NULL)
    {
        Book = NULL;
        throw TEXT("Opening Book allocation error");
    }

    xBook = (char *)(GlobalLock(hBook));
    Book = NULL;
    int i = 0;
    int side = white;

    while ((c = parse(fd, &mv, side)) >= 0)
    {
        if (c == 1)
        {
            tmp[++i] = mv;
            side = otherside[side];
        }
        else if (c == 0 && i > 0)
        {
            entry = (struct BookEntry *)(Book_alloc(sizeof(struct BookEntry)));
            mp = LPWORD(Book_alloc((i + 1) * sizeof(WORD)));

            if (entry == 0 || mp == 0)
            {
                Book = NULL;
                GlobalUnlock(hBook);
                GlobalFree(hBook);
                throw TEXT("Opening book allocation error");
            }

            entry->mv = mp;
            entry->next = Book;
            Book = entry;

            for (int j = 1; j <= i; j++)
                *(mp++) = tmp[j];

            *mp = 0;
            i = 0;
            side = white;
        }
    }

    fclose(fd);
#endif
}

/*
  Go thru each of the opening lines of play and check for a match with the
  current game listing. If a match occurs, generate a random number. If this
  number is the largest generated so far then the next move in this line
  becomes the current "candidate". After all lines are checked, the
  candidate move is put at the top of the Tree[] array and will be played by
  the program. Note that the program does not handle book transpositions.
*/
void OpeningBook(WORD *hint)
{
    short j, pnt;
    WORD m, *mp;
    DWORD r, r0;
#if 0
    srand((unsigned int) time ((long *) 0));
#endif
    r0 = m = 0;
    BookEntry *p = Book;

    while (p != NULL)
    {
        mp = p->mv;

        for (j = 1; j <= GameCnt; j++)
            if (GameList[j].gmove != *(mp++))
                break;

        if (j > GameCnt)
        {
            if ((r = ::rand()) > r0)
            {
                r0 = r;
                m = *mp;
                *hint = *(++mp);
            }
        }

        p = p->next;
    }

    for (pnt = TrPnt[1]; pnt < TrPnt[2]; pnt++)
        if (((Tree[pnt].f << 8) | Tree[pnt].t) == m)
            Tree[pnt].score = 0;

    pick (TrPnt[1], TrPnt[2] - 1);

    if (Tree[TrPnt[1]].score < 0)
    {
        FreeBook();
        Book = NULL;
    }
}
