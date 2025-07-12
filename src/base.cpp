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

extern "C" const unsigned char res_icon[];
extern "C" const size_t        res_icon_len;

const std::string DLL_PATH = "..\\game\\build\\";
const std::string DLL_NAME = "GAME";
const std::string NEW_DLL_POSTFIX = "_NEW";
const std::string WIN_NOM = "A GAME";
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
    std::string dllName;
    std::function<void(GameAssets&, GameState&)> gameInit;
    std::function<void(const GameAssets&, GameState&)> gameUpdateAndDraw;
    std::filesystem::path gameDllPath;
    std::filesystem::path gameNewDllPath;
    dylib dll;

    GameCasesState gcs;
    AutomationEventList ael;

    BaseState(const std::string& dllName) :
        dllName(dllName),
        gameDllPath(std::filesystem::current_path() / (dllName + ".dll")),
        gameNewDllPath(std::filesystem::current_path() / (dllName + NEW_DLL_POSTFIX + ".dll")),
        dll(std::filesystem::exists(gameDllPath) ? gameDllPath : gameNewDllPath)
    {
        setFunc();
    }

    void setFunc() {
        gameInit = dll.get_function<void(GameAssets&, GameState&)>("init");
        gameUpdateAndDraw = dll.get_function<void(const GameAssets&, GameState&)>("updateAndDraw");
    }

    void reloadDll() {
        dll = dylib(dllName);
        setFunc();
    }

    void checkLoadDLL() {
        if (std::filesystem::exists(gameNewDllPath)) {
            dll = dylib(gameNewDllPath);
            std::filesystem::remove(gameDllPath);
            std::filesystem::copy_file(gameNewDllPath, gameDllPath);
            reloadDll();
            std::filesystem::remove(gameNewDllPath);
        }
    }

    void saveState(GameState& gs, const std::string& name = "") {
        auto [data, out] = zpp::bits::data_out();
        auto _ = out(gs, gcs);
        std::ofstream o(name.length() ? name : "state", std::ios::binary);
        o.write((const char*) data.data(), data.size());
        o.close();
    }

    void loadState(GameState& gs, const std::string& name = "") {
        auto [data, in] = zpp::bits::data_in();
        auto filename = name.length() ? name : "state";
        uintmax_t file_size = std::filesystem::file_size(filename);
        std::ifstream i(filename, std::ios::binary);
        data.resize(file_size);
        i.seekg(0, std::ios::beg);
        i.read(reinterpret_cast<char*>(data.data()), data.size());
        auto _ = in(gs, gcs);
    }
};

void initWindow() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WIN_NOM.c_str());
    SetWindowIcon(LoadImageFromMemory(".png", res_icon, res_icon_len));
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
            gs = bs.gcs.gameCases.at(bs.gcs.casen).gs;
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
            gs = bs.gcs.gameCases.at(casen).gs;
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
            bs.loadState(gs);
        if (IsKeyPressed(KEY_R))
            bs.gameInit(ga, gs);
    }

}

int main() 
{
    initWindow();

    BaseState bs(DLL_PATH + DLL_NAME);
    GameAssets ga;
    GameState gs;

    bs.checkLoadDLL();
    bs.gameInit(ga, gs);

    while (!WindowShouldClose()) {
        bs.checkLoadDLL();
        processInput(bs, ga, gs);

        bs.gameUpdateAndDraw(ga, gs);
    }

    CloseWindow();

    return 0;
}