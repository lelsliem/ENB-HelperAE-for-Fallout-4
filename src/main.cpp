#include "PCH.h"
#include <shlobj.h>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include "enbhelper.h"

// Enforces clean global tracking definitions for the linker table
bool bLoaded = false;
bool bRunning{ false };
std::thread workerThread;
std::shared_mutex stateMutex;

struct ThreadCachedData {
    float time = 0.0f;
    float weatherTransition = 0.0f;
    unsigned long currentWeatherID = 0;
    unsigned long outgoingWeatherID = 0;
    int currentWeatherClass = -1;
    int outgoingWeatherClass = -1;
    unsigned long locationID = 0;
    unsigned long worldSpaceID = 0;
    unsigned long skyMode = 0;
    bool isInterior = false;
    RE::NiTransform cameraLocal;
    RE::NiTransform cameraWorld;
    RE::NiTransform cameraOldWorld;
} cachedData;

bool validInterior(RE::PlayerCharacter* player)
{
    if (player && player->parentCell) {
        return player->parentCell->cellFlags.all(RE::TESObjectCELL::Flag::kInterior) && player->parentCell->lightingTemplate != nullptr;
    }
    return false;
}

int32_t CalculateClassification(RE::TESWeather* weather)
{
    if (!weather) return 0xFFFFFFFF;
    const auto flags = weather->weatherData;
    if (flags) {
        if ((*flags & 1) != 0)  return 0;
        if ((*flags & 2) != 0)  return 1;
        if ((*flags & 4) != 0)  return 2;
        if ((*flags & 8) != 0)  return 3;
    }
    return 0xFFFFFFFF;
}

void EnbHelperWorkerLoop()
{
    while (bRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        const auto sky = RE::Sky::GetSingleton();
        const auto player = RE::PlayerCharacter::GetSingleton();
        const auto playerCamera = RE::PlayerCamera::GetSingleton();

        if (!sky || !player) continue;

        ThreadCachedData freshTick;
        freshTick.time = sky->currentGameHour;
        freshTick.isInterior = validInterior(player);

        if (freshTick.isInterior) {
            freshTick.weatherTransition = sky->lightingTransition == 0.00f ? 1.00f : sky->lightingTransition;
            freshTick.currentWeatherID = player->parentCell->lightingTemplate->formID;
        } else {
            freshTick.weatherTransition = sky->currentWeatherPct;
            if (sky->currentWeather) {
                freshTick.currentWeatherID = sky->currentWeather->formID;
                freshTick.currentWeatherClass = CalculateClassification(sky->currentWeather);
            }
        }

        if (sky->lastWeather) {
            freshTick.outgoingWeatherID = sky->lastWeather->formID;
            freshTick.outgoingWeatherClass = CalculateClassification(sky->lastWeather);
        }

        if (player->currentLocation) freshTick.locationID = player->currentLocation->formID;
        if (player->cachedWorldspace) freshTick.worldSpaceID = player->cachedWorldspace->formID;
        freshTick.skyMode = sky->mode.underlying();

        if (playerCamera && playerCamera->cameraRoot) {
            const auto cameraNode = playerCamera->cameraRoot.get();
            if (cameraNode) {
                freshTick.cameraLocal = cameraNode->local;
                freshTick.cameraWorld = cameraNode->world;
                freshTick.cameraOldWorld = cameraNode->previousWorld;
            }
        }

        {
            std::unique_lock<std::shared_mutex> lock(stateMutex);
            cachedData = freshTick;
        }
    }
}

// ---- Feature: ReShade Hook Bridge API Interface ----
struct ReShadeSharedMemoryExchange {
    float in_game_time;
    int32_t is_interior_cell;
    uint32_t weather_form_id;
    float padding;
};

extern "C" __declspec(dllexport) void* GetReShadeBridgePointer()
{
    static ReShadeSharedMemoryExchange bridgeBuffer{};
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    bridgeBuffer.in_game_time = cachedData.time;
    bridgeBuffer.is_interior_cell = cachedData.isInterior ? 1 : 0;
    bridgeBuffer.weather_form_id = cachedData.currentWeatherID;
    return &bridgeBuffer;
}

void InitializeLog()
{
    wchar_t folderPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
        std::wstring path(folderPath);
        path += L"\\My Games\\Fallout4\\F4SE\\ENBHelperF4.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::string(path.begin(), path.end()), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%T] [%l] %v");
    }
}

extern "C" __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    InitializeLog();
    F4SE::Init(a_f4se);
    
    bRunning = true;
    workerThread = std::thread(EnbHelperWorkerLoop);
    workerThread.detach();

    bLoaded = true;
    return true;
}
