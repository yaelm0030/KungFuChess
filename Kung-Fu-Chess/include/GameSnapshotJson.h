#pragma once

// JSON serialization hooks for GameSnapshot and its nested types, for the future
// networking layer. Kept out of GameSnapshot.h/Position.h/Types.h so that
// <nlohmann/json.hpp> doesn't get pulled into every translation unit that includes
// those widely-shared headers. ADL only needs to_json declared in a header included
// before the call site, in the type's namespace (global here) - it doesn't need to
// live beside the struct definition. to_json only: no from_json for composite types.

#include <nlohmann/json.hpp>

#include "GameSnapshot.h"
#include "Position.h"
#include "Types.h"

NLOHMANN_JSON_SERIALIZE_ENUM(Color, {
    { Color::w, "w" },
    { Color::b, "b" },
})

NLOHMANN_JSON_SERIALIZE_ENUM(PieceType, {
    { PieceType::K, "K" },
    { PieceType::Q, "Q" },
    { PieceType::R, "R" },
    { PieceType::B, "B" },
    { PieceType::N, "N" },
    { PieceType::P, "P" },
})

NLOHMANN_JSON_SERIALIZE_ENUM(PieceState, {
    { PieceState::idle, "idle" },
    { PieceState::move, "move" },
    { PieceState::jump, "jump" },
    { PieceState::short_rest, "short_rest" },
    { PieceState::long_rest, "long_rest" },
})

void to_json(nlohmann::json& json, const Position& position);
void to_json(nlohmann::json& json, const PixelPosition& pixel_position);
void to_json(nlohmann::json& json, const PieceSnapshot& piece_snapshot);
void to_json(nlohmann::json& json, const MoveRecord& move_record);
void to_json(nlohmann::json& json, const GameSnapshot& snapshot);
