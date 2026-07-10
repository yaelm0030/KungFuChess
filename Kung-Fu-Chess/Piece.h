#pragma once

#include "Board.h"

class Piece {
public:
    virtual ~Piece() = default;

    // Returns true if a piece of this type can move from (start_x, start_y)
    // to (dest_x, dest_y): the offset matches the piece's movement shape and,
    // for sliding pieces, every cell strictly between the two is empty. The
    // occupant of the destination cell (if any) is not considered here.
    virtual bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;
};

class King : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Queen : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Rook : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Bishop : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Knight : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class PieceFactory {
public:
    // Returns the shared instance (Flyweight) implementing the movement
    // rules for the given piece type, or nullptr if no rule is implemented.
    static const Piece* get_piece(PieceType type);
};
