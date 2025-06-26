#include <array>

#include "raylib.h"

#include "../util/arena.h"
#include "raymath.h"

#define BOARD_WIDTH  18
#define BOARD_HEIGHT 48
#define MAX_PARTICLES 1024
#define TILE_RADIUS 16
#define ROW_HEIGHT (TILE_RADIUS * sqrt(3))
#define BOARD_SPEED 0.01f
#define BULLET_SPEED 10.0f
#define BULLET_RADIUS_V TILE_RADIUS
#define BULLET_RADIUS_H TILE_RADIUS * 0.8f
#define GUN_SPEED 0.02f
#define BOARD_ACC 0.0f
#define GRAVITY 10.0f
#define COLORS std::array<Color, 6>{ RED, GREEN, BLUE, ORANGE, YELLOW, PINK }


struct Thing {
    unsigned char col = 0;
};

struct Board {
    float pos = 0;
    std::array<std::array<Thing, BOARD_WIDTH>, BOARD_HEIGHT> things;
};

struct Gun {
    float dir = 0;
    Thing armed;
};

struct Particle {
    Thing thing;
    Vector2 pos = Vector2Zero();
    Vector2 vel = Vector2Zero();
};

struct Bullet {
    bool exists = false;
    Thing thing;
    Vector2 pos = Vector2Zero();
    Vector2 vel = Vector2Zero();
    int lstEmpRow, lstEmpCol;
};

struct GameState {
    unsigned int seed;
    Board board;
    Gun gun;
    Arena<MAX_PARTICLES, Particle> particles;
    Bullet bullet;
};