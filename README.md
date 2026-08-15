# ENBHelperF4

A native F4SE plugin for Fallout 4 (Next-Gen / 1.11.221) that exposes live game state —
time of day, weather, location, and camera transforms — to ENB and ReShade through a small,
stable set of plain C exports. It is built with **CommonLibF4** and resolves all game
addresses through the **Address Library**, so it is version-safe across game updates without
maintaining raw offsets.

This repository is a from-scratch rewrite of an AI-generated first draft. The first version
loaded and "worked" but contained real bugs (see [Changelog](#changelog)); this version is
the corrected, verified build.

## What it does

ENB and ReShade shaders frequently need to react to in-game conditions (night vs day,
interior vs exterior, rain vs clear, current worldspace). ENBHelperF4 gives them that data
as plain `GetProcAddress`-able C functions that return current values on demand.

All getters sample the game state **on the calling thread** and return immediately. ENB and
ReShade call them from the render (game) thread, so there is no cross-thread access to game
objects and no background thread — the data is always fresh and race-free.

## Exports

| Function | Returns |
|---|---|
| `GetTime(float&)` | Current game hour (0.0–24.0) |
| `GetWeatherTransition(float&)` | Weather transition progress 0–1 (interiors: 1.0) |
| `GetCurrentWeather(unsigned long&)` | Current weather FormID |
| `GetOutgoingWeather(unsigned long&)` | Weather FormID transitioning out |
| `GetCurrentWeatherClassification(int&)` | 0=pleasant/sunny, 1=cloudy, 2=rainy, 3=snow, -1=unknown |
| `GetOutgoingWeatherClassification(int&)` | Same codes, for the outgoing weather |
| `GetCurrentLocationID(unsigned long&)` | Current location FormID |
| `GetWorldSpaceID(unsigned long&)` | Current worldspace FormID |
| `GetSkyMode(unsigned long&)` | Sky mode enum value (`RE::Sky::Mode`) |
| `GetPlayerCameraTransformMatrices(NiTransform&, NiTransform&, NiTransform&)` | Camera local / world / previous-world transforms |
| `GetIsInterior(bool&)` | 1 if the player is inside an interior cell |
| `GetReShadeBridgePointer()` | Stable pointer to a small shared struct (time, interior flag, weather FormID) for ReShade shaders |
| `GetHealthStatus(ENBHelperHealth*)` | All of the above in one struct, one call |
| `GetPluginVersion()` | `1.5` |
| `IsLoaded()` | True once the plugin has loaded successfully |

The exports are unmangled via `src/ENBHelperF4.def` so ENB/ReShade can look them up by name.
The F4SE entry points come from the CommonLibF4 plugin rule and `F4SE_PLUGIN_LOAD`.

## Behavior notes

- **Weather classification** reads the real `WeatherDataFlags` bits (`kPleasant/kCloudy/kRainy/kSnow`)
  from the weather record's `weatherData` array. (The original draft dereferenced the whole
  array as a single int and read the wind-speed byte as a flag — wrong.)
- **Interior detection** uses the player's parent cell `IsInterior()`. Interiors report the
  lighting template's FormID as the current weather and a fixed transition of 1.0.
- **Weather transition**: the value of `Sky::currentWeatherPct` (0–1) while a weather change
  is in progress.
- **Camera transforms** are the local/world/previous-world transforms of the camera root node.

## Requirements

- Fallout 4 with **F4SE** (Next-Gen 1.11.221) and the **Address Library for F4SE Plugins**
- Visual Studio 2022 Build Tools (v143, C++23)
- [xmake](https://xmake.io/) 3.0.0+

## Building

```sh
xmake f -m releasedbg -y
xmake build
```

Output: `build/windows/x64/releasedbg/ENBHelperF4.dll`

The vendored CommonLibF4 and commonlib-shared are included in `lib/` (no submodules), so the
project builds out of the box. The static CRT (`/MT`) is used, matching CommonLibF4.

## Installing

Copy `ENBHelperF4.dll` to your `Data/F4SE/Plugins/` folder. On load, a log is written to
`My Games/Fallout4/F4SE/ENBHelperF4.log`.

## Changelog

### v1.5.0 — corrected rewrite
- **Removed the 60 Hz worker thread.** The original ran a detached thread that read
  `RE::Sky`, `PlayerCharacter`, and `PlayerCamera` off the game thread — a data race /
  use-after-free risk. All getters now sample on demand on the calling thread.
- **Fixed `CalculateClassification`** to read the actual `WeatherDataFlags` bits instead of
  treating the whole `weatherData` array as one int.
- **Fixed interior detection** to use `IsInterior()` instead of requiring a lighting template.
- **Idiomatic entry point**: `F4SE_PLUGIN_LOAD` + `F4SE::Init` with log rotation (replaces
  the hand-rolled `DllMain`/manual spdlog setup).
- **Fixed the vendored CommonLibF4 layout** (the nested submodule was empty and the include
  path wrong) and pinned the MSVC toolchain with `/MT`.
- Kept the `.def`-based unmangled exports for ENB/ReShade compatibility.

## License

GPL-3.0 — see [LICENSE](LICENSE). The original release builds based on the AI-generated
first draft are withdrawn; this repository is the authoritative source.
