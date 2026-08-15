# ENBHelperF4

A small F4SE plugin that tells ENB and ReShade what the game is doing: what time it is,
what the weather's up to, whether you're inside, and where the camera is pointing.

No config files, no INI, no hotkeys. Load the DLL, and shaders call a handful of plain C
functions to read current game state.

## Why this exists

ENB and ReShade shaders keep wanting to react to the game — darken things at night, change
the look when it rains, behave differently indoors. The game doesn't expose that state in a
way shaders can read, so this plugin sits in between. It reads the game on their behalf and
hands the values over through `GetProcAddress`-able functions.

## The short, honest history

The first version of this plugin was written by an AI. It loaded, it "worked", and it was
wrong in a few important ways: it ran a background thread that read game objects off the
game thread (a data race waiting to happen), and its weather classification read the wrong
byte of the weather data. The README at the time made bold claims about "asynchronous thread
execution" eliminating stutters — none of which were true.

This version is the rewrite: same idea, but checked and corrected against how the game
actually works, then tested in-game. AI still helps out with this project — it just doesn't
write the parts that have to be right.

## What it exports

| Function | Returns |
|---|---|
| `GetTime(float&)` | Current game hour (0.0–24.0) |
| `GetWeatherTransition(float&)` | Weather transition progress 0–1 |
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

The exports keep their C names via `src/ENBHelperF4.def` so ENB/ReShade can find them by
name. The F4SE entry points come from the CommonLibF4 plugin rule and `F4SE_PLUGIN_LOAD` —
no hand-rolled `DllMain`.

## How it works

- Every getter re-reads the game state on the calling thread and returns. ENB and ReShade
  call these from the render (game) thread, so the values are current and nothing is shared
  across threads.
- Game addresses come from the **Address Library**, via CommonLibF4. No hardcoded offsets,
  so a game update doesn't silently break it.
- Weather classification reads the real `WeatherDataFlags` bits from the weather record.

## Things worth knowing

- Weather classification codes: 0 pleasant/sunny, 1 cloudy, 2 rainy, 3 snow, -1 unknown.
- Interiors: `GetIsInterior` is true, the "current weather" becomes the cell's lighting
  template FormID, and the transition reads as the game's lighting transition (or 1.0 if the
  game hasn't set one). That's deliberate, not a bug.
- Weather transition is `Sky::currentWeatherPct` while a change is in progress.
- Camera values are the local/world/previous-world transforms of the camera root node.

## Known limitations

- It only reads. There's no API for shaders to push values back into the game.
- The getters must be called from the game/render thread. That's where ENB and ReShade call
  them from; if something else calls them from a worker thread, you're on your own.
- `GetPluginVersion()` returns a float (1.5). Floats are a silly way to version things, but
  it's the ABI that's already out there.

## Building

Requirements: Fallout 4 with F4SE (Next-Gen 1.11.221) and the Address Library for F4SE
Plugins, Visual Studio 2022 Build Tools, and xmake 3.0.0+.

```sh
xmake f -m releasedbg -y
xmake build
```

Output: `build/windows/x64/releasedbg/ENBHelperF4.dll`

The vendored CommonLibF4 and commonlib-shared live in `lib/` (no submodules), so a fresh
clone just builds. The CRT is static (`/MT`), matching CommonLibF4.

One gotcha: if you build from Git Bash, xmake can decide the platform is "mingw" because
`MSYSTEM` is set, and then fail trying to build spdlog with a gcc that doesn't exist
(`cannot get program for cc`). The `xmake.lua` pins the platform to windows to prevent that.
If you ever hit it anyway, `xmake f -p windows -m releasedbg -y` sorts it out.

## Installing

Copy `ENBHelperF4.dll` to your `Data/F4SE/Plugins/` folder. On load it writes a log to
`My Games/Fallout4/F4SE/ENBHelperF4.log`.

## Changelog

### v1.5.0 — the one where I fixed the AI's bugs

- Removed the 60 Hz worker thread. The original ran a detached thread that poked `RE::Sky`,
  `PlayerCharacter`, and `PlayerCamera` from outside the game thread. The game owns those
  objects; reading them from another thread is a data race. Getters now sample on demand on
  the calling thread — which is where ENB and ReShade call from anyway.
- Fixed weather classification. The old code treated the whole 20-byte `weatherData` array
  as one int, so it read the wind-speed byte as a "flag". Now it reads the actual
  `kPleasant/kCloudy/kRainy/kSnow` bits.
- Fixed interior detection. It used to require a lighting template, so any interior without
  one was reported as exterior. Now it just asks the cell.
- Replaced the hand-rolled `DllMain` and manual spdlog setup with the standard
  `F4SE_PLUGIN_LOAD` entry point (log rotation included).
- Vendored CommonLibF4 flat. The nested submodule was empty and the include path was wrong,
  so the repo didn't build out of the box. It does now.
- Kept the `.def`-based unmangled exports; that part was fine.

## AI and this project

This project is openly AI-assisted. The first draft, the bugs, and the overblown README
were AI-generated. The rewrite, the fixes, and this README are human — and this time the
claims match the code. If you're using AI to write game plugins, take this as the cautionary
tale: the code will load, and it will still be wrong in the ways a human who knows the
runtime has to catch.

## License

GPL-3.0 — see [LICENSE](LICENSE).
