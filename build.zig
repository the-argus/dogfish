const std = @import("std");
const builtin = @import("builtin");
const app_name = "dogfish";

const release_flags = [_][]const u8{ "-std=c11", "-DNDEBUG", "-DRELEASE" };
const debug_flags = [_][]const u8{
    "-std=c11",
};
var chosen_flags: ?[]const []const u8 = null;

const zcc = @import("compile_commands");

const c_sources = [_][]const u8{
    "src/airplane.c",
    "src/bullet.c",
    "src/camera.c",
    "src/debug.c",
    "src/gamestate.c",
    "src/fps_camera.c",
    "src/input.c",
    "src/physics.c",
    "src/main.c",
    "src/render_pipeline.c",
    "src/skybox.c",
    "src/threadutils.c",
    "src/quicksort.c",
    "src/terrain.c",
    "src/terrain_internal.c",
    "src/terrain_voxel_data.c",
    "src/terrain_render.c",
    "src/mesher.c",
};

const Library = struct {
    // name in build.zig
    remote_name: []const u8,
    // the name given to this library in its build.zig. usually in addStaticLibrary
    artifact_name: []const u8,
};

const libraries = [_]Library{
    .{ .remote_name = "raylib", .artifact_name = "raylib" },
    .{ .remote_name = "fast_noise", .artifact_name = "FastNoiseLite" },
};

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    // keep track of any targets we create
    var targets = std.ArrayList(*std.Build.CompileStep).init(b.allocator);

    const exe = b.addExecutable(.{
        .name = app_name,
        .optimize = mode,
        .target = target,
    });
    try targets.append(exe);

    chosen_flags = if (mode == .Debug) &debug_flags else &release_flags;

    var flags = std.ArrayList([]const u8).init(b.allocator);
    try flags.appendSlice(chosen_flags.?);

    // this adds intellisense for any headers which are not present in
    // the source of dependencies, but are built and installed
    try flags.append(try includePrefixFlag(b.allocator, b.install_prefix));

    exe.addCSourceFiles(&c_sources, try flags.toOwnedSlice());

    for (targets.items) |t| {
        // always link libc
        t.linkLibC();

        // link libraries from build.zig.zon
        for (libraries) |library| {
            const dep = b.dependency(library.remote_name, .{
                .target = target,
                .optimize = mode,
            });
            t.linkLibrary(dep.artifact(library.artifact_name));

            // special raylib case because we want to get some of its internal
            // headers
            if (std.mem.eql(u8, library.artifact_name, "raylib")) {
                const raylib_internal_includes = zcc.extractIncludeDirsFromCompileStep(b, dep.artifact(library.artifact_name));

                // HACK: we know src/external/glfw is included... so just recognize that and include "src" for us
                for (raylib_internal_includes) |internal_include| {
                    var include_iter = internal_include;

                    // traverse upwards until we find the raylib "src" dir
                    for (0..10) |_| {
                        include_iter = std.fs.path.dirname(include_iter) orelse break;
                        if (std.mem.eql(u8, std.fs.path.basename(include_iter), "src")) {
                            t.addIncludePath(.{ .path = include_iter });
                            break;
                        }
                    }
                }
            }
        }
    }

    // links and includes which are shared across platforms
    try include(targets, "src/");
    try include(targets, "src/include");

    // platform-specific additions
    switch (target.getOsTag()) {
        .windows => {},
        .macos => {},
        .linux => {
            try link(targets, "GL");
            try link(targets, "X11");
        },
        else => {},
    }

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    for (targets.items) |t| {
        b.installArtifact(t);
    }

    // compile commands step
    zcc.createStep(b, "cdb", try targets.toOwnedSlice());
}

fn link(
    targets: std.ArrayList(*std.Build.CompileStep),
    lib: []const u8,
) !void {
    for (targets.items) |target| {
        target.linkSystemLibrary(lib);
    }
}

fn include(
    targets: std.ArrayList(*std.Build.CompileStep),
    path: []const u8,
) !void {
    for (targets.items) |target| {
        target.addIncludePath(std.Build.LazyPath{ .path = path });
    }
}

fn includePrefixFlag(ally: std.mem.Allocator, path: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(ally, "-I{s}/include", .{path});
}
