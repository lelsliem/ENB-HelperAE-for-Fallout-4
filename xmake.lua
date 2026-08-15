-- ENBHelperF4 — xmake build
--
-- Requirements:
--   xmake >= 3.0.0, Visual Studio 2022 (v143), Fallout 4 / F4SE runtime 1.11.221
--
-- Build:     xmake f -m releasedbg -y && xmake build

set_xmakever("3.0.0")

set_project("ENBHelperF4")

-- Platform pin: when xmake runs from Git Bash / MSYS2 (MSYSTEM=MINGW64 is set),
-- it can misdetect the platform as "mingw" and try to build the spdlog package
-- with a nonexistent gcc, failing with "cannot get program for cc". Force the
-- platform to windows on a Windows host so the project configures identically
-- from any shell. A command-line `xmake f -p <plat>` still overrides this.
if os.host() == "windows" then
    set_config("plat", "windows")
end

set_version("1.5.1")
set_license("GPL-3.0")
set_languages("c++23")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- Static CRT: /MT (release), /MTd (debug). Applied at project scope so the vendored
-- CommonLibF4 / commonlib-shared static libs are built with the same runtime.
if is_mode("release") then
    set_runtimes("MT")
else
    set_runtimes("MTd")
end

-- Vendored CommonLibF4 (its lib/commonlib-shared submodule is populated).
includes("lib/commonlibf4/commonlibf4-5ba1928a32c6ccd5690164b79066fc2f5dcb5c65")

target("ENBHelperF4", function()
    set_kind("shared")

    add_rules("commonlibf4.plugin", {
        name        = "ENBHelperF4",
        author      = "lelsliem",
        description = "ENBHelperF4 - ReShade/ENB bridge for Fallout 4 AE",
        version     = "1.5.1",
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    add_defines("NOMINMAX", "_CRT_SECURE_NO_WARNINGS")

    -- The ENB-facing getters are exported unmangled via the .def file so ENB/ReShade
    -- can GetProcAddress them by name. The F4SEPlugin_Query/Load entry points are NOT
    -- in the .def — the commonlibf4.plugin rule generates F4SEPlugin_Version, and
    -- F4SE_PLUGIN_LOAD (see main.cpp) supplies F4SEPlugin_Load.
    if is_plat("windows") and is_subhost("msvc") then
        add_ldflags("-DEF:" .. path.join(os.scriptdir(), "src", "ENBHelperF4.def"), { force = true })
    end
end)
