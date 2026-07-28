#include "CommandEnvelope.h"

void to_json(nlohmann::json& json, const CommandEnvelope& envelope) {
    json = nlohmann::json{
        { "color", envelope.color },
        { "line", envelope.line },
    };
}

void from_json(const nlohmann::json& json, CommandEnvelope& envelope) {
    envelope.color = json.at("color").get<Color>();
    envelope.line = json.at("line").get<std::string>();
}
