#include <array>

#include "raylib.h"

#include "../util/arena.h"
#include "raymath.h"

#define BOARD_WIDTH  12
#define BOARD_HEIGHT 36
#define MAX_PARTICLES 1024
#define TILE_RADIUS 24
#define ROW_HEIGHT (TILE_RADIUS * sqrt(3))
#define BOARD_SPEED 0.01f
#define BULLET_SPEED 1000.0f
#define BULLET_RADIUS_V TILE_RADIUS
#define BULLET_RADIUS_H TILE_RADIUS * 0.8f
#define GUN_SPEED 2.0f
#define BOARD_ACC 0.0f
#define GRAVITY 5000.0f
#define COLORS std::array<Color, 6>{ RED, GREEN, BLUE, ORANGE, YELLOW, PINK }
#define TOGO std::vector<ThingPos>{{pos.row, pos.col + 1}, {pos.row, pos.col - 1}, {pos.row - 1, pos.col}, {pos.row + 1, pos.col}, {pos.row - 1, (pos.row % 2) ? (pos.col + 1) : (pos.col - 1)}, {pos.row + 1, (pos.row % 2) ? (pos.col + 1) : (pos.col - 1)}}
#define TOGOI std::vector<int>{1, 0, 3, 2, 5, 4}
#define N_TO_DROP 4

struct ThingPos {
    int row, col;
};

struct ThingRef {
    bool exists;
    ThingPos pos;
};

struct Thing {
    unsigned char clr;
};

struct Tile {
    Thing thing;
    ThingRef ref;
    std::array<ThingRef, 6> neighs;
};

struct Board {
    float pos = 0;
    int nFullRows = 0;
    std::array<std::array<Tile, BOARD_WIDTH>, BOARD_HEIGHT> things;
};

struct Gun {
    float dir = 0;
    Thing armed;
};

struct Particle {
    bool exists = true;
    Thing thing;
    Vector2 pos = Vector2Zero();
    Vector2 vel = Vector2Zero();
    bool bounced = false;
};

struct Bullet {
    bool exists = false;
    Thing thing;
    Vector2 pos = Vector2Zero();
    Vector2 vel = Vector2Zero();
    ThingPos lstEmp;
};

struct GameState {
    unsigned int seed;
    Board board;
    Gun gun;
    Arena<MAX_PARTICLES, Particle> particles;
    Bullet bullet;
};