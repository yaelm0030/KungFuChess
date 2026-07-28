#include "CommandEnvelope.h"

void to_json(nlohmann::json& json, const CommandEnvelope& envelope) {
    json = nlohmann::json{
        { "color", envelope.color },
        { "line", envelope.line },
    };
}

void from_json(const nlohmann::json& json, CommandEnvelope& envelope) {
    // NLOHMANN_JSON_SERIALIZE_ENUM's generated from_json does not throw on an
    // unmapped value - it silently falls back to the first enumerator (Color::w).
    // Validate the wire string explicitly instead of trusting that lenient decode.
    const auto color_string = json.at("color").get<std::string>();
    if (color_string == "w") {
        envelope.color = Color::w;
    } else if (color_string == "b") {
        envelope.color = Color::b;
    } else {
        throw nlohmann::json::other_error::create(501, "invalid color: " + color_string, nullptr);
    }
    envelope.line = json.at("line").get<std::string>();
}
