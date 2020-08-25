/*
  C source for GNU CHESS

  Revision: 1990-09-30

  Modified by Daryl Baker for use in MS WINDOWS environment

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

#include "hittest.h"
#include "board.h"
#include <iostream>

AbstractHitTest::~AbstractHitTest()
{

}

#ifdef WINCE
void HitTestCE::init(HDC)
{
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            POINT aptl[4];
            Board::QuerySqCoords(x, y, aptl);

            //voor windows ce gebruiken we rechthoekige hitboxes die binnen de square passen
            _hitrgn[y << 3 | x] = CreateRectRgn(aptl[3].x, aptl[3].y, aptl[1].x, aptl[1].y);
        }
    }
}

void HitTestCE::destroy()
{

}

int HitTestCE::test(int x, int y, HDC)
{
    for (int i = 0; i < 64; ++i)
        if (PtInRegion(_hitrgn[i], x, y))
            return i;

    return -1;
}
#else
void HitTest::init(HDC)
{
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            POINT aptl[4];
            Board::QuerySqCoords(x, y, aptl);
            _hitrgn[y << 3 | x] = ::CreatePolygonRgn(aptl, 4, WINDING);
        }
    }
}

void HitTest::destroy()
{
    for (int i = 0; i < 64; ++i)
        ::DeleteObject(_hitrgn[i]);
}

//HDC tijdelijk voor debuggen
int HitTest::test(int x, int y, HDC)
{
    for (int i = 0; i < 64; ++i)
        if (PtInRegion(_hitrgn[i], x, y))
            return i;

    return -1;
}
#endif


