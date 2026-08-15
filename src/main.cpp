// src/main.cpp
//
// ENBHelperF4 — F4SE entry point and game-state sampler.
//
// The original AI-generated version ran a detached 60 Hz worker thread that read
// RE::Sky / PlayerCharacter / PlayerCamera directly off the game thread. Those
// objects are owned and mutated by the game thread, so reading them from another
// thread is a use-after-free / data race. ENB and ReShade already call the exported
// getters from the render (game) thread, so the right design is to sample on demand
// inside the getters — no worker thread, no race.
//
#include "PCH.h"
#include "enbhelper.h"

#include <shared_mutex>

bool bLoaded = false;              // single definition for linker
std::shared_mutex stateMutex;      // guards cachedData between concurrent getters
ThreadCachedData cachedData{};

static bool validInterior(const RE::PlayerCharacter* a_player) noexcept
{
    return a_player && a_player->parentCell && a_player->parentCell->IsInterior();
}

// Weather classification bitmask lives at TESWeather::weatherData[kFlags]; each bit is a
// WeatherDataFlags value (Pleasant/Cloudy/Rainy/Snow). The original code dereferenced the
// whole 20-byte array as one int, so it read the wind-speed byte as a "flag" — wrong.
static std::int32_t CalculateClassification(const RE::TESWeather* a_weather) noexcept
{
    if (!a_weather) {
        return -1;
    }

    const auto flags = a_weather->weatherData[static_cast<std::size_t>(RE::TESWeather::WeatherData::kFlags)];
    if (flags & static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kPleasant)) {
        return 0;
    }
    if (flags & static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kCloudy)) {
        return 1;
    }
    if (flags & static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kRainy)) {
        return 2;
    }
    if (flags & static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kSnow)) {
        return 3;
    }
    return -1;
}

// Reads the current game state into `cachedData`. Must be called on the game/render
// thread — the exported getters all call this before returning, and ENB/ReShade invoke
// them from the render thread, so this is safe.
void RefreshCachedState() noexcept
{
    const auto* sky = RE::Sky::GetSingleton();
    const auto* player = RE::PlayerCharacter::GetSingleton();
    const auto* playerCamera = RE::PlayerCamera::GetSingleton();

    ThreadCachedData fresh{};

    if (!sky || !player) {
        return;
    }

    fresh.time = sky->currentGameHour;
    fresh.isInterior = validInterior(player);

    if (fresh.isInterior) {
        fresh.weatherTransition = sky->lightingTransition == 0.0f ? 1.0f : sky->lightingTransition;
        if (player->parentCell && player->parentCell->lightingTemplate) {
            fresh.currentWeatherID = player->parentCell->lightingTemplate->formID;
        }
    } else {
        fresh.weatherTransition = sky->currentWeatherPct;
        if (sky->currentWeather) {
            fresh.currentWeatherID = sky->currentWeather->formID;
            fresh.currentWeatherClass = CalculateClassification(sky->currentWeather);
        }
    }

    if (sky->lastWeather) {
        fresh.outgoingWeatherID = sky->lastWeather->formID;
        fresh.outgoingWeatherClass = CalculateClassification(sky->lastWeather);
    }

    if (player->currentLocation) {
        fresh.locationID = player->currentLocation->formID;
    }
    if (player->cachedWorldspace) {
        fresh.worldSpaceID = player->cachedWorldspace->formID;
    }

    fresh.skyMode = sky->mode.underlying();

    if (playerCamera && playerCamera->cameraRoot) {
        if (const auto cameraNode = playerCamera->cameraRoot.get()) {
            fresh.cameraLocal = cameraNode->local;
            fresh.cameraWorld = cameraNode->world;
            fresh.cameraOldWorld = cameraNode->previousWorld;
        }
    }

    std::unique_lock lock(stateMutex);
    cachedData = fresh;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
    F4SE::Init(a_f4se, {
        .logName    = "ENBHelperF4",  // CommonLibF4 appends ".log" itself
        .logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v",
        .logRotate  = 10,
    });

    bLoaded = true;
    REX::INFO("ENBHelperF4 loaded (v{})", GetPluginVersion());
    return true;
}
