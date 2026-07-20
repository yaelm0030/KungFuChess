#include "GameSnapshotJson.h"

void to_json(nlohmann::json& json, const Position& position) {
    json = nlohmann::json{ { "x", position.x }, { "y", position.y } };
}

void to_json(nlohmann::json& json, const PixelPosition& pixel_position) {
    json = nlohmann::json{ { "x", pixel_position.x }, { "y", pixel_position.y } };
}

void to_json(nlohmann::json& json, const PieceSnapshot& piece_snapshot) {
    json = nlohmann::json{
        { "type", piece_snapshot.type },
        { "color", piece_snapshot.color },
        { "pixels_location", piece_snapshot.pixels_location },
        { "target_pixels_location", piece_snapshot.target_pixels_location },
        { "progress", piece_snapshot.progress },
        { "state", piece_snapshot.state },
        { "cooldown_progress", piece_snapshot.cooldown_progress },
    };
}

void to_json(nlohmann::json& json, const MoveRecord& move_record) {
    json = nlohmann::json{
        { "type", move_record.type },
        { "color", move_record.color },
        { "source", move_record.source },
        { "destination", move_record.destination },
    };
}

void to_json(nlohmann::json& json, const GameSnapshot& snapshot) {
    json = nlohmann::json{
        { "board_width", snapshot.board_width },
        { "board_height", snapshot.board_height },
        { "pieces", snapshot.pieces },
        { "is_game_over", snapshot.is_game_over },
        { "score_w", snapshot.score_w },
        { "score_b", snapshot.score_b },
        { "move_history", snapshot.move_history },
        { "selected", snapshot.selected },
    };
}
