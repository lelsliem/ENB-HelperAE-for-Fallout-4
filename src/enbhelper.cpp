#include "PCH.h"
#include "enbhelper.h"
#include <shared_mutex>

extern std::shared_mutex stateMutex;
extern ThreadCachedData cachedData;
extern bool bLoaded;

extern "C" DLLEXPORT bool GetTime(float& time)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    time = cachedData.time;
    return true;
}

extern "C" DLLEXPORT bool GetWeatherTransition(float& t)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    t = cachedData.weatherTransition;
    return true;
}

extern "C" DLLEXPORT bool GetCurrentWeather(unsigned long& id)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    id = cachedData.currentWeatherID;
    return true;
}

extern "C" DLLEXPORT bool GetOutgoingWeather(unsigned long& id)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    id = cachedData.outgoingWeatherID;
    return true;
}

extern "C" DLLEXPORT bool GetCurrentWeatherClassification(int& c)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    c = cachedData.currentWeatherClass;
    return true;
}

extern "C" DLLEXPORT bool GetOutgoingWeatherClassification(int& c)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    c = cachedData.outgoingWeatherClass;
    return true;
}

extern "C" DLLEXPORT bool GetCurrentLocationID(unsigned long& id)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    id = cachedData.locationID;
    return true;
}

extern "C" DLLEXPORT bool GetWorldSpaceID(unsigned long& id)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    id = cachedData.worldSpaceID;
    return true;
}

extern "C" DLLEXPORT bool GetSkyMode(unsigned long& mode)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    mode = cachedData.skyMode;
    return true;
}

extern "C" DLLEXPORT bool GetPlayerCameraTransformMatrices(RE::NiTransform& m_local, RE::NiTransform& m_world, RE::NiTransform& m_oldworld)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    m_local = cachedData.cameraLocal;
    m_world = cachedData.cameraWorld;
    m_oldworld = cachedData.cameraOldWorld;
    return true;
}

extern "C" DLLEXPORT bool GetIsInterior(bool& isInterior)
{
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    isInterior = cachedData.isInterior;
    return true;
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
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    bridgeBuffer.in_game_time = cachedData.time;
    bridgeBuffer.is_interior_cell = cachedData.isInterior ? 1 : 0;
    bridgeBuffer.weather_form_id = cachedData.currentWeatherID;
    return &bridgeBuffer;
}

extern "C" DLLEXPORT bool GetHealthStatus(ENBHelperHealth* out)
{
    if (!out) return false;
    std::shared_lock<std::shared_mutex> lock(stateMutex);
    out->in_game_time = cachedData.time;
    out->current_weather_id = static_cast<uint32_t>(cachedData.currentWeatherID);
    out->outgoing_weather_id = static_cast<uint32_t>(cachedData.outgoingWeatherID);
    out->current_weather_class = static_cast<int32_t>(cachedData.currentWeatherClass);
    out->outgoing_weather_class = static_cast<int32_t>(cachedData.outgoingWeatherClass);
    out->worldspace_id = static_cast<uint32_t>(cachedData.worldSpaceID);
    out->location_id = static_cast<uint32_t>(cachedData.locationID);
    out->sky_mode = static_cast<uint32_t>(cachedData.skyMode);
    out->is_interior = cachedData.isInterior ? 1 : 0;
    return true;
}

extern "C" DLLEXPORT float GetPluginVersion()
{
    // Version 1.5
    return 1.5f;
}

extern "C" DLLEXPORT bool IsLoaded()
{
    return bLoaded;
}
