# ENBHelperF4 v1.5.1

**The corrected rebuild.** The AI-written 1.5.0 release was pulled — it had a data-race worker thread, wrong weather-classification byte, broken interior detection, and an overblown README. This release is the rewrite, verified by building and testing in-game. The honest full story is in the README.

## What it does

A small F4SE plugin that tells ENB and ReShade what the game is doing: time of day, weather, location, interior/exterior, and camera transforms. Shaders read current values through a handful of plain C functions — no config, no INI, no hotkeys.

- `GetTime`, `GetWeatherTransition`, `GetCurrentWeather`/`GetOutgoingWeather` (FormIDs), weather classification (0 sunny / 1 cloudy / 2 rainy / 3 snow)
- `GetCurrentLocationID`, `GetWorldSpaceID`, `GetSkyMode`, `GetIsInterior`
- `GetPlayerCameraTransformMatrices` (local/world/previous-world)
- ReShade memory bridge + one-call `GetHealthStatus` snapshot
- All getters sample on demand on the calling (render) thread — race-free

## Fixes in this release

- **Removed the 60 Hz worker thread** — it read `RE::Sky`, `PlayerCharacter`, and `PlayerCamera` from outside the game thread (a data race). Getters now sample on demand.
- **Fixed weather classification** — the old code read the wind-speed byte as a "flag"; it now reads the real `kPleasant/kCloudy/kRainy/kSnow` bits.
- **Fixed interior detection** — no longer requires a lighting template (that misreported many interiors as exterior).
- **Fixed the startup CTD** — ENB's d3d11 proxy calls the camera getter with a mismatched prototype and a `-1` output pointer; every export write is now SEH-guarded and skips the write instead of killing the game. Verified: ENB polls the exports every frame, live weather/location values flow, no crash.
- Idiomatic `F4SE_PLUGIN_LOAD` entry point, log rotation, dependencies vendored flat so the repo builds out of the box.
- Heartbeat diagnostic (added to prove data flows, then stripped for this release build).

## Requirements

- Fallout 4 Next-Gen / AE (runtime **1.11.221**)
- F4SE **0.7.8**
- Address Library for F4SE Plugins (NG)

## Install

Extract `F4SE\Plugins\ENBHelperF4.dll` into your `Data\` folder (or install with your mod manager). On load it writes `My Games\Fallout4\F4SE\ENBHelperF4.log`.

## Checksums

- Zip: `1df6a610455e81322324ec9d79e526001f1a8430b1da2a2246c7bdf3a7b8fc60`
- DLL: `5d42a435c75eba365f866a5fe2c87440be0a25ae05c7ffc3beab4402ef4b18a0`

## Credits and lineage

The export ABI is the standard ENB Helper API — originally from **ENB Helper SE by aers** (Nexus 23174); the Fallout 4 port reference is **doodlum's enb-helper-alt**. Neither declares an explicit license, so this is an unofficial continuation with full credit to both; the code behind the ABI is this project's own reimplementation. The rewrite, fixes, and docs were done by a **Codebuff** coding agent at the request of the repo owner. License: GPL-3.0.
