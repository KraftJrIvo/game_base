#include "game.h"
#include "game_cfg.h"
#include "raylib.h"

#include "../util/vec_ops.h"
#include "raymath.h"
#include <cmath>
#include <limits>
#include <map>

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
        return (pos.row >= 0 && pos.row < BOARD_HEIGHT && pos.col >= 0 && pos.col < ((pos.row % 2) ? (BOARD_WIDTH - 1) : (BOARD_WIDTH)));
    }

    Rectangle getBoardRect(const GameState& gs) {
        float bWidth = TILE_RADIUS * 2 * BOARD_WIDTH;
        float bHeight = ROW_HEIGHT * BOARD_HEIGHT;
        Vector2 bPos = {(GetScreenWidth() - bWidth) * 0.5f, (float)GetScreenHeight() - bHeight + gs.board.pos};
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

    int countBotEmpRows(const GameState& gs) {
        int n = 0;
        bool keep = true;
        for (int row = BOARD_HEIGHT - 1; row >= 0; --row) {
            for (int col = 0; col < BOARD_WIDTH - (row % 2); ++col) {
                if (gs.board.things[row][col].ref.exists) {
                    keep = false;
                    break;
                }
            }
            if (!keep) break;
            n++;
        }
        return n;
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
                nei.neighs[TOGOI[i]].exists = exists;
                if (exists) nei.neighs[TOGOI[i]].pos = pos;
                if (nei.ref.exists)
                    dis.neighs[i] = nei.ref;
                else
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
        if (i > 0 && pos.row - i <= gs.board.nFulRowsTop)
            gs.board.nFulRowsTop = pos.row + 1;
    }

    void addParticle(GameState& gs, const Thing& thing, Vector2 pos, Vector2 vel) {
        gs.particles.acquire(Particle{true, thing, pos, vel});
    }

    void removeThing(GameState& gs, const ThingPos& pos) {
        gs.board.things[pos.row][pos.col].ref.exists = false;
        updateNeighs(gs, pos, false);
        if (pos.row < gs.board.nFulRowsTop)
            gs.board.nFulRowsTop = pos.row + 1;
    }

    void rearm(GameState& gs) {
        gs.gun.armed.clr = (unsigned char)getRandVal(gs, 0, COLORS.size() - 1);
    }

    DLL_EXPORT void init(GameState& gs)
    {
        gs = GameState{0};
        
        gs.seed = rand() % std::numeric_limits<int>::max();

        for (int row = 0; row < BOARD_HEIGHT - BOARD_EMP_BOT_ROW_GAP; ++row)
            for (int col = 0; col < BOARD_WIDTH - (row % 2); ++col)
                 addTile(gs, {row, col}, Tile{{(unsigned char)getRandVal(gs, 0, COLORS.size() - 1)}, {true, {row, col}}, {0,0,0,0,0,0}});

        rearm(gs);

        float bHeight = ROW_HEIGHT * BOARD_HEIGHT;
    }

    void shootAndRearm(GameState& gs) {
        gs.bullet.exists = true;
        gs.bullet.rebouncing = false;
        float dir = gs.gun.dir + PI * 0.5f;
        gs.bullet.thing.clr = gs.gun.armed.clr;
        gs.bullet.vel = BULLET_SPEED * Vector2{cos(dir), -sin(dir)};
        gs.bullet.pos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};

        rearm(gs);
    }

    void checkDropRecur(GameState& gs, const ThingPos& pos, unsigned char clr, Arena<MAX_TODROP, ThingPos>& todrop, std::map<int, std::map<int, bool>>& visited, bool first = true) 
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col))
            return;
        visited[pos.row][pos.col] = true;
        if (checkBounds(gs, pos)) {
            const auto& tile = gs.board.things[pos.row][pos.col];
            if ((tile.ref.exists && tile.thing.clr == clr) || first) {
                if (tile.ref.exists && tile.thing.clr == clr) todrop.acquire(pos);
                for (auto& n : tile.neighs)
                    if (n.exists) checkDropRecur(gs, n.pos, clr, todrop, visited, false);
            }
        }
    }

    bool isConnectedToTop(const GameState& gs, const ThingPos& pos, std::map<int, std::map<int, bool>>& visited)
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col))
            return false;
        visited[pos.row][pos.col] = true;
        if (checkBounds(gs, pos)) {
            auto& tile = gs.board.things[pos.row][pos.col];
            if (tile.ref.exists) {
                bool connected = (pos.row == gs.board.nFulRowsTop - 1);
                for (auto& n : tile.neighs)
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
            auto& tile = gs.board.things[pos.row][pos.col];
            if (tile.ref.exists) {
                removeThing(gs, pos);
                addParticle(gs, gs.board.things[pos.row][pos.col].thing, getPixByPos(gs, pos), Vector2Zero());
                for (auto& n : tile.neighs)
                    if (n.exists) massRemoveUnconnected(gs, n.pos, visited, false);
            }
        }
    }

    void addShakeRecur(GameState& gs, const ThingPos& pos, std::map<int, std::map<int, bool>>& visited, unsigned char clr, float shake, int depth, int curdepth = 0, bool clrstreak = true)
    {
        if (visited.count(pos.row) && visited[pos.row].count(pos.col) || curdepth >= depth)
            return;
        visited[pos.row][pos.col] = true;
        auto& tile = gs.board.things[pos.row][pos.col];
        if (tile.ref.exists && curdepth == 0) tile.shake = std::max(tile.shake, shake / (curdepth + 1));
        if (tile.ref.exists || curdepth == 0) {
            for (int i = 0; i < 6; ++i) {
                if (checkBounds(gs, TOGO[i])) {
                    auto& n = gs.board.things[TOGO[i].row][TOGO[i].col];
                    bool samecolor = (clrstreak && n.thing.clr == clr);
                    if (n.ref.exists) n.shake = std::max(n.shake, samecolor ? shake : (shake / (curdepth + 2)));
                }
            }
            if (clrstreak) {
                for (int i = 0; i < 6; ++i) {
                    if (checkBounds(gs, TOGO[i])) {
                        auto& n = gs.board.things[TOGO[i].row][TOGO[i].col];
                        bool samecolor = (n.thing.clr == clr);
                        if (n.ref.exists && samecolor) addShakeRecur(gs, n.ref.pos, visited, clr, shake, depth, curdepth, true);
                    }
                }
            }
            if (!clrstreak || curdepth == 0) {
                for (int i = 0; i < 6; ++i) {
                    if (checkBounds(gs, TOGO[i])) {
                        auto& n = gs.board.things[TOGO[i].row][TOGO[i].col];
                        bool samecolor = (n.thing.clr == clr);
                        if (n.ref.exists && !samecolor) addShakeRecur(gs, n.ref.pos, visited, clr, shake, depth, curdepth + 1, false);
                    }
                }
            }
        }
    }

    void checkDrop(GameState& gs, const ThingPos& pos) {
        if (gs.bullet.todrop.count() >= N_TO_DROP) {
            for (int i = 0; i < gs.bullet.todrop.count(); ++i) {
                auto& td = gs.bullet.todrop.at(i);
                removeThing(gs, td);
                addParticle(gs, gs.board.things[td.row][td.col].thing, getPixByPos(gs, td), {gs.bullet.vel.x * 0.1f, -500.5f});
            }
            for (int i = 0; i < gs.bullet.todrop.count(); ++i) {
                auto& td = gs.bullet.todrop.at(i);
                auto& thing = gs.board.things[td.row][td.col];
                for (auto& n : thing.neighs) {
                    std::map<int, std::map<int, bool>> visited;
                    massRemoveUnconnected(gs, n.pos, visited);
                }
            }
        }
        gs.bullet.todrop.clear();
    }

    float easeOutBounce(float x) 
    {
        float n1 = 7.5625f;
        float d1 = 2.75f;

        if (x < 1 / d1) {
            return n1 * x * x;
        } else if (x < 2 / d1) {
            return n1 * (x - 1.5 / d1) * (x - 1.5 / d1) + 0.75;
        } else if (x < 2.5 / d1) {
            return n1 * (x - 2.25 / d1) * (x - 2.25 / d1) + 0.9375;
        } else {
            return n1 * (x - 2.625 / d1) * (x - 2.625 / d1) + 0.984375;
        }
    }

    void flyBullet(GameState& gs, float delta)
    {
        if (gs.bullet.exists)
            gs.bullet.pos += gs.bullet.vel * delta;
        if (gs.bullet.pos.y + TILE_RADIUS < 0)
            gs.bullet.exists = false;
        
        auto brect = getBoardRect(gs);
        auto bulpos = getPosByPix(gs, gs.bullet.pos);

        if (gs.bullet.rebouncing) {
            gs.bullet.pos = GetSplinePointBezierQuad(gs.bullet.pos, gs.bullet.rebCp, gs.bullet.rebEnd, gs.bullet.rebounce);
            float prog = (float)(GetTime() - gs.bullet.rebTime)/BULLET_REBOUNCE_TIME;
            if (prog > 1.0f) {
                gs.bullet.exists = false;
                addTile(gs, gs.bullet.lstEmp, Tile{{gs.bullet.thing.clr}});
                gs.bullet.todrop.acquire(gs.bullet.lstEmp);
                checkDrop(gs, gs.bullet.lstEmp);
                gs.bullet.rebouncing = false;
            } else {
                gs.bullet.rebounce = easeOutBounce(prog);
            }
        } else {
            if (gs.bullet.pos.x - BULLET_RADIUS_H < brect.x || gs.bullet.pos.x + BULLET_RADIUS_H > brect.x + brect.width)
                gs.bullet.vel.x *= -1.0f;

            if (bulpos.row >= 0 && bulpos.row < BOARD_HEIGHT && bulpos.col >= -1 && bulpos.col < BOARD_WIDTH && (bulpos.col == -1 || !gs.board.things[bulpos.row][bulpos.col].ref.exists)) {
                gs.bullet.lstEmp = {bulpos.row, bulpos.col};
                if (bulpos.row % 2) {
                    if (bulpos.col == BOARD_WIDTH - 1)
                        gs.bullet.lstEmp.col--;
                    else if (bulpos.col == -1)
                        gs.bullet.lstEmp.col++;
                }
            }
            
            for (int i = 0; i < BOARD_HEIGHT; ++i) {
                for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                    const auto& tile = gs.board.things[i][j];
                    if (tile.ref.exists) {
                        Vector2 tpos = getPixByPos(gs, {i, j});
                        if (Vector2DistanceSqr(tpos, gs.bullet.pos) < (TILE_RADIUS + BULLET_RADIUS_H) * (TILE_RADIUS + BULLET_RADIUS_H)) 
                        {
                            std::map<int, std::map<int, bool>> vis;
                            checkDropRecur(gs, gs.bullet.lstEmp, gs.bullet.thing.clr, gs.bullet.todrop, vis);
                            std::map<int, std::map<int, bool>> vis2;
                            addShakeRecur(gs, gs.bullet.lstEmp, vis2, gs.bullet.thing.clr, SHAKE_TIME, SHAKE_DEPTH);
                            gs.bullet.rebouncing = true;
                            gs.bullet.rebounce = 0.0f;
                            gs.bullet.rebCp = gs.bullet.pos - Vector2Normalize(gs.bullet.vel) * BULLET_REBOUNCE;
                            gs.bullet.rebEnd = getPixByPos(gs, gs.bullet.lstEmp);
                            gs.bullet.rebTime = GetTime();
                            break;
                        }
                    }
                }
                if (!gs.bullet.exists)
                    break;
            }
        }
    }

    void flyParticles(GameState& gs, float delta) {
        bool someInFrame = false;
        for (int i = 0; i < gs.particles.count(); ++i) {
            auto& prt = gs.particles.at(i);
            if (prt.exists) {
                prt.pos += prt.vel * delta;
                prt.vel += GRAVITY * Vector2{0.0f, 1.0f} * delta;
                if (prt.pos.y < GetScreenHeight())
                    someInFrame = true;
            }
        }
        if (!someInFrame)
            gs.particles.clear();
    }

    void update(GameState& gs) 
    {
        auto delta = GetFrameTime() / UPDATE_ITS;

        if (fabs(GetMouseDelta().x) > 0) {
            Vector2 gunPos = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() - TILE_RADIUS};
            gs.gun.dir = atan2(gunPos.y - GetMouseY(), GetMouseX() - gunPos.x) - PI * 0.5f;      
        } else if (IsKeyDown(KEY_LEFT)) {
            gs.gun.dir += gs.gun.speed * delta;
            gs.gun.speed += GUN_ACC * delta;
        } else if (IsKeyDown(KEY_RIGHT)) {
            gs.gun.dir -= gs.gun.speed * delta;
            gs.gun.speed += GUN_ACC * delta;
        } else {
            gs.gun.speed = GUN_START_SPEED;
        }
        gs.gun.speed = std::clamp(gs.gun.speed, GUN_START_SPEED, GUN_FULL_SPEED);
        gs.gun.dir = std::clamp(gs.gun.dir, -PI * 0.45f, PI * 0.45f);

        if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) && !gs.bullet.exists)
            shootAndRearm(gs);
        flyBullet(gs, delta);
    }

    void updateOnce(GameState& gs) 
    {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH - (i % 2); ++j) {
                Tile& tile = gs.board.things[i][j];
                if (tile.ref.exists) {
                    if (tile.shake < SHAKE_TIME || gs.bullet.todrop.count() < N_TO_DROP - 1)
                        tile.shake = std::max(tile.shake - GetFrameTime(), 0.0f);
                    else
                        tile.shake = std::min(tile.shake + GetFrameTime() * 2, MAX_SHAKE);
                }
            }
        }

        auto mpos = getPosByPix(gs, {(float)GetMouseX(), (float)GetMouseY()});
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && mpos.row >= 0 && mpos.row < BOARD_HEIGHT && mpos.col >= 0 && mpos.col < BOARD_WIDTH) {
            addTile(gs, mpos, Tile{});
            gs.board.things[mpos.row][mpos.col].thing.clr = GetRandomValue(0, COLORS.size() - 1);
        }

        flyParticles(gs, GetFrameTime());
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
                    Vector2 shake = (2.0f * Vector2{RAND_FLOAT - 0.5f, RAND_FLOAT - 0.5f}) * tile.shake * SHAKE_STR;
                    drawThing(gs, tpos + shake, tile.thing);
                }
            }
        }
        auto brect = getBoardRect(gs);
        DrawRectangleRec({brect.x - 3.0f, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);
        DrawRectangleRec({brect.x + brect.width, 0.0f, 3.0f, (float)GetScreenHeight()}, WHITE);

        //auto mpos = getPosByPix(gs, {(float)GetMouseX(), (float)GetMouseY()});
        //std::map<int, std::map<int, bool>> visited;
        //if (isConnectedToTop(gs, mpos, visited))
        //    DrawCircleV(getPixByPos(gs, mpos), 5, WHITE);
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
        const float TICKSTEP = TILE_RADIUS * 2;
        Vector2 pos = gunPos;
        float dir = gs.gun.dir + PI * 0.5f;
        for (int i = 0; i < NTICKS; ++i) {
            pos += TICKSTEP * Vector2{cos(dir), -sin(dir)};
            DrawCircleV(pos, 3, WHITE);
        }
    }    

    DLL_EXPORT void updateAndDraw(GameState& gs) 
    {
        for (int i = 0; i < UPDATE_ITS; ++i)
            update(gs);
        updateOnce(gs);

        BeginDrawing();
        ClearBackground(BLACK);
        drawBoard(gs);
        drawParticles(gs);
        drawGun(gs);
        drawBullet(gs);
        EndDrawing();
    }

} // extern "C"