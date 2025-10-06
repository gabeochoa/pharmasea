-- xmake.lua for pharmasea
set_project("pharmasea")
set_version("0.1.0")


-- toolchain
set_toolchains("clang")

-- build modes
add_rules("mode.debug", "mode.release")

-- pkg-config deps
add_requires("pkgconfig::raylib", {system = true, alias = "raylib"})
add_requires("pkgconfig::libdw", {system = true, optional = true, alias = "libdw"})
add_requires("pkgconfig::libunwind", {system = true, optional = true, alias = "libunwind"})

-- GameNetworkingSockets local paths
local GNS_INC = "/Users/gabe/p/GameNetworkingSockets/include"
local GNS_LIBDIR = "/Users/gabe/p/GameNetworkingSockets/build/bin"

-- output directory similar to makefile
set_objectdir("output")
set_targetdir(".")

-- precompiled header (disabled due to macOS C++ stdlib compatibility)
-- set_policy("build.pcheader", true)

-- build optimizations for incremental builds
set_policy("build.warning", true)

-- default flags
add_cxxflags("-std=c++2a")  -- match makefile exactly
add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Wuninitialized", "-Wshadow", "-Wconversion")
add_cxxflags("-DTRACY_ENABLE")


-- match makefile NOFLAGS (warnings off/on and -Werror)
add_cxxflags("-Wno-deprecated-volatile", "-Wno-missing-field-initializers", "-Wno-c99-extensions", "-Wno-unused-function", "-Wno-sign-conversion", "-Wno-implicit-int-float-conversion", "-Werror")

-- backward-cpp feature flags (xmake doesn't expand $(if ...)), gate via has_package
if has_package("libdw") then
    add_defines("BACKWARD_HAS_DW=1")
else
    add_defines("BACKWARD_HAS_DW=0")
end
if has_package("libunwind") then
    add_defines("BACKWARD_HAS_LIBUNWIND=1")
else
    add_defines("BACKWARD_HAS_LIBUNWIND=0")
end

-- include directories
add_includedirs(GNS_INC, "vendor")

-- improve dependency tracking
add_includedirs("src", "src/engine", "src/components", "src/dataclass", "src/layers", "src/system", "src/network")

-- optimize for incremental builds
add_cxxflags("-ftime-trace")  -- clang time trace for better dependency tracking

-- library search directories
add_linkdirs(GNS_LIBDIR, "vendor")

-- target
target("pharmasea")
	set_kind("binary")

	-- sources
	add_files("src/*.cpp", "src/**/*.cpp", "src/engine/**/*.cpp", "src/network/**/*.cpp")

	-- headers for dependency tracking (be more specific to avoid unnecessary rebuilds)
	add_headerfiles("src/**/*.h", "src/engine/**/*.h", "src/components/**/*.h", "src/dataclass/**/*.h", "src/layers/**/*.h", "src/system/**/*.h")

    -- precompiled header (disabled due to macOS C++ stdlib compatibility)
    -- set_pcxxheader("src/pch.hpp")
    
    -- manually include precompiled header like makefile does (disabled due to C++ stdlib issues)
    -- add_cxxflags("-include src/pch.hpp")
    -- clang invalid-utf8 warnings from vendored headers should not be errors
    add_cxxflags("-Wno-invalid-utf8", {force = true})

    -- use project logging/validation instead of vendor stubs
    add_defines("AFTER_HOURS_REPLACE_LOGGING", "AFTER_HOURS_REPLACE_VALIDATE")
	add_defines("FMT_HEADER_ONLY", "FMT_USE_NONTYPE_TEMPLATE_PARAMETERS=0", "FMT_CONSTEVAL=")

	-- macOS specifics
	if is_plat("macosx") then
		-- nothing extra yet
	end

	-- link raylib and GNS
	add_packages("raylib")

	add_links("GameNetworkingSockets")

	-- link backward platform libs (none on macos per makefile)
	if not is_plat("macosx") then
		add_links("dl")
		add_ldflags("-Wl,--export-dynamic")
	end

	-- post build actions similar to makefile
	after_build(function (target)
		-- install_name_tool rewrite like makefile
		if is_plat("macosx") then
			os.execv("install_name_tool", {"-change", "@rpath/libGameNetworkingSockets.dylib", path.join(GNS_LIBDIR, "libGameNetworkingSockets.dylib"), target:targetfile()})
		end
	end)

	-- run hook similar to post-build execution
	on_run(function (target)
		os.execv(target:targetfile(), {})
	end)

-- convenience phony targets
task("bring-gns")
	on_run(function()
		print("Ensure GameNetworkingSockets dylib is present in project root if needed")
	end)
