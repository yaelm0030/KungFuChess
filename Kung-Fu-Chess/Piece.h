#pragma once

#include "Board.h"

class Piece {
public:
    virtual ~Piece() = default;

    // Returns true if a piece of this type can move from (start_x, start_y)
    // to (dest_x, dest_y): the offset matches the piece's movement shape,
    // the path is not blocked, and the destination is not occupied by a
    // piece of the same color as the one at the start cell.
    virtual bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;

    // Returns true if another piece sits strictly between (start_x, start_y)
    // and (dest_x, dest_y), blocking the move. Sliding pieces (Rook, Bishop,
    // Queen) check every in-between cell; King and Knight can never be
    // blocked, since the king only moves one cell and the knight jumps.
    virtual bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;
};

class King : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Queen : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Rook : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Bishop : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Knight : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class Pawn : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class PieceFactory {
public:
    // Returns the shared instance (Flyweight) implementing the movement
    // rules for the given piece type, or nullptr if no rule is implemented.
    static const Piece* get_piece(PieceType type);
};
