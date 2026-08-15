#pragma once

#include <shared_mutex>
#include <atomic>
#include "RE/Fallout.h"

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

// True once F4SE_PLUGIN_LOAD has run
extern bool bLoaded;

// How many times a consumer has called one of the exported getters since load.
// The heartbeat in RefreshCachedState logs this so the log proves data flows.
extern std::atomic<long> g_callCount;

// Cached game state, guarded so concurrent getters don't tear reads
extern std::shared_mutex stateMutex;

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
};

extern ThreadCachedData cachedData;

// Re-samples game state on the calling thread. Implemented in main.cpp.
void RefreshCachedState() noexcept;

// Compact health struct returned by GetHealthStatus
struct ENBHelperHealth {
    float in_game_time;
    uint32_t current_weather_id;
    uint32_t outgoing_weather_id;
    int32_t current_weather_class;
    int32_t outgoing_weather_class;
    uint32_t worldspace_id;
    uint32_t location_id;
    uint32_t sky_mode;
    int32_t is_interior; // 0 or 1
};

// The exported names/signatures implement the ENB Helper SE ABI (aers, Nexus
// 23174; Fallout 4 port by doodlum, enb-helper-alt). ENB and ReShade look
// these up by name, so they must stay stable. Do not rename or reorder.
extern "C" {

// Basic getters
DLLEXPORT bool GetTime(float& time);
DLLEXPORT bool GetWeatherTransition(float& t);
DLLEXPORT bool GetCurrentWeather(unsigned long& id);
DLLEXPORT bool GetOutgoingWeather(unsigned long& id);
DLLEXPORT bool GetCurrentWeatherClassification(int& c);
DLLEXPORT bool GetOutgoingWeatherClassification(int& c);
DLLEXPORT bool GetCurrentLocationID(unsigned long& id);
DLLEXPORT bool GetWorldSpaceID(unsigned long& id);
DLLEXPORT bool GetSkyMode(unsigned long& mode);
DLLEXPORT bool GetPlayerCameraTransformMatrices(RE::NiTransform& m_local, RE::NiTransform& m_world, RE::NiTransform& m_oldworld);
DLLEXPORT bool GetIsInterior(bool& isInterior);

// ReShade bridge pointer (returns pointer to internal bridge struct)
DLLEXPORT void* GetReShadeBridgePointer();

// Health and version
DLLEXPORT bool GetHealthStatus(ENBHelperHealth* out);
DLLEXPORT float GetPluginVersion();

// Utility
DLLEXPORT bool IsLoaded();

}
