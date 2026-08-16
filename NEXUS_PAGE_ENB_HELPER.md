# ENB Helper AE

## Short description (for the "Short description" field — ≤ 350 chars)

Tells ENB and ReShade what Fallout 4 is doing: time of day, weather and classification, location, interior/exterior, and camera transforms — read on demand, no config, no background threads. Requires F4SE and Address Library. Originally AI-written and pulled for bugs; this v1.5.2 rebuild is corrected, tested in-game, and fully documented.

---

## Full description

# ENB Helper AE

A small F4SE plugin that tells ENB and ReShade what the game is doing: what time it is, what the weather's up to, whether you're inside, and where the camera is pointing. Shaders call a handful of plain C functions to read current game state. No config files, no INI, no hotkeys — load the DLL and it works.

## The honest backstory (please read this)

The first release of this mod (v1.5.0) was written by an AI in a single sitting. It loaded, it "worked", and it was wrong in ways that matter:

- **A 60 Hz background thread** read game objects off the game thread — a data race waiting to happen. The game owns those objects; reading them from another thread is undefined behavior.
- **Weather classification read the wrong byte** of the weather data — it treated the whole 20-byte array as one int, so it read the wind-speed byte as a "flag".
- **Interior detection was broken** — it demanded a lighting template that plenty of interiors don't have, so those interiors were reported as exterior.
- **The README lied** — it boasted about "asynchronous thread execution" eliminating stutters. None of that was true.

That release was pulled. This is the corrected rebuild.

## What this version fixes

- **Removed the data-race worker thread.** Getters now sample on demand on the calling thread — which is exactly where ENB and ReShade call from anyway. Nothing is shared across threads, so there is nothing to race.
- **Fixed weather classification.** It now reads the actual `kPleasant` / `kCloudy` / `kRainy` / `kSnow` bits from the weather record, not the wind-speed byte.
- **Fixed interior detection.** It now just asks the cell. No lighting-template requirement.
- **Fixed a startup CTD that ENB itself caused.** During in-game testing, ENB's d3d11 proxy called the camera export with a mismatched prototype and a `-1` output pointer — the unguarded copy-out killed the game at startup. Every export write is now SEH-guarded: a bad caller pointer skips the write instead of crashing the game. Disassembling the faulting instruction proved it was a write through a caller-supplied poisoned pointer, not a bug in our reads.
- **Verified, not assumed.** While testing, the plugin logged a heartbeat line (call counts and live game state once per second) proving data actually flows: ENB polls the getters every frame — tens of thousands of calls per session with live time, weather, and location values. That diagnostic logging was stripped for this release build; the exports are unchanged.

## What it exports

- `GetTime` — current game hour (0.0–24.0)
- `GetWeatherTransition` — weather transition progress 0–1
- `GetCurrentWeather` / `GetOutgoingWeather` — weather FormIDs
- `GetCurrentWeatherClassification` / `GetOutgoingWeatherClassification` — 0 sunny / 1 cloudy / 2 rainy / 3 snow, -1 unknown
- `GetCurrentLocationID` / `GetWorldSpaceID` — location and worldspace FormIDs
- `GetSkyMode` — sky mode enum value
- `GetPlayerCameraTransformMatrices` — camera local / world / previous-world transforms
- `GetIsInterior` — true if the player is inside an interior cell
- `GetReShadeBridgePointer` — a stable pointer to a small shared struct (time, interior flag, weather FormID) for ReShade shaders
- `GetHealthStatus` — all of the above in one struct, one call
- `GetPluginVersion` — 1.52

All getters sample on demand on the render thread. Game addresses come from the **Address Library** via CommonLibF4 — no hard-coded offsets, so a game update doesn't silently break it.

## Known limitations (honest ones)

