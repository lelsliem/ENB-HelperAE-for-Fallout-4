-- include subprojects
includes("lib/commonlibf4")

-- set project constants
set_project("ENBHelperF4")
set_version("1.5.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- main shared target that produces ENBHelperF4.dll
target("ENBHelperF4")
    set_kind("shared")
    set_basename("ENBHelperF4")
    set_targetdir("build/windows/$(arch)/$(mode)")

    add_rules("commonlibf4.plugin", {
        name = "ENBHelperF4",
        author = "lelsliem",
        description = "ENBHelperF4 - ReShade/ENB bridge for Fallout 4 AE"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    add_defines("NOMINMAX", "_CRT_SECURE_NO_WARNINGS")

    -- On Windows with MSVC, pass the .def to the linker so exports match exactly
    if is_plat("windows") and is_subhost("msvc") then
        add_ldflags("-DEF:ENBHelperF4.def", {force = true})
    end

    if is_plat("windows") then
        add_syslinks("user32", "kernel32")
    end

-- convenience target for debug builds that produces the same basename
target("ENBHelperF4_debug")
    set_kind("shared")
    set_basename("ENBHelperF4")
    set_targetdir("build/windows/$(arch)/debug")

    add_rules("commonlibf4.plugin", {
        name = "ENBHelperF4",
        author = "lelsliem",
        description = "ENBHelperF4 (debug build)"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
    add_defines("NOMINMAX", "_CRT_SECURE_NO_WARNINGS")

    if is_plat("windows") and is_subhost("msvc") then
        add_ldflags("-DEF:ENBHelperF4.def", {force = true})
    end
