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

void from_json(const nlohmann::json& json, Position& position) {
    position.x = json.at("x").get<int>();
    position.y = json.at("y").get<int>();
}

void from_json(const nlohmann::json& json, PixelPosition& pixel_position) {
    pixel_position.x = json.at("x").get<int>();
    pixel_position.y = json.at("y").get<int>();
}

void from_json(const nlohmann::json& json, PieceSnapshot& piece_snapshot) {
    piece_snapshot.type = json.at("type").get<PieceType>();
    piece_snapshot.color = json.at("color").get<Color>();
    piece_snapshot.pixels_location = json.at("pixels_location").get<PixelPosition>();
    piece_snapshot.target_pixels_location = json.at("target_pixels_location").get<PixelPosition>();
    piece_snapshot.progress = json.at("progress").get<double>();
    piece_snapshot.state = json.at("state").get<PieceState>();
    piece_snapshot.cooldown_progress = json.at("cooldown_progress").get<double>();
}

void from_json(const nlohmann::json& json, MoveRecord& move_record) {
    move_record.type = json.at("type").get<PieceType>();
    move_record.color = json.at("color").get<Color>();
    move_record.source = json.at("source").get<Position>();
    move_record.destination = json.at("destination").get<Position>();
}

void from_json(const nlohmann::json& json, GameSnapshot& snapshot) {
    snapshot.board_width = json.at("board_width").get<int>();
    snapshot.board_height = json.at("board_height").get<int>();
    snapshot.pieces = json.at("pieces").get<std::vector<PieceSnapshot>>();
    snapshot.is_game_over = json.at("is_game_over").get<bool>();
    snapshot.score_w = json.at("score_w").get<int>();
    snapshot.score_b = json.at("score_b").get<int>();
    snapshot.move_history = json.at("move_history").get<std::vector<MoveRecord>>();
    // selected is the one lenient field: absent key or explicit null both mean nullopt
    snapshot.selected = json.value("selected", std::optional<Position>{});
}
