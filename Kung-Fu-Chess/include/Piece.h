#pragma once

#include "Board.h"

// Strategy pattern: each piece type implements its own movement rule behind
// this common interface, so callers never branch on piece type.
class Piece {
public:
    virtual ~Piece() = default;

    // True if this piece can legally move from start to dest: shape, path,
    // and same-color capture are all satisfied.
    virtual bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;

    // True if another piece blocks the path between start and dest.
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

// Factory + Flyweight: hands out one shared instance per piece type.
class PieceFactory {
public:
    static const Piece* get_piece(PieceType type);
};
