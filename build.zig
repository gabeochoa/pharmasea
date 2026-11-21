const std = @import("std");

// Mirrors the makefile's compile/link; make is shell glue over this now
// (count, cppcheck, prof/leak/alloc stay there). See
// ~/p/port_to_zig_build.md for the measured tradeoffs.
//
// Sources are discovered by walking src/, so adding a .cpp never needs a
// build.zig edit.

const NOFLAGS = [_][]const u8{
    "-Wno-deprecated-volatile",
    "-Wno-missing-field-initializers",
    "-Wno-c99-extensions",
    "-Wno-unused-function",
    "-Wno-sign-conversion",
    "-Wno-implicit-int-float-conversion",
    "-Wno-deprecated-literal-operator",
    "-Wno-invalid-utf8",
    "-Wno-implicit-float-conversion",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    var flags: std.ArrayList([]const u8) = .empty;
    flags.appendSlice(b.allocator, &.{
        "-std=c++2a",
        "-Wall",
        "-Wextra",
        "-Wuninitialized",
        "-Wshadow",
        "-Wno-conversion",
        "-Wno-pedantic",
        "-g",
        // Same forced include + fmt defines the make PCH build used.
        "-include",
        "src/pch.hpp",
        "-DFMT_HEADER_ONLY",
        "-DFMT_USE_NONTYPE_TEMPLATE_PARAMETERS=0",
        "-DFMT_CONSTEVAL=",
        "-DTRACY_ENABLE",
        "-DAFTER_HOURS_ENABLE_MCP",
        "-DAFTERHOURS_SINGLE_RENDER_PASS",
        "-DENABLE_TESTS=1",
        // pkg-config found no libdw/libunwind on the dev machine; the
        // makefile computes these, keep them 0 until a consumer needs dw.
        "-DBACKWARD_HAS_DW=0",
        "-DBACKWARD_HAS_LIBUNWIND=0",
        "-Ivendor",
        // Project headers via -iquote so src/strings.h never shadows
        // libc's <strings.h> (same note as the makefile).
        "-iquote",
        "src",
    }) catch @panic("OOM");
    flags.appendSlice(b.allocator, &NOFLAGS) catch @panic("OOM");
    if (target.result.os.tag == .macos) {
        flags.appendSlice(b.allocator, &.{
            "-DAFTER_HOURS_ENABLE_MACOS_GESTURES",
            "-fblocks",
        }) catch @panic("OOM");
    }

    var sources: std.ArrayList([]const u8) = .empty;
    collectCpp(b, &sources, "src", .recursive);
    // The only vendor TU: upstream files plugin (TestRoots, save paths).
    sources.append(b.allocator, "vendor/afterhours/src/plugins/files.cpp") catch @panic("OOM");
    // Walk order is filesystem order; sort so the build cache key is stable.
    std.mem.sort([]const u8, sources.items, {}, lessThan);

    mod.addCSourceFiles(.{
        .files = sources.items,
        .flags = flags.items,
        .language = .cpp,
    });

    // System raylib via pkg-config (what the makefile links). The vendored
    // vendor/raylib headers are 4.6-dev and must not shadow the 5.5 ones.
    mod.linkSystemLibrary("raylib", .{ .use_pkg_config = .yes });
    mod.addLibraryPath(b.path("vendor"));
    mod.linkSystemLibrary("GameNetworkingSockets", .{});
    mod.addRPathSpecial("$ORIGIN/../../vendor");
    if (target.result.os.tag == .macos) {
        mod.linkFramework("OpenGL", .{});
        mod.linkFramework("AppKit", .{});
    }

    const exe = b.addExecutable(.{ .name = "pharmasea", .root_module = mod });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    // The game loads resources/ relative to cwd, and cwd for a RunArtifact is
    // the cache dir, not the repo.
    run_cmd.setCwd(b.path("."));
    if (b.args) |args| run_cmd.addArgs(args);
    b.step("run", "Run the game").dependOn(&run_cmd.step);
}

fn lessThan(_: void, a: []const u8, c: []const u8) bool {
    return std.mem.lessThan(u8, a, c);
}

fn collectCpp(
    b: *std.Build,
    list: *std.ArrayList([]const u8),
    sub_path: []const u8,
    depth: enum { flat, recursive },
) void {
    const io = b.graph.io;
    var dir = b.build_root.handle.openDir(io, sub_path, .{ .iterate = true }) catch |err|
        std.debug.panic("cannot open {s}: {t} (submodules checked out?)", .{ sub_path, err });
    defer dir.close(io);

    switch (depth) {
        .flat => {
            var it = dir.iterate();
            while (it.next(io) catch @panic("iterate failed")) |entry| {
                if (entry.kind != .file or !std.mem.endsWith(u8, entry.name, ".cpp")) continue;
                list.append(b.allocator, b.pathJoin(&.{ sub_path, entry.name })) catch @panic("OOM");
            }
        },
        .recursive => {
            var walker = dir.walk(b.allocator) catch @panic("OOM");
            defer walker.deinit();
            while (walker.next(io) catch @panic("walk failed")) |entry| {
                if (entry.kind != .file or !std.mem.endsWith(u8, entry.basename, ".cpp")) continue;
                // entry.path is only valid until the next next(); pathJoin dupes it.
                list.append(b.allocator, b.pathJoin(&.{ sub_path, entry.path })) catch @panic("OOM");
            }
        },
    }
}
