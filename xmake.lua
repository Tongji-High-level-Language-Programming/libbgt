add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_requires("libsdl3_ttf", {configs = {shared = false}, debug = is_mode("debug")})

set_encodings("source:utf-8")

option("use_ansi")
    set_default(false)
    set_showmenu(true)
    set_description("Use ANSI encoding instead of UTF-8")
    add_defines("USE_ANSI")

target("libbgt")
    set_languages("c++latest")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("include/", {public = true})
    add_headerfiles("include/*.h")
    add_headerfiles("include/internal/*.h", {install = false})
    add_packages("libsdl3_ttf")

    if is_plat("windows") then
        add_syslinks("vcruntime", "msvcrt")
    end

target("libbgt_vendored")
    set_default(false)
    set_kind("static")
    add_deps("libbgt")
    on_build(function (target)
        import("utils.archive.merge_staticlib")

        local staticlibs = {target:dep("libbgt"):targetfile()}

        for _, pkg in pairs(target:dep("libbgt"):pkgs()) do
            for _, libfile in pairs(pkg:libraryfiles()) do
                table.append(staticlibs, libfile)
            end
        end
        
        staticlibs = table.unique(staticlibs)

        print("merging static libraries: ", staticlibs)

        merge_staticlib(target, target:targetfile(), staticlibs)
    end)

target("demo")
    set_languages("c++latest")
    set_kind("binary")
    add_files("demo/**.cpp")
    add_deps("libbgt")

