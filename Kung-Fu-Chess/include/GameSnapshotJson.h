#pragma once

// JSON hooks for GameSnapshot, kept separate so json.hpp isn't pulled into every shared header.

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

void from_json(const nlohmann::json& json, Position& position);
void from_json(const nlohmann::json& json, PixelPosition& pixel_position);
void from_json(const nlohmann::json& json, PieceSnapshot& piece_snapshot);
void from_json(const nlohmann::json& json, MoveRecord& move_record);
void from_json(const nlohmann::json& json, GameSnapshot& snapshot);
