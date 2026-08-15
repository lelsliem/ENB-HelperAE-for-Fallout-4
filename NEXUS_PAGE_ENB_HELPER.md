# ENB Helper AE

## Short description (for the "Short description" field — must be ≤ 350 chars)

Tells ENB and ReShade what Fallout 4 is doing: time of day, weather and classification, location, interior/exterior, and camera transforms — read on demand, no config, no background threads. Requires F4SE and Address Library. Originally AI-written and pulled for bugs; this v1.5.1 rebuild is corrected, tested in-game, and fully documented.

---

## Full description

# ENB Helper AE

A small F4SE plugin that tells ENB and ReShade what the game is doing: what time it is, what the weather's up to, whether you're inside, and where the camera is pointing. Shaders call a handful of plain C functions to read current game state. No config files, no INI, no hotkeys — load the DLL and it works.

## The honest backstory (read this)

The first release of this mod (v1.5.0) was written by an AI in a single sitting. It loaded, it "worked", and it was wrong in ways that matter:

- A 60 Hz background thread read game objects off the game thread — a data race.
- Weather classification read the wrong byte of the weather data.
- Interior detection demanded a lighting template that plenty of interiors don't have.
- The README at the time boasted about "asynchronous thread execution". None of that was true.

That release was pulled. This is the corrected rebuild (v1.5.1): rewritten against how the game actually behaves, verified by building and testing in-game, with a crash-handling pass on top. Both drafts were AI-written; the difference is that this one was checked against the real runtime and the claims here match the code.

## What it exports

- `GetTime` — current game hour
- `GetWeatherTransition` — weather transition progress 0–1
- `GetCurrentWeather` / `GetOutgoingWeather` — weather FormIDs
- `GetCurrentWeatherClassification` / `GetOutgoingWeatherClassification` — 0 sunny / 1 cloudy / 2 rainy / 3 snow
- `GetCurrentLocationID` / `GetWorldSpaceID` — location and worldspace FormIDs
- `GetSkyMode` — sky mode enum
- `GetPlayerCameraTransformMatrices` — camera local / world / previous-world transforms
- `GetIsInterior` — inside an interior cell?
- `GetReShadeBridgePointer` — stable pointer to a small shared struct for ReShade shaders
- `GetHealthStatus` — all of the above in one struct, one call
- `GetPluginVersion` — 1.51

All getters sample on demand on the calling (render) thread — no background thread, no shared state racing.

## Fixes in v1.5.1

- **Removed the data-race worker thread** — getters sample on demand, which is where ENB/ReShade call from anyway.
- **Fixed weather classification** — reads the real `WeatherDataFlags` bits, not the wind-speed byte.
- **Fixed interior detection** — uses `IsInterior()`, no lighting-template requirement.
- **Fixed the startup CTD** — ENB's d3d11 proxy calls the camera getter with a mismatched prototype and a `-1` output pointer; every export write is now SEH-guarded and skips the write instead of killing the game. Verified: ENB polls the exports every frame with live weather/location values, no crashes.
- Idiomatic `F4SE_PLUGIN_LOAD` entry point, log rotation, dependencies vendored flat so the repo builds out of the box.

## Requirements

- Fallout 4 Next-Gen / AE (runtime 1.11.221)
- F4SE 0.7.8
- Address Library for F4SE Plugins (NG)

## Install

Extract `F4SE\Plugins\ENBHelperF4.dll` into your `Data\` folder, or install with your mod manager. On load it writes `My Games\Fallout4\F4SE\ENBHelperF4.log`.

## Credits and lineage

The export ABI this plugin implements is the standard ENB Helper API:

- **ENB Helper SE by aers** — https://www.nexusmods.com/skyrimspecialedition/mods/23174 — the original plugin ENB expects to find. The function names and signatures are the API contract: ENB and ReShade look them up by name, so any ENB-Helper-compatible plugin must export exactly these.
- **enb-helper-alt by doodlum** — https://github.com/doodlum/enb-helper-alt — the Fallout 4 / Skyrim port of that API this project started from. Its code even credits aers.

Two things worth being honest about:

- **No explicit license upstream.** doodlum's repo has no license file, and aers' original is closed-source on Nexus. The API and a couple of implementation patterns this project derives from it carry no explicit upstream permission. If either author objects, the code behind the ABI is a reimplementation and can be reworked — the function names can't change, but the code can.
- **This is a rewrite, not a copy.** On-demand sampling instead of a background thread, corrected classification and interior detection. It shares the public API and the behaviors that API dictates, but the implementation is its own.

## Changelog

**v1.5.1** — the corrected rebuild (the AI-written v1.5.0 was pulled): removed the data-race worker thread, fixed weather classification to read the real flags bits, fixed interior detection, SEH-guarded every export write against poisoned caller pointers (fixes the ENB startup CTD), idiomatic entry point, vendored dependencies, verified in-game.

## Author and AI disclosure

The rewrite, the fixes, and this documentation were done by a **Codebuff** coding agent at the request of the mod author, then reviewed and tested in-game by a human who knows the runtime. The first draft was also AI-written — and that's exactly why this page exists: the AI's version was pulled for real bugs, and this one was checked. If you're using AI to write game plugins, the cautionary tale still stands: the code will load, and it will still be wrong in the ways only verification catches.
