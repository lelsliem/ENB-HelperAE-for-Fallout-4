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
#include <utility>

bool bLoaded = false;              // single definition for linker
std::shared_mutex stateMutex;      // guards cachedData between concurrent getters
ThreadCachedData cachedData{};

// Indoors there is no sky weather, so ENB needs stable stand-ins: the cell's
// lighting template stands in for "current weather", and the interior lighting
// transition stands in for the weather transition. These are design choices
// for the plugin — ENB just requires a value indoors — kept here as named
// helpers so the intent survives.

// The cell the player is in, when it is an interior cell.
static const RE::TESObjectCELL* InteriorCell(const RE::PlayerCharacter* a_player) noexcept
{
    const auto* cell = a_player ? a_player->parentCell : nullptr;
    return cell && cell->IsInterior() ? cell : nullptr;
}

// The interior cell's lighting template, used as the indoor "weather" FormID.
static const RE::BGSLightingTemplate* InteriorLighting(const RE::PlayerCharacter* a_player) noexcept
{
    const auto* cell = InteriorCell(a_player);
    return cell ? cell->lightingTemplate : nullptr;
}

// Weather transition while indoors: the game animates interior lighting via
// Sky::lightingTransition (0-1). Mid-change we report it; idle (0) reports 1.0,
// which ENB reads as "no transition in progress".
static float InteriorWeatherTransition(const RE::Sky* a_sky) noexcept
{
    const auto t = a_sky->lightingTransition;
    return t > 0.0f ? t : 1.0f;
}

// Weather classification is part of the ENB Helper API contract: 0=sunny,
// 1=cloudy, 2=rainy, 3=snow, -1=unknown. TESWeather stores one flags byte
// (WeatherData::kFlags) whose bits are WeatherDataFlags; the table keeps the
// mapping explicit and adding a flag is a one-line change.
static constexpr std::pair<std::uint8_t, std::int32_t> kWeatherFlagToClass[] = {
    { static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kPleasant), 0 },
    { static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kCloudy),   1 },
    { static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kRainy),    2 },
    { static_cast<std::uint8_t>(RE::TESWeather::WeatherDataFlags::kSnow),     3 },
};

static std::int32_t CalculateClassification(const RE::TESWeather* a_weather) noexcept
{
    if (!a_weather) {
        return -1;
    }

    const auto flags = a_weather->weatherData[static_cast<std::size_t>(RE::TESWeather::WeatherData::kFlags)];
    for (const auto& [flag, classification] : kWeatherFlagToClass) {
        if ((flags & flag) != 0) {
            return classification;
        }
    }
    return -1;
}

// The camera node is owned by the game and can go stale — right after load,
// or whenever the camera is rebuilt. Dereferencing a stale node faults, which
// is exactly what the first in-game run did. The snapshot runs under SEH, so a
// fault leaves the transforms at their defaults instead of crashing the game
// (same pattern the SUP plugin uses for game-owned pointers).
static void SnapshotCamera(const RE::PlayerCamera* a_camera, ThreadCachedData& a_out)
{
    if (!a_camera || !a_camera->cameraRoot) {
        return;
    }

    __try {
        const auto cameraNode = a_camera->cameraRoot.get();
        if (cameraNode) {
            a_out.cameraLocal = cameraNode->local;
            a_out.cameraWorld = cameraNode->world;
            a_out.cameraOldWorld = cameraNode->previousWorld;
        }
    } __except (1) {  // == EXCEPTION_EXECUTE_HANDLER; the macro doesn't resolve in this TU
        // Camera pointer went stale mid-read; keep the defaults.
    }
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

    // Indoors: report the lighting template as the weather and the interior
    // lighting transition as the transition value. Outdoors: sky weather.
    if (const auto* light = InteriorLighting(player)) {
        fresh.isInterior = true;
        fresh.currentWeatherID = light->formID;
        fresh.weatherTransition = InteriorWeatherTransition(sky);
    } else {
        fresh.isInterior = false;
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

    SnapshotCamera(playerCamera, fresh);

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
