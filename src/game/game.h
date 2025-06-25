#include "raylib.h"
#include "raymath.h"

struct GameConfig {
    float SPEED = 5.0f;
    float ACC = 0.033f;
};

struct GameState {
    GameConfig cfg = GameConfig();
    Rectangle rect = {100, 100, 100, 50};
    Vector2 vel = Vector2Zero();
};