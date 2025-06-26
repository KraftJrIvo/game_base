#include "game.h"
#include "raylib.h"

#include "../util/vec_ops.h"
#include "raymath.h"
#include <cmath>
#include <limits>

#if defined(_WIN32) || defined(_WIN64)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C" {

    int getRandVal(GameState& gs, int min, int max) {
        SetRandomSeed(gs.seed++);
        return GetRandomValue(min, max);
    }

    DLL_EXPORT void init(GameState& gs)
    {
        gs = GameState{0};
        
        gs.seed = GetRandomValue(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

        for (int i = 0; i < 3 * BOARD_HEIGHT / 4; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                gs.board.things[i][j] = Thing{(unsigned char)getRandVal(gs, 1, COLORS.size())};
            }
        }

        gs.gun.armed.col = (unsigned char)getRandVal(gs, 1, COLORS.size());

        float bHeight = ROW_HEIGHT * BOARD_HEIGHT;
        gs.board.pos = -bHeight * 0.5f;
    }

    void shootAndRearm(GameState& gs) {
        gs.bullet.exists = true;
        float dir = gs.gun.dir + PI * 0.5f;
        gs.bullet.thing = gs.gun.armed;
        gs.bullet.vel = BULLET_SPEED * Vector2{cos(dir), -sin(dir)};
        gs.bullet.pos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};

        gs.gun.armed.col = (unsigned char)getRandVal(gs, 1, COLORS.size());
    }

    void flyBullet(GameState& gs)
    {
        if (gs.bullet.exists)
            gs.bullet.pos += gs.bullet.vel;
        if (gs.bullet.pos.y + TILE_RADIUS < 0)
            gs.bullet.exists = false;
        float bWidth = TILE_RADIUS * 2 * BOARD_WIDTH;
        Vector2 bPos = {(GetScreenWidth() - bWidth) * 0.5f, gs.board.pos};
        if (gs.bullet.pos.x - BULLET_RADIUS_H < bPos.x || gs.bullet.pos.x + BULLET_RADIUS_H > bPos.x + bWidth)
            gs.bullet.vel.x *= -1.0f;

        int row = floor((gs.bullet.pos.y - bPos.y) / ROW_HEIGHT);
        int col = floor((gs.bullet.pos.x - bPos.x - float(row % 2) * TILE_RADIUS) / (TILE_RADIUS * 2));
        if (row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH && gs.board.things[row][col].col == 0 &&
            (col < BOARD_WIDTH - 1 || !(row % 2))) {
            gs.bullet.lstEmpRow = row;
            gs.bullet.lstEmpCol = col;
        }
        
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                const Thing& thing = gs.board.things[i][j];
                if (thing.col) {
                    float offset = float(i % 2) * TILE_RADIUS;
                    Vector2 tpos = {offset + bPos.x + TILE_RADIUS + j * TILE_RADIUS * 2, (float)(bPos.y + (i + 0.5f) * ROW_HEIGHT)};
                    if (Vector2DistanceSqr(tpos, gs.bullet.pos) < (TILE_RADIUS + BULLET_RADIUS_H) * (TILE_RADIUS + BULLET_RADIUS_H)) {
                        gs.bullet.exists = false;
                        gs.board.things[gs.bullet.lstEmpRow][gs.bullet.lstEmpCol] = gs.bullet.thing;
                        break;
                    }
                }
            }
            if (!gs.bullet.exists)
                break;
        }
    }

    void update(GameState& gs) 
    {
        if (fabs(GetMouseDelta().x) > 0) {
            Vector2 gunPos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};
            gs.gun.dir = atan2(gunPos.y - GetMouseY(), GetMouseX() - gunPos.x) - PI * 0.5f;      
        } else if (IsKeyDown(KEY_LEFT)) {
            gs.gun.dir += GUN_SPEED;
        } else if (IsKeyDown(KEY_RIGHT)) {
            gs.gun.dir -= GUN_SPEED;
        }
        gs.gun.dir = std::clamp(gs.gun.dir, -PI * 0.45f, PI * 0.45f);

        if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) && !gs.bullet.exists)
            shootAndRearm(gs);
        flyBullet(gs);
    }

    void drawThing(const GameState& gs, Vector2 pos, const Thing& thing) {
        DrawCircle(pos.x, pos.y, TILE_RADIUS, COLORS[thing.col - 1]);
    }

    void drawBoard(const GameState& gs) {
        float bWidth = TILE_RADIUS * 2 * BOARD_WIDTH;
        Vector2 bPos = {(GetScreenWidth() - bWidth) * 0.5f, gs.board.pos};
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                const Thing& thing = gs.board.things[i][j];
                if (thing.col) {
                    float offset = float(i % 2) * TILE_RADIUS;
                    Vector2 tpos = {offset + bPos.x + TILE_RADIUS + j * TILE_RADIUS * 2, (float)(bPos.y + (i + 0.5f) * ROW_HEIGHT)};
                    drawThing(gs, tpos, thing);
                }
            }
        }
        DrawRectangleRec({bPos.x - 3.0f, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);
        DrawRectangleRec({bPos.x + bWidth, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);

        //int row = floor((GetMouseY() - bPos.y) / ROW_HEIGHT);
        //int col = floor((GetMouseX() - bPos.x - float(row % 2) * TILE_RADIUS) / (TILE_RADIUS * 2));
        //if (row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH)
        //    drawThing(gs, {float(row % 2) * TILE_RADIUS + bPos.x + TILE_RADIUS + col * TILE_RADIUS * 2, (float)(bPos.y + (row + 0.5f) * ROW_HEIGHT)}, Thing{3});
    }

    void drawBullet(const GameState& gs) {
        if (gs.bullet.exists) {
            //float bWidth = TILE_RADIUS * 2 * BOARD_WIDTH;
            //Vector2 bPos = {(GetScreenWidth() - bWidth) * 0.5f, gs.board.pos};
            //float offset = float(gs.bullet.lstEmpRow % 2) * TILE_RADIUS;
            //Vector2 tpos = {offset + bPos.x + TILE_RADIUS + gs.bullet.lstEmpCol * TILE_RADIUS * 2, (float)(bPos.y + (gs.bullet.lstEmpRow + 0.5f) * ROW_HEIGHT)};
            //DrawCircleV(tpos, TILE_RADIUS, GRAY);
            DrawCircleV(gs.bullet.pos, BULLET_RADIUS_V, COLORS[gs.bullet.thing.col - 1]);
        }
    }

    void drawGun(const GameState& gs) {
        Vector2 gunPos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};
        DrawCircleV(gunPos, TILE_RADIUS + 3, WHITE);
        drawThing(gs, gunPos, gs.gun.armed);

        const int NTICKS = 50;
        const float TICKSTEP = 100;
        Vector2 pos = gunPos;
        float dir = gs.gun.dir + PI * 0.5f;
        for (int i = 0; i < NTICKS; ++i) {
            pos += TICKSTEP * Vector2{cos(dir), -sin(dir)};
            DrawCircleV(pos, 3, WHITE);
        }
    }    

    DLL_EXPORT void updateAndDraw(GameState& gs) 
    {
        update(gs);

        BeginDrawing();
        ClearBackground(BLACK);
        drawBoard(gs);
        drawGun(gs);
        drawBullet(gs);
        EndDrawing();
    }

} // extern "C"