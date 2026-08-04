// src/main.cpp
#include "PCH.h"
#include "enbhelper.h"

#include <shlobj.h>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <vector>
#include <windows.h>
#include <chrono>

bool bLoaded = false;                 // single definition for linker
std::atomic<bool> bRunning{ false };
std::thread workerThread;
std::shared_mutex stateMutex;
ThreadCachedData cachedData{};

static bool validInterior(RE::PlayerCharacter* player)
{
    if (player && player->parentCell) {
        return player->parentCell->cellFlags.all(RE::TESObjectCELL::Flag::kInterior) &&
               player->parentCell->lightingTemplate != nullptr;
    }
    return false;
}

static int32_t CalculateClassification(RE::TESWeather* weather)
{
    if (!weather) {
        return 0xFFFFFFFF;
    }

    const auto flags = weather->weatherData;
    if (flags) {
        if ((*flags & 1) != 0)  return 0;
        if ((*flags & 2) != 0)  return 1;
        if ((*flags & 4) != 0)  return 2;
        if ((*flags & 8) != 0)  return 3;
    }

    return 0xFFFFFFFF;
}

static void EnbHelperWorkerLoop()
{
    spdlog::info("ENBHelperF4: Worker loop started");
    while (bRunning.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        const auto sky = RE::Sky::GetSingleton();
        const auto player = RE::PlayerCharacter::GetSingleton();
        const auto playerCamera = RE::PlayerCamera::GetSingleton();

        if (!sky || !player) {
            continue;
        }

        ThreadCachedData freshTick{};
        freshTick.time = sky->currentGameHour;
        freshTick.isInterior = validInterior(player);

        if (freshTick.isInterior) {
            freshTick.weatherTransition = sky->lightingTransition == 0.0f ? 1.0f : sky->lightingTransition;
            if (player->parentCell && player->parentCell->lightingTemplate) {
                freshTick.currentWeatherID = player->parentCell->lightingTemplate->formID;
            }
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

        if (player->currentLocation) {
            freshTick.locationID = player->currentLocation->formID;
        }

        if (player->cachedWorldspace) {
            freshTick.worldSpaceID = player->cachedWorldspace->formID;
        }

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
    spdlog::info("ENBHelperF4: Worker loop exiting");
}

static void InitializeLog()
{
    wchar_t folderPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
        std::wstring wpath(folderPath);
        wpath += L"\\My Games\\Fallout4\\F4SE\\ENBHelperF4.log";

        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string narrowPath;
        if (sizeNeeded > 0) {
            std::vector<char> buffer(sizeNeeded);
            WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, buffer.data(), sizeNeeded, nullptr, nullptr);
            narrowPath.assign(buffer.data());
        } else {
            narrowPath = "ENBHelperF4.log";
        }

        try {
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(narrowPath, true);
            auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
            log->set_level(spdlog::level::info);
            log->flush_on(spdlog::level::info);
            spdlog::set_default_logger(std::move(log));
            spdlog::set_pattern("[%T] [%l] %v");
        } catch (...) {
            // If logging initialization fails, continue without crashing.
        }
    }
}

static void ShutdownPlugin()
{
    spdlog::info("ENBHelperF4: Shutdown requested");
    bRunning.store(false, std::memory_order_relaxed);
    if (workerThread.joinable()) {
        workerThread.join();
    }
    spdlog::info("ENBHelperF4: Shutdown complete");
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* /*a_f4se*/, F4SE::PluginInfo* a_info)
{
    if (!a_info) return false;

    // Some F4SE headers don't define kInfoVersion; use the numeric value expected instead.
    a_info->infoVersion = 1;        // safe default for most F4SE headers
    a_info->name = "ENBHelperF4";
    a_info->version = 150;          // integer version (1.5 -> 150)

    // Optional runtime check can be added here if your F4SE headers provide it.
    return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    (void)a_f4se; // parameter intentionally unused here
    InitializeLog();
    spdlog::info("ENBHelperF4 v{:.1f}", GetPluginVersion());

    F4SE::Init(a_f4se);

    bRunning.store(true, std::memory_order_relaxed);
    workerThread = std::thread(EnbHelperWorkerLoop);

    bLoaded = true;
    spdlog::info("ENBHelperF4: Worker thread started");
    return true;
}

// DllMain to ensure graceful shutdown on process detach
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_DETACH:
        // If lpReserved is NULL, the process is exiting normally; call shutdown
        if (!lpReserved) {
            ShutdownPlugin();
        }
        break;
    }
    return TRUE;
}
#endif
