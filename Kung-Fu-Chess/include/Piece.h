#pragma once

#include "Board.h"

class Piece {
public:
    virtual ~Piece() = default;

    virtual bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;
    virtual bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const = 0;
    virtual bool can_pass_through_units() const { return false; }

protected:
    bool legal_if_shape_matches(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const;
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
    bool can_pass_through_units() const override;
};

class Pawn : public Piece {
public:
    bool is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
    bool has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const override;
};

class PieceFactory {
public:
    static const Piece* get_piece(PieceType type);
};
