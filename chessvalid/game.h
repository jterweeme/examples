#ifndef GAME_H
#define GAME_H

#include <string>

class Game
{
private:
    static constexpr uint8_t EMPTY = 0;
    static constexpr uint8_t WHITE_PAWN = 1;
    static constexpr uint8_t WHITE_KNIGHT = 2;
    static constexpr uint8_t WHITE_BISHOP = 3;
    static constexpr uint8_t WHITE_ROOK = 4;
    static constexpr uint8_t WHITE_QUEEN = 5;
    static constexpr uint8_t WHITE_KING = 6;
    static constexpr uint8_t BLACK_PAWN = 7;
    static constexpr uint8_t BLACK_KNIGHT = 8;
    static constexpr uint8_t BLACK_BISHOP = 9;
    static constexpr uint8_t BLACK_ROOK = 10;
    static constexpr uint8_t BLACK_QUEEN = 11;
    static constexpr uint8_t BLACK_KING = 12;

    static constexpr uint8_t KINGSIDE = 13;
    static constexpr uint8_t QUEENSIDE = 14;
    static constexpr uint8_t WHITE = 15;
    static constexpr uint8_t BLACK = 16;

    uint8_t _board[8][8];

    static char _fenPiece(uint8_t piece);
    static std::string _asciiPiece(uint8_t piece);
    uint8_t getXY(uint8_t x, uint8_t y) const;
public:
    Game();
    void move(uint8_t fromx, uint8_t fromy, uint8_t tox, uint8_t toy);
    void moveLong(const char *s);
    void reset();
    void castling(uint8_t color, uint8_t side);
    void readForsythe(const std::string &s);
    std::string writeForsythe() const;
    std::string asciiBoardCM9K() const;
    std::string asciiBoard() const;
};

#endif

