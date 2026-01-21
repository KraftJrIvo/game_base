#include <cstddef>
#include <filesystem>
#include <math.h>
#include <string>
#include <time.h>
#include <functional>
#include <vector>
#include <fstream>
#include <iostream>

#include "raylib.h"

#include "../game/src/game.h"
#include "util/zpp_bits.h"

#define NOGDI
#define NOUSER
#include <dylib.hpp>

#include "resources.h"

const std::string LIB_NAME = "GAME";
#if defined(_WIN32)
const std::string LIB_PATH = "..\\game\\build\\";
const std::string LIB_PREFIX = "";
const std::string LIB_EXT = ".dll";
#elif defined(__linux__)
const std::string LIB_PATH = "../game/build/";
const std::string LIB_PREFIX = "lib";
const std::string LIB_EXT = ".so";
#endif
const std::string NEW_LIB_POSTFIX = "_NEW";
const int TARGET_FPS = 60;

struct GameCase {
    GameState gs = GameState();
    std::vector<AutomationEvent> events;
};

struct GameCasesState {
    std::vector<GameCase> gameCases;
    bool recording = false;
    int replaying = false;
    int casen = -1;
    int frame = 0;
    int aelframe = 0;
};

struct BaseState {
    Vector2 winSz, baseWinSz;
    std::string libPath, libName;
    std::function<void(GameAssets&, GameState&)> gameInit;
    std::function<void(GameState&)> gameReset;
    std::function<void(GameState&, const GameState&)> gameSetState;
    std::function<void(GameState&)> gameUpdateAndDraw;
    std::filesystem::path gameLibPath, gameLibName, gameNewLibName, gameLibNameFull, gameNewLibNameFull;
    dylib lib;

    GameCasesState gcs;
    AutomationEventList ael;

    BaseState(const std::string& libPath, const std::string& libName) :
        libPath(libPath),
        libName(libName),
        gameLibPath(std::filesystem::current_path() / libPath),
        gameLibName(libName),
        gameLibNameFull(LIB_PREFIX + libName + LIB_EXT),
        gameNewLibName(libName + NEW_LIB_POSTFIX),
        gameNewLibNameFull(LIB_PREFIX + libName + NEW_LIB_POSTFIX + LIB_EXT),
        lib(gameLibPath, std::filesystem::exists(gameLibPath / gameLibNameFull) ? gameLibName : gameNewLibName)
    {
        setFunc();
    }

    void setFunc() {
        gameInit = lib.get_function<void(GameAssets&, GameState&)>("init");
        gameReset = lib.get_function<void(GameState&)>("reset");
        gameSetState = lib.get_function<void(GameState&, const GameState&)>("setState");
        gameUpdateAndDraw = lib.get_function<void(GameState&)>("updateAndDraw");
    }

    void reloadLib() {
        lib = dylib(libPath, libName);
        setFunc();
    }

    void checkLoadLib() {
        if (std::filesystem::exists(gameLibPath / gameNewLibNameFull)) {
            lib = dylib(gameLibPath / gameNewLibNameFull);
            std::filesystem::remove(gameLibPath / gameLibNameFull);
            std::filesystem::copy_file(gameLibPath / gameNewLibNameFull, gameLibPath / gameLibNameFull);
            reloadLib();
            std::filesystem::remove(gameLibPath / gameNewLibNameFull);
        }
    }

    void saveState(GameState& gs, const std::string& name = "") {
        auto [data, out] = zpp::bits::data_out();
        auto _ = out(gs, gcs);
        std::ofstream o(name.length() ? name : "state", std::ios::binary);
        o.write((const char*) data.data(), data.size());
        o.close();
    }

    void loadState(GameAssets& ga, GameState& gs, const std::string& name = "") {
        auto [data, in] = zpp::bits::data_in();
        auto filename = name.length() ? name : "state";
        uintmax_t file_size = std::filesystem::file_size(filename);
        std::ifstream i(filename, std::ios::binary);
        data.resize(file_size);
        i.seekg(0, std::ios::beg);
        i.read(reinterpret_cast<char*>(data.data()), data.size());
        GameState ngs;
        auto _ = in(ngs, gcs);
        gameSetState(gs, ngs);
    }
};

void initWindow() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1, 1, WIN_NOM);
    SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetWindowPosition(GetMonitorWidth(GetCurrentMonitor()) * 0.5f - WINDOW_WIDTH * 0.5f, GetMonitorHeight(GetCurrentMonitor()) * 0.5f - WINDOW_HEIGHT * 0.5f);
    SetWindowIcon(LoadImage("../game/res/icon.png"));
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

    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        if (bs.gcs.recording)
            StopAutomationEventRecording();
        bs.gcs.replaying = false;
        bs.gcs.recording = true;
        bs.gcs.casen = bs.gcs.gameCases.size();
        bs.gcs.gameCases.push_back(GameCase());
        bs.gcs.gameCases.back().gs = gs;
        bs.ael = LoadAutomationEventList(0);
        SetAutomationEventList(&bs.ael);
        SetAutomationEventBaseFrame(0);
        StartAutomationEventRecording();
    } else if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        if (bs.gcs.recording) {
            StopAutomationEventRecording();
            auto& events = bs.gcs.gameCases[bs.gcs.casen].events;
            events = std::vector<AutomationEvent>(bs.ael.count);
            memcpy(events.data(), bs.ael.events, sizeof(AutomationEvent) * bs.ael.count);
            bs.gcs.recording = false;        
        } else if (bs.gcs.casen < bs.gcs.gameCases.size()) {
            bs.gcs.replaying = true;
            bs.gameSetState(gs, bs.gcs.gameCases.at(bs.gcs.casen).gs);
            bs.gcs.frame = 0;
            bs.gcs.aelframe = 0;
            ResetInputState();
        }
    }

    int key = GetKeyPressed();
    bool digitPressed = key >= KEY_ZERO && key <= KEY_NINE;
    if (digitPressed) {
        int casen = std::stoi(std::string{(char)(key)}) - 1;
        if (casen < 0) casen = 9;
        if (casen < bs.gcs.gameCases.size()) {
            bs.gcs.replaying = true;
            bs.gcs.casen = casen;
            bs.gameSetState(gs, bs.gcs.gameCases.at(casen).gs);
            bs.gcs.frame = 0;
            bs.gcs.aelframe = 0;
            ResetInputState();
        }
    }

    if (bs.gcs.replaying) {
        auto& events = bs.gcs.gameCases.at(bs.gcs.casen).events;
        while (bs.gcs.frame == events[bs.gcs.aelframe].frame) {
            PlayAutomationEvent(events[bs.gcs.aelframe]);
            bs.gcs.aelframe++;
            if (bs.gcs.aelframe == events.size()) {
                bs.gcs.replaying = false;
                bs.gcs.frame = 0;
                bs.gcs.aelframe = 0;
            }
        }
        bs.gcs.frame++;
    } else {
        if (IsKeyPressed(KEY_S)/* || IsKeyPressed(KEY_SPACE)*/)
            bs.saveState(gs);
        if (IsKeyPressed(KEY_L))
            bs.loadState(ga, gs);
        if (IsKeyPressed(KEY_R))
            bs.gameInit(ga, gs);
    }

}

int main() 
{
    initWindow();

    BaseState bs(LIB_PATH, LIB_NAME);
    GameAssets ga;
    GameState gs;

    bs.checkLoadLib();
    bs.gameInit(ga, gs);

    while (!WindowShouldClose()) {
        bs.checkLoadLib();
        processInput(bs, ga, gs);

        bs.gameUpdateAndDraw(gs);
    }

    CloseWindow();

    return 0;
}