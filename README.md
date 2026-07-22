# ENBHelperF4 - Next-Gen Asynchronous Performance Edition

A native F4SE script extender plugin built from scratch using the modern **CommonLibF4 Next-Gen Template** to provide an optimized data bridge between the Fallout 4 game engine and ENB graphics pipelines.

## 🚀 Key Modernizations & Optimizations
* **Asynchronous Thread Execution**: Completely decouples cell tracking and matrix math from the game's primary rendering loop. Calculations execute on a dedicated 60Hz background worker thread to completely eliminate micro-stutters and frame-time spikes in dense urban environments.
* **Un-Mangled Symbol Exports**: Explicitly links all core graphical hook functions via a module definition map file, preventing string-stripping during release configurations so the ENB binaries hook data instantly.
* **Natively Multi-Version Aware**: Fully compliant with the official script extender structure offsets for Fallout 4 Next-Gen Update builds (**v1.11.220, v1.11.221, and newer**).
* **ReShade Memory Bridge**: Includes a custom exported pointer entry (`GetReShadeBridgePointer`) allowing post-processing shaders to synchronize variables in real-time alongside ENB.

## 🛠️ Supported Graphics Pipeline Exports
* `GetTime` (In-Game Hour Tracking)
* `GetWeatherTransition` (Climate Transition Percentages)
* `GetCurrentWeather` / `GetOutgoingWeather` (Form ID Climate Tracking)
* Weather Classifications (Sunny, Cloudy, Rainy, Snowy Bitmasks)
* Location & Worldspace ID tracking (Supports DLC worldspaces natively)
* `GetPlayerCameraTransformMatrices` (3D coordinate extraction matrices)

## 🏗️ Compiling the Project
This project uses **xmake** for modern, standalone compilation management with zero manual dependency hunting.

1. Ensure **XMake (v3.0.9+)** and Visual Studio 2022 Build Tools are installed.
2. Clone the repository with its accompanying submodules:
   ```bash
   git clone --recursive https://github.com/lelsliem/ENB-HelperAE-for-Fallout-4
   ```
3. Initialize configurations and compile the release binary module:
   ```bash
   xmake config -p windows -a x64 -m release
   xmake
   ```
