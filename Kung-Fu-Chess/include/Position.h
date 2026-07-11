#pragma once

// A logical board coordinate. Pure data: no pixel/UI concerns, no chess
// rules. Shared by the Model, Rules/Arbiter, GameEngine, and Controller
// layers as the common unit of "where on the board."
struct Position {
    int x;
    int y;
};
