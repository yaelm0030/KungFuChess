#pragma once

// Wire format for a client command crossing the MessageBus: pairs the acting
// color with the raw command line, since the receiving side has no
// connection map of its own to derive the color from.

#include <string>

#include <nlohmann/json.hpp>

#include "GameSnapshotJson.h" // reuses the existing Color to_json/from_json
#include "Types.h"

struct CommandEnvelope {
    Color color;
    std::string line;

    bool operator==(const CommandEnvelope& other) const { return color == other.color && line == other.line; }
};

void to_json(nlohmann::json& json, const CommandEnvelope& envelope);
void from_json(const nlohmann::json& json, CommandEnvelope& envelope);
