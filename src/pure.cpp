#include <cstddef>
#include <math.h>
#include <string>
#include <time.h>

#include "raylib.h"

#include "../game/src/game.h"

extern "C" const unsigned char res_icon_png[];
extern "C" const size_t        res_icon_png_len;

const std::string DLL_NAME = "GAME";
const std::string NEW_DLL_POSTFIX = "_NEW";
const int TARGET_FPS = 60;

struct BaseState {
    Vector2 winSz, baseWinSz;
};

void initWindow() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WIN_NOM);
    SetWindowIcon(LoadImageFromMemory(".png", res_icon_png, res_icon_png_len));
    SetTargetFPS(TARGET_FPS);    
    SetExitKey(KEY_F4);
}

void processInput(BaseState& bs, GameAssets& ga, GameState& gs) 
{
    PollInputEvents();

    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11) || (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))) {
        if (!IsWindowFullscreen()) {
            bs.baseWinSz = {(float)GetScreenWidth(), (float)GetScreenHeight()};
            auto mid = GetCurrentMonitor();
            auto newWinSz = Vector2{(float)GetMonitorWidth(mid), (float)GetMonitorHeight(mid)};
            float xCoeff = newWinSz.x / bs.winSz.x;       
            float yCoeff = newWinSz.y / bs.winSz.y;   
            bs.winSz = newWinSz;
            SetWindowSize(int(bs.winSz.x), int(bs.winSz.y));
        }
        ToggleFullscreen();
        if (!IsWindowFullscreen()) {
            EnableCursor();
            auto newWinSz = bs.baseWinSz;
            float xCoeff = newWinSz.x / bs.winSz.x;       
            float yCoeff = newWinSz.y / bs.winSz.y;       
            bs.winSz = newWinSz;
            SetWindowSize(int(bs.winSz.x), int(bs.winSz.y));
        }
    }
}

int main() 
{
    initWindow();

    BaseState bs;
    GameAssets ga;
    GameState gs;

    init(ga, gs);

    while (!WindowShouldClose()) {
        processInput(bs, ga, gs);
        updateAndDraw(gs);
    }

    CloseWindow();

    return 0;
}