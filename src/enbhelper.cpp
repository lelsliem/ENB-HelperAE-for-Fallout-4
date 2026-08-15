#include "PCH.h"
#include "enbhelper.h"
#include <shared_mutex>

extern std::shared_mutex stateMutex;
extern ThreadCachedData cachedData;
extern bool bLoaded;

std::atomic<long> g_callCount{0};

// Every getter refreshes the state on the calling thread first, then reads its
// field. ENB/ReShade call these from the render thread, so nothing is shared
// across threads.

// Writes *out = value without letting a bad output pointer crash the game.
// This exists because a real caller — ENB's d3d11 proxy — called
// GetPlayerCameraTransformMatrices with rdx = -1 (a mismatched prototype), and
// the unguarded copy killed the game at startup. Under SEH a fault costs one
// skipped write instead of a CTD.
template <typename T>
static bool GuardedWrite(T& a_out, const T& a_value)
{
    __try {
        a_out = a_value;
        return true;
    } __except (1) {  // == EXCEPTION_EXECUTE_HANDLER; the macro doesn't resolve in this TU
        return false;
    }
}

extern "C" DLLEXPORT bool GetTime(float& time)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(time, cachedData.time);
}

extern "C" DLLEXPORT bool GetWeatherTransition(float& t)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(t, cachedData.weatherTransition);
}

extern "C" DLLEXPORT bool GetCurrentWeather(unsigned long& id)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(id, cachedData.currentWeatherID);
}

extern "C" DLLEXPORT bool GetOutgoingWeather(unsigned long& id)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(id, cachedData.outgoingWeatherID);
}

extern "C" DLLEXPORT bool GetCurrentWeatherClassification(int& c)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(c, cachedData.currentWeatherClass);
}

extern "C" DLLEXPORT bool GetOutgoingWeatherClassification(int& c)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(c, cachedData.outgoingWeatherClass);
}

extern "C" DLLEXPORT bool GetCurrentLocationID(unsigned long& id)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(id, cachedData.locationID);
}

extern "C" DLLEXPORT bool GetWorldSpaceID(unsigned long& id)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(id, cachedData.worldSpaceID);
}

extern "C" DLLEXPORT bool GetSkyMode(unsigned long& mode)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(mode, cachedData.skyMode);
}

extern "C" DLLEXPORT bool GetPlayerCameraTransformMatrices(RE::NiTransform& m_local, RE::NiTransform& m_world, RE::NiTransform& m_oldworld)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    GuardedWrite(m_local, cachedData.cameraLocal);
    GuardedWrite(m_world, cachedData.cameraWorld);
    GuardedWrite(m_oldworld, cachedData.cameraOldWorld);
    return true;
}

extern "C" DLLEXPORT bool GetIsInterior(bool& isInterior)
{
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    return GuardedWrite(isInterior, cachedData.isInterior);
}

// ReShade bridge struct (kept small and stable)
struct ReShadeSharedMemoryExchange {
    float in_game_time;
    int32_t is_interior_cell;
    uint32_t weather_form_id;
    float padding;
};

extern "C" DLLEXPORT void* GetReShadeBridgePointer()
{
    static ReShadeSharedMemoryExchange bridgeBuffer{};
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    bridgeBuffer.in_game_time = cachedData.time;
    bridgeBuffer.is_interior_cell = cachedData.isInterior ? 1 : 0;
    bridgeBuffer.weather_form_id = cachedData.currentWeatherID;
    return &bridgeBuffer;
}

// SEH can't live in a function that owns a mutex lock (C2712), so the guarded
// struct write is its own helper with no destructible locals.
static bool WriteHealthSnapshot(const ENBHelperHealth& a_src, ENBHelperHealth* a_out)
{
    __try {
        a_out->in_game_time = a_src.in_game_time;
        a_out->current_weather_id = a_src.current_weather_id;
        a_out->outgoing_weather_id = a_src.outgoing_weather_id;
        a_out->current_weather_class = a_src.current_weather_class;
        a_out->outgoing_weather_class = a_src.outgoing_weather_class;
        a_out->worldspace_id = a_src.worldspace_id;
        a_out->location_id = a_src.location_id;
        a_out->sky_mode = a_src.sky_mode;
        a_out->is_interior = a_src.is_interior;
        return true;
    } __except (1) {  // == EXCEPTION_EXECUTE_HANDLER; the macro doesn't resolve in this TU
        return false;
    }
}

extern "C" DLLEXPORT bool GetHealthStatus(ENBHelperHealth* out)
{
    if (!out) {
        return false;
    }
    RefreshCachedState();
    std::shared_lock<std::shared_mutex> lock(stateMutex);

    ENBHelperHealth snapshot{};
    snapshot.in_game_time = cachedData.time;
    snapshot.current_weather_id = static_cast<uint32_t>(cachedData.currentWeatherID);
    snapshot.outgoing_weather_id = static_cast<uint32_t>(cachedData.outgoingWeatherID);
    snapshot.current_weather_class = static_cast<int32_t>(cachedData.currentWeatherClass);
    snapshot.outgoing_weather_class = static_cast<int32_t>(cachedData.outgoingWeatherClass);
    snapshot.worldspace_id = static_cast<uint32_t>(cachedData.worldSpaceID);
    snapshot.location_id = static_cast<uint32_t>(cachedData.locationID);
    snapshot.sky_mode = static_cast<uint32_t>(cachedData.skyMode);
    snapshot.is_interior = cachedData.isInterior ? 1 : 0;
    return WriteHealthSnapshot(snapshot, out);
}

extern "C" DLLEXPORT float GetPluginVersion()
{
    // Version 1.5.1 — encoded as a float because that's the ABI that's out there
    return 1.51f;
}

extern "C" DLLEXPORT bool IsLoaded()
{
    return bLoaded;
}
