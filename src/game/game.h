#include <array>

#include "raylib.h"

#include "../util/arena.h"
#include "raymath.h"

#define UPDATE_ITS  5
#define BOARD_WIDTH 12
#define BOARD_HEIGHT 36
#define MAX_PARTICLES 10240
#define TILE_RADIUS 24
#define ROW_HEIGHT (float)(TILE_RADIUS * sqrt(3))
#define BOARD_SPEED 0.01f
#define BULLET_SPEED 1000.0f
#define BULLET_RADIUS_V TILE_RADIUS
#define BULLET_RADIUS_H TILE_RADIUS * 0.8f
#define BULLET_REBOUNCE TILE_RADIUS
#define BULLET_REBOUNCE_TIME 0.25f
#define GUN_START_SPEED 0.5f
#define GUN_FULL_SPEED 2.0f
#define GUN_ACC 10.0f
#define BOARD_ACC 0.0f
#define GRAVITY 2500.0f
#define COLORS std::array<Color, 5>{ RED, GREEN, BLUE, ORANGE, PINK }
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
    float speed = GUN_START_SPEED;
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
    bool rebouncing = false;
    float rebounce;
    Vector2 rebCp, rebEnd;
    double rebTime;
};

struct GameState {
    unsigned int seed;
    Board board;
    Gun gun;
    Arena<MAX_PARTICLES, Particle> particles;
    Bullet bullet;
};