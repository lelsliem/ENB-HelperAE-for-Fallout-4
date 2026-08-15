# ENBHelperF4

A small F4SE plugin that tells ENB and ReShade what the game is doing: what time it is,
what the weather's up to, whether you're inside, and where the camera is pointing.

No config files, no INI, no hotkeys. Load the DLL, and shaders call a handful of plain C
functions to read current game state.

Written by a **Codebuff** coding agent at the request of the repo owner. See
[History](#history) for how it got here and [Credits and lineage](#credits-and-lineage)
for where the API comes from.

## Why this exists

ENB and ReShade shaders keep wanting to react to the game — darken things at night, change
the look when it rains, behave differently indoors. The game doesn't expose that state in a
way shaders can read, so this plugin sits in between. It reads the game on their behalf and
hands the values over through `GetProcAddress`-able functions.

## History

This plugin's story is worth telling, because it explains why the code looks the way it
does.

**Aug 2026 — the AI draft.** The first version was written by an AI in a single sitting. It
loaded, it "worked", and it was wrong in ways that matter: a 60 Hz background thread read
game objects off the game thread (a data race waiting to happen), weather classification
read the wrong byte of the weather data, and interior detection demanded a lighting
template that plenty of interiors don't have. The README at the time boasted about
"asynchronous thread execution" eliminating stutters. None of that was true.

**Aug 2026 — the rewrite.** Same idea, rewritten by a Codebuff coding agent against how
the game actually behaves: on-demand sampling on the render thread, real WeatherDataFlags bits,
plain `IsInterior()`, an idiomatic `F4SE_PLUGIN_LOAD` entry point, and dependencies
vendored so the repo builds out of the box. Then it was tested in-game, and this README was
rewritten to say only what the code does.

**Aug 2026 — the crash nobody saw coming.** The rewrite loaded clean, but ENB itself
crashed it: ENB's d3d11 proxy calls `GetPlayerCameraTransformMatrices` with a
mismatched prototype and passes a `-1` output pointer, and the unguarded copy-out
killed the game at startup. Disassembling the faulting instruction showed the truth
(a write through a poisoned pointer passed in by the caller, not a bad read on our
side), and every output getter now writes through an SEH guard that skips the write
instead of dying. ENB's calls are still wrong-shaped, but they can no longer crash the
game.

**Now.** The plugin is in the state this README describes: honest, tested, with its
limitations written down. A heartbeat line in the log proves data actually flows
(ENB polls the getters every frame — tens of thousands of calls per session, live
weather/location values) and crashes are provably gone. The export ABI is the
standard ENB Helper one (see [Credits and lineage](#credits-and-lineage));
everything behind it is this project's own code. AI still helps out — it just
doesn't write the parts that have to be right.

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
| `GetPluginVersion()` | `1.51` (1.5.1 as a float) |
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
- `GetPluginVersion()` returns a float (1.51). Floats are a silly way to version things, but
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

### v1.5.1 — the one where I fixed the AI's bugs

The AI-written 1.5.0 release was pulled from Nexus; this is the corrected rebuild.

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
- Hardened every export against callers passing bad output pointers. ENB's
  d3d11 proxy calls the camera getter with a `-1` output pointer (mismatched
  prototype) and that crashed the game at startup; all output writes now run
  under SEH and skip the write instead of dying.
- Added a heartbeat log line so working is provable, not assumed: while a
  consumer calls the exports, the log shows live call counts and game state
  (time, weather, location) once per second. This is the verification build —
  the heartbeat can be stripped for a final release if desired.

## Credits and lineage

The export ABI this plugin implements comes from **ENB Helper SE** by **aers**
(Nexus 23174) — the plugin ENB itself expects to find. The function names and
signatures aren't optional: ENB and ReShade look them up by name, so any
ENB-Helper-compatible plugin has to export exactly these. The Fallout 4 port of
that API this project started from is **enb-helper-alt** by doodlum
(https://github.com/doodlum/enb-helper-alt), which covers both Skyrim and
Fallout 4.

Two things worth being honest about:

- doodlum's repo has no license file, and aers' original is closed-source on
  Nexus. So the API, and a couple of implementation patterns this project
  derives from it, carry no explicit upstream permission. If either author
  objects, the code behind the ABI is a reimplementation and can be reworked —
  the function names can't change, but the code can.
- This implementation is a rewrite: on-demand sampling instead of a background
  thread, corrected classification and interior detection. It shares the
  public API and the behaviors that API dictates, but it is not a copy of
  either upstream project.

## AI and this project

This project is openly AI-assisted — including this very rewrite. The first draft, the
bugs, and the overblown README were generated by an AI that didn't understand the runtime.
The rewrite, the fixes, and this README were also written by an AI — a Codebuff coding
agent — but this time against the actual game code, verified by building and testing
in-game, and reviewed by a human who knows the runtime. The claims here match the code,
which is the difference that matters. If you're using AI to write game plugins, the
cautionary tale still stands: the code will load, and it will still be wrong in the ways
only verification catches.

## Author

The rewrite — the corrected code, the vendored build, and this documentation — was written
by a **Codebuff** coding agent at the request of the repo owner. The full story is in
[History](#history). If aers or doodlum object to the lineage, [Credits and
lineage](#credits-and-lineage) explains what can and can't change.

## License

GPL-3.0 — see [LICENSE](LICENSE).
