// ENBHelperF4 — the F4SE side: samples game state on demand and feeds the
// exported getters in enbhelper.cpp.
//
// The AI-generated first draft ran a detached 60 Hz thread that read
// RE::Sky / PlayerCharacter / PlayerCamera off the game thread. The game owns
// those objects; that was a data race. Everything here runs on the calling
// (render) thread now, which is where ENB and ReShade call from anyway.
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

// The classification bitmask lives at TESWeather::weatherData[kFlags] — one byte,
// one bit per WeatherDataFlags value. The original code read the whole 20-byte
// array as a single int, which grabbed the wind-speed byte instead.
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

// Pulls the current game state into `cachedData`. Only ever called from the
// exported getters, and only from the game/render thread — which is where
// ENB/ReShade call from.
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
