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

target("test-libbgt")
    set_languages("c++latest")
    set_kind("binary")
    add_files("tests/**.cpp")
    add_includedirs("include/")
    add_deps("libbgt")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

