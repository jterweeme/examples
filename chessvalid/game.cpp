#include "game.h"
#include <cstring>

Game::Game()
{

}

void Game::reset()
{
    _board[0][0] = WHITE_ROOK;
    _board[0][1] = WHITE_KNIGHT;
    _board[0][2] = WHITE_BISHOP;
    _board[0][3] = WHITE_QUEEN;
    _board[0][4] = WHITE_KING;
    _board[0][5] = WHITE_BISHOP;
    _board[0][6] = WHITE_KNIGHT;
    _board[0][7] = WHITE_ROOK;

    _board[7][0] = BLACK_ROOK;
    _board[7][1] = BLACK_KNIGHT;
    _board[7][2] = BLACK_BISHOP;
    _board[7][3] = BLACK_QUEEN;
    _board[7][4] = BLACK_KING;
    _board[7][5] = BLACK_BISHOP;
    _board[7][6] = BLACK_KNIGHT;
    _board[7][7] = BLACK_ROOK;

    for (int i = 0; i < 8; ++i)
    {
        _board[1][i] = WHITE_PAWN;
        _board[6][i] = BLACK_PAWN;
    }

    for (int i = 2; i < 6; ++i)
        for (int j = 0; j < 8; ++j)
            _board[i][j] = EMPTY;

}

void Game::move(uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy)
{
    _board[toy][tox] = _board[fromy][fromx];
    _board[fromy][fromx] = EMPTY;
}

void Game::castling(uint8_t color, uint8_t side)
{

}

/*!
 * \brief Game::moveLong
 * \param s
 *
 * move long algebraic notation
 */
void Game::moveLong(const char *s)
{
    size_t len = strlen(s);

    if (len < 3)
        throw "invalid notation";

    //castling
    if (s[0] == '0')
    {

    }
}

uint8_t Game::getXY(uint8_t x, uint8_t y) const
{
    return _board[y][x];
}

char Game::_fenPiece(uint8_t piece)
{
    switch (piece)
    {
    case WHITE_ROOK:
        return 'R';
    case WHITE_KNIGHT:
        return 'N';
    case WHITE_BISHOP:
        return 'B';
    case WHITE_QUEEN:
        return 'Q';
    case WHITE_KING:
        return 'K';
    case WHITE_PAWN:
        return 'P';
    case BLACK_ROOK:
        return 'r';
    case BLACK_KNIGHT:
        return 'n';
    case BLACK_BISHOP:
        return 'b';
    case BLACK_QUEEN:
        return 'q';
    case BLACK_KING:
        return 'k';
    case BLACK_PAWN:
        return 'p';
    case EMPTY:
        return '.';
    }

    throw "should not got here";
}

std::string Game::_asciiPiece(uint8_t piece)
{
    switch (piece)
    {
    case WHITE_ROOK:
        return std::string("WR");
    case WHITE_KNIGHT:
        return std::string("WN");
    case WHITE_BISHOP:
        return std::string("WB");
    case WHITE_QUEEN:
        return std::string("WQ");
    case WHITE_KING:
        return std::string("WK");
    case WHITE_PAWN:
        return std::string("WP");
    case BLACK_ROOK:
        return std::string("BR");
    case BLACK_KNIGHT:
        return std::string("BN");
    case BLACK_BISHOP:
        return std::string("BB");
    case BLACK_QUEEN:
        return std::string("BQ");
    case BLACK_KING:
        return std::string("BK");
    case BLACK_PAWN:
        return std::string("BP");
    case EMPTY:
        return std::string("--");
    }

    throw "should never got here";
}

/*!
 * \brief Game::asciiBoardCM9K
 * \return
 *
 * ASCII board zoals in Chessmaster 9000:
 * twee letters voor elk schaakstuk
 */
std::string Game::asciiBoardCM9K() const
{
    std::string ret;

    for (int j = 0; j < 8; ++j)
    {
        for (int i = 0; i < 8; ++i)
        {
            const int y = 8 - j - 1;
            ret.append(_asciiPiece(_board[y][i]));
            ret.append("  ");
        }

        ret.append("\n");
    }

    return ret;
}

std::string Game::asciiBoard() const
{
    std::string ret;

    for (int j = 0; j < 8; ++j)
    {
        for (int i = 0; i < 8; ++i)
        {
            const int y = 8 - j - 1;
            ret.push_back(_fenPiece(_board[y][i]));
            ret.append("  ");
        }

        ret.append("\n");
    }

    return ret;
}

std::string Game::writeForsythe() const
{
    std::string ret;

    for (int j = 0; j < 8; ++j)
    {
        uint8_t empty = 0;

        for (int i = 0; i < 8; ++i)
        {
            const int y = 8 - j - 1;

            if (getXY(i, y) == EMPTY)
            {
                ++empty;
                continue;
            }

            if (empty > 0)
            {
                ret.push_back('0' + empty);
                empty = 0;
            }

            ret.push_back(_fenPiece(getXY(i, y)));
        }

        if (empty > 0)
            ret.push_back('0' + empty);

        if (j < 7)
            ret.push_back('/');
    }

    ret.append(" w KQkq - 0 1");

    return ret;
}

