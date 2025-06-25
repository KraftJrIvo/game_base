#include "game.h"

#include "../util/vec_ops.h"

#if defined(_WIN32) || defined(_WIN64)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C" {

    DLL_EXPORT void updateAndDraw(GameState& state) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawRectangleRec(state.rect, RED);
        EndDrawing();

        float velX = ((float)IsKeyDown(KEY_RIGHT)) * 1.0f - ((float)IsKeyDown(KEY_LEFT)) * 1.0f;
        float velY = ((float)IsKeyDown(KEY_DOWN)) * 1.0f - ((float)IsKeyDown(KEY_UP)) * 1.0f;
        Vector2 mov = state.cfg.SPEED * Vector2{velX, velY};
        state.vel = Vector2Lerp(state.vel, mov, state.cfg.ACC);
        state.rect.x += state.vel.x;
        state.rect.y += state.vel.y;
    }

} // extern "C"