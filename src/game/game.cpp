#include "game.h"
#include "raylib.h"

#include "../util/vec_ops.h"
#include "raymath.h"
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <list>

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

    bool checkBounds(const GameState& gs, const ThingPos& pos) {
        return (pos.row >= 0 && pos.row < BOARD_HEIGHT && pos.col >= 0 && pos.col < BOARD_WIDTH);
    }

    Rectangle getBoardRect(const GameState& gs) {
        float bWidth = TILE_RADIUS * 2 * BOARD_WIDTH;
        float bHeight = ROW_HEIGHT * BOARD_HEIGHT;
        Vector2 bPos = {(GetScreenWidth() - bWidth) * 0.5f, gs.board.pos};
        return {bPos.x, bPos.y, bWidth, bHeight};
    }

    ThingPos getPosByPix(const GameState& gs, const Vector2& pix) {
        auto brec = getBoardRect(gs);
        int row = floor((pix.y - brec.y) / ROW_HEIGHT);
        int col = floor((pix.x - brec.x - float(row % 2) * TILE_RADIUS) / (TILE_RADIUS * 2));
        return {row, col};
    }

    Vector2 getPixByPos(const GameState& gs, const ThingPos& pos) {
        auto brec = getBoardRect(gs);
        float offset = float(pos.row % 2) * TILE_RADIUS;
        return {offset + brec.x + TILE_RADIUS + pos.col * TILE_RADIUS * 2, (float)(brec.y + (pos.row + 0.5f) * ROW_HEIGHT)};
    }

    bool checkFullRow(const GameState& gs, int row) {
        bool fullrow = true;
        for (int i = 0; i < BOARD_WIDTH - (row % 2); ++i) {
            if (!gs.board.things[row][i].ref.exists) {
                fullrow = false;
                break;
            }
        }
        return fullrow;
    }

    void updateNeighs(GameState& gs, const ThingPos& pos, bool exists) {
        for (int i = 0; i < 6; ++i) {
            auto& dis = gs.board.things[pos.row][pos.col];
            if (checkBounds(gs, TOGO[i])) {
                auto& nei = gs.board.things[TOGO[i].row][TOGO[i].col];
                if (nei.ref.exists) {
                    dis.neighs[i] = nei.ref;
                    nei.neighs[TOGOI[i]].exists = exists;
                    if (exists) nei.neighs[TOGOI[i]].pos = pos;
                } else
                    dis.neighs[i].exists = false;
            } else {
                dis.neighs[i].exists = false;
            }
        }
    }

    void addTile(GameState& gs, const ThingPos& pos, const Tile& tile) {
        auto& th = gs.board.things[pos.row][pos.col];
        th = tile;
        th.ref.exists = true;
        th.ref.pos = pos;
        updateNeighs(gs, pos, true);

        int i = 0;
        while ((pos.row - i > 0) && checkFullRow(gs, pos.row - i)) i++;
        if (i > 0 && pos.row - i <= gs.board.nFullRows)
            gs.board.nFullRows = pos.row + 1;
    }

    void addParticle(GameState& gs, const Thing& thing, Vector2 pos, Vector2 vel) {
        gs.particles.acquire(Particle{true, thing, pos, vel});
    }

    void flyParticles(GameState& gs) {
        for (int i = 0; i < gs.particles.count(); ++i) {
            auto& prt = gs.particles.at(i);
            if (prt.exists) {
                prt.pos += prt.vel * GetFrameTime();
                prt.vel += GRAVITY * Vector2{0.0f, 1.0f} * GetFrameTime();
                if (prt.pos.y > GetScreenHeight()) {
                    //if (!prt.bounced) {
                    //    prt.vel = {prt.vel.x, -0.8f * prt.vel.y};
                    //    prt.bounced = true;
                    //} else {
                        prt.exists = false;
                    //}
                }
            }
        }
    }

    void removeThing(GameState& gs, const ThingPos& pos) {
        gs.board.things[pos.row][pos.col].ref.exists = false;
        updateNeighs(gs, pos, false);
        if (pos.row < gs.board.nFullRows)
            gs.board.nFullRows = pos.row + 1;
    }

    void rearm(GameState& gs) {
        gs.gun.armed.clr = (unsigned char)getRandVal(gs, 0, COLORS.size() - 1);
    }

    DLL_EXPORT void init(GameState& gs)
    {
        gs = GameState{0};
        
        gs.seed = GetRandomValue(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

        for (int row = 0; row < 3 * BOARD_HEIGHT / 4; ++row)
            for (int col = 0; col < BOARD_WIDTH - (row % 2); ++col)
                 addTile(gs, {row, col}, Tile{{(unsigned char)getRandVal(gs, 0, COLORS.size() - 1)}, {true, {row, col}}, {0,0,0,0,0,0}});

        rearm(gs);

        float bHeight = ROW_HEIGHT * BOARD_HEIGHT;
        gs.board.pos = -bHeight * 0.5f;
    }

    void shootAndRearm(GameState& gs) {
        gs.bullet.exists = true;
        float dir = gs.gun.dir + PI * 0.5f;
        gs.bullet.thing.clr = gs.gun.armed.clr;
        gs.bullet.vel = BULLET_SPEED * Vector2{cos(dir), -sin(dir)};
        gs.bullet.pos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};

        rearm(gs);
    }

    void checkDropRecur(GameState& gs, const ThingPos& pos, unsigned char clr, std::vector<ThingPos>& todrop, std::map<int, std::map<int, bool>>& visited) 
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col))
            return;
        visited[pos.row][pos.col] = true;
        const auto& tile = gs.board.things[pos.row][pos.col];
        if (tile.thing.clr == clr) {
            todrop.push_back(pos);
            for (auto& n : tile.neighs)
                if (n.exists) checkDropRecur(gs, n.pos, clr, todrop, visited);
        }
    }

    bool isConnectedToTop(const GameState& gs, const ThingPos& pos, std::map<int, std::map<int, bool>>& visited)
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col))
            return false;
        visited[pos.row][pos.col] = true;
        if (checkBounds(gs, pos)) {
            auto& thing = gs.board.things[pos.row][pos.col];
            if (thing.ref.exists) {
                bool connected = (pos.row == gs.board.nFullRows - 1);
                for (auto& n : thing.neighs)
                    if (n.exists && !connected) connected |= isConnectedToTop(gs, n.pos, visited);
                visited[pos.row][pos.col] = connected;
                return connected;
            }
        }
        return false;
    }

    void massRemoveUnconnected(GameState& gs, const ThingPos& pos, std::map<int, std::map<int, bool>>& visited, bool check = true)
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col))
            return;
        visited[pos.row][pos.col] = true;
        std::map<int, std::map<int, bool>> visCon;
        if (checkBounds(gs, pos) && (!check || !isConnectedToTop(gs, pos, visCon))) {
            auto& thing = gs.board.things[pos.row][pos.col];
            if (thing.ref.exists) {
                removeThing(gs, pos);
                addParticle(gs, gs.board.things[pos.row][pos.col].thing, getPixByPos(gs, pos), Vector2Zero());
                for (auto& n : thing.neighs)
                    if (n.exists) massRemoveUnconnected(gs, n.pos, visited, false);
            }
        }
    }

    void checkDrop(GameState& gs, const ThingPos& pos) {
        std::vector<ThingPos> todrop;
        std::map<int, std::map<int, bool>> visited;
        checkDropRecur(gs, pos, gs.board.things[pos.row][pos.col].thing.clr, todrop, visited);
        if (todrop.size() >= N_TO_DROP) {
            for (auto& td : todrop) {
                removeThing(gs, td);
                addParticle(gs, gs.board.things[td.row][td.col].thing, getPixByPos(gs, td), gs.bullet.vel);
            }
            for (auto& td : todrop) {
                auto& thing = gs.board.things[td.row][td.col];
                for (auto& n : thing.neighs) {
                    std::map<int, std::map<int, bool>> visited;
                    massRemoveUnconnected(gs, n.pos, visited);
                }
            }
        }

    }

    void flyBullet(GameState& gs)
    {
        if (gs.bullet.exists)
            gs.bullet.pos += gs.bullet.vel * GetFrameTime();
        if (gs.bullet.pos.y + TILE_RADIUS < 0)
            gs.bullet.exists = false;
        
        auto brect = getBoardRect(gs);
        auto bulpos = getPosByPix(gs, gs.bullet.pos);

        if (gs.bullet.pos.x - BULLET_RADIUS_H < brect.x || gs.bullet.pos.x + BULLET_RADIUS_H > brect.x + brect.width)
            gs.bullet.vel.x *= -1.0f;

        if (bulpos.row >= 0 && bulpos.row < BOARD_HEIGHT && bulpos.col >= 0 && bulpos.col < BOARD_WIDTH && !gs.board.things[bulpos.row][bulpos.col].ref.exists && (bulpos.col < BOARD_WIDTH - 1 || !(bulpos.row % 2)))
            gs.bullet.lstEmp = {bulpos.row, bulpos.col};
        
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                const auto& tile = gs.board.things[i][j];
                if (tile.ref.exists) {
                    Vector2 tpos = getPixByPos(gs, {i, j});
                    if (Vector2DistanceSqr(tpos, gs.bullet.pos) < (TILE_RADIUS + BULLET_RADIUS_H) * (TILE_RADIUS + BULLET_RADIUS_H)) {
                        gs.bullet.exists = false;
                        addTile(gs, gs.bullet.lstEmp, Tile{{gs.bullet.thing.clr}});
                        checkDrop(gs, gs.bullet.lstEmp);
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
            gs.gun.dir += GUN_SPEED * GetFrameTime();
        } else if (IsKeyDown(KEY_RIGHT)) {
            gs.gun.dir -= GUN_SPEED * GetFrameTime();
        }
        gs.gun.dir = std::clamp(gs.gun.dir, -PI * 0.45f, PI * 0.45f);

        if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) && !gs.bullet.exists)
            shootAndRearm(gs);
        flyBullet(gs);
        flyParticles(gs);

        auto mpos = getPosByPix(gs, {(float)GetMouseX(), (float)GetMouseY()});
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && mpos.row >= 0 && mpos.row < BOARD_HEIGHT && mpos.col >= 0 && mpos.col < BOARD_WIDTH) {
            addTile(gs, mpos, Tile{});
            gs.board.things[mpos.row][mpos.col].thing.clr = GetRandomValue(0, 5);
        }
    }

    void drawThing(const GameState& gs, Vector2 pos, const Thing& thing) {
        DrawCircle(pos.x, pos.y, TILE_RADIUS, COLORS[thing.clr]);

        //for (auto& n : thing.neighs)
        //    if (n.exists) DrawLineV(pos, (getPixByPos(gs, n.pos) + pos) * 0.5, WHITE);
    }

    void drawParticles(GameState& gs) {
        for (int i = 0; i < gs.particles.count(); ++i) {
            auto& prt = gs.particles.at(i);
            if (prt.exists)
                drawThing(gs, prt.pos, prt.thing);
        }
    }

    void drawBoard(const GameState& gs) {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                const Tile& tile = gs.board.things[i][j];
                if (tile.ref.exists) {
                    Vector2 tpos = getPixByPos(gs, {i, j});
                    drawThing(gs, tpos, tile.thing);
                }
            }
        }
        auto brect = getBoardRect(gs);
        DrawRectangleRec({brect.x - 3.0f, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);
        DrawRectangleRec({brect.x + brect.width, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);

        auto mpos = getPosByPix(gs, {(float)GetMouseX(), (float)GetMouseY()});
        std::map<int, std::map<int, bool>> visited;
        if (isConnectedToTop(gs, mpos, visited))
            DrawCircleV(getPixByPos(gs, mpos), 5, WHITE);

    }

    void drawBullet(const GameState& gs) {
        if (gs.bullet.exists) {
            DrawCircleV(gs.bullet.pos, BULLET_RADIUS_V, COLORS[gs.bullet.thing.clr]);
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
        drawParticles(gs);
        drawGun(gs);
        drawBullet(gs);
        EndDrawing();
    }

} // extern "C"