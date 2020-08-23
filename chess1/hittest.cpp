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

HitTest::HitTest()
{

}

void HitTest::init()
{
    POINT ptls[4];
    POINT toppt, botpt;

    for (int i = 0; i < 8; i++)
    {
        Board::QuerySqOrigin(i, 0, ptls + 0);
        Board::QuerySqOrigin(i, 8, ptls + 1);
        Board::QuerySqOrigin(i + 1, 8, ptls + 2);
        Board::QuerySqOrigin(i + 1, 0, ptls + 3);
#ifndef WINCE
        _hitrgn[i] = ::CreatePolygonRgn(ptls, 4, WINDING);
#endif
    }

    Board::QuerySqOrigin(0, 0, &botpt);
    Board::QuerySqOrigin(0, 8, &toppt);
    _deltay = botpt.y - toppt.y;
}

void HitTest::destroy()
{
    for (int i = 0; i < 8; i++)
        ::DeleteObject(_hitrgn[i]);
}

int HitTest::_horzHitTest(int x, int y)
{
    for (int i = 0; i < 8; i++)
        if (::PtInRegion(_hitrgn[i], x, y))
            return i;

    return -1;
}

int HitTest::test(int x, int y)
{
    POINT sq00;
    int xsq = _horzHitTest(x, y);

    if (xsq == -1)
        return -1;

    Board::QuerySqOrigin(0, 0, &sq00);

    if (y > sq00.y)
        return -1;

    if (y < sq00.y - _deltay)
        return -1;

    int ysq = 7 - (y - (sq00.y - _deltay)) / (_deltay / 8);
    return ysq * 8 + xsq;
}