- It only reads. There's no API for shaders to push values back into the game.
- The getters must be called from the game/render thread — that's where ENB and ReShade call them from. If something else calls them from a worker thread, you're on your own.
- ENB's proxy still calls the camera getter with a mismatched prototype, so the camera data may not reach ENB even though the game survives. The export is guarded and safe; making ENB's camera data actually land would require reverse-engineering ENB's expected signature. (This is the one known functional gap — everything else delivers real values, verified.)
- `GetPluginVersion()` returns a float (1.52). Floats are a silly way to version things, but it's the ABI that's already out there.

## Requirements

- Fallout 4 Next-Gen / AE — **1.11.137 through 1.11.221** (this DLL covers all five
  1.11.x runtimes via the Address Library). **1.10.980 / 1.10.984 need the separate
  pre-NG DLL** included in the same package — the Address Library uses two different ID
  schemes across the NG era, so no single binary can cover both families.
- F4SE matching your runtime
- Address Library for F4SE Plugins (NG)

## Install

Extract `F4SE\Plugins\ENBHelperF4.dll` into your `Data\` folder, or install with your mod manager. On load it writes `My Games\Fallout4\F4SE\ENBHelperF4.log`.

## Credits and lineage

The export ABI this plugin implements is the standard ENB Helper API. It is not optional or original to this mod — ENB and ReShade look the functions up by name, so any ENB-Helper-compatible plugin has to export exactly these:

- **ENB Helper SE by aers** — https://www.nexusmods.com/skyrimspecialedition/mods/23174 — the original plugin that defined this API. This is the plugin ENB itself expects to find.
- **enb-helper-alt by doodlum** — https://github.com/doodlum/enb-helper-alt — the Fallout 4 / Skyrim port of that API that this project started from. Its own code credits aers.

Two things worth being honest about:

- **Neither upstream has an explicit license.** aers' original is closed-source on Nexus; doodlum's repo has no license file (all rights reserved by default). The API — and a couple of implementation patterns this project derives from it — carry no explicit upstream permission. If either author objects, the code behind the ABI is a reimplementation and can be reworked: the function names can't change, but the code can.
- **This is a rewrite, not a copy.** On-demand sampling instead of a background thread, corrected classification and interior detection, guarded writes. It shares the public API and the behaviors that API dictates, but the implementation is its own. The lineage is credited here precisely because of that history.

## Changelog

**v1.5.2 — multiversion 1.11.x (corrected):**
- One DLL now covers every 1.11.x runtime (1.11.137 through 1.11.221) via the Address
  Library, instead of just 1.11.221.
- **1.10.980 / 1.10.984 are not covered by this DLL** — the Address Library uses a
  different ID scheme for those runtimes, proven by an in-game test on 1.10.984
  (`Invalid ID: 4798212`). The package ships a second pre-NG DLL for them.
- Pre-NG 1.10.163 is not covered either — different memory layout, needs its own build.

**v1.5.1 — the corrected rebuild** (the AI-written v1.5.0 was pulled):
- Removed the 60 Hz worker thread (data race) — getters sample on demand.
- Fixed weather classification to read the real `WeatherDataFlags` bits.
- Fixed interior detection to use `IsInterior()`.
- SEH-guarded every export write against poisoned caller pointers (fixes the ENB startup CTD).
- Idiomatic `F4SE_PLUGIN_LOAD` entry point with log rotation.
- Dependencies vendored flat — the repo builds out of the box, static CRT.
- Verified in-game with heartbeat diagnostics (then stripped for release).

## Author and AI disclosure

The rewrite, the fixes, and this documentation were done by a **Codebuff** coding agent at the request of the mod author, then reviewed and tested in-game by a human who knows the runtime. The first draft was also AI-written — and that is exactly why this page exists: the AI's version was pulled for real bugs, and this one was checked against how the game actually behaves. If you are using AI to write game plugins, the cautionary tale still stands: the code will load, and it will still be wrong in the ways only verification catches.

**License:** GPL-3.0. The full source is at https://github.com/lelsliem/ENB-HelperAE-for-Fallout-4 — every claim on this page is backed by the code and the commit history there.
