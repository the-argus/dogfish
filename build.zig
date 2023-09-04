const std = @import("std");
const builtin = @import("builtin");
const app_name = "dogfish";

const release_flags = [_][]const u8{ "-std=c11", "-DNDEBUG", "-DRELEASE" };
const debug_flags = [_][]const u8{
    // TODO: remove this, noise_3d function relies on UB
    "-fno-sanitize=undefined",
    "-std=c11",
};
var chosen_flags: ?[]const []const u8 = null;

const common = @import("./build/common.zig");
const include = common.include;
const link = common.link;
const includePrefixFlag = common.includePrefixFlag;

const cdb = @import("./build/compile_commands.zig");
const makeCdb = cdb.makeCdb;

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
};

const Library = struct {
    // name in build.zig
    remote_name: []const u8,
    // the name given to this library in its build.zig. usually in addStaticLibrary
    artifact_name: []const u8,
};

const libraries = [_]Library{
    .{ .remote_name = "raylib", .artifact_name = "raylib" },
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
    cdb.registerCompileSteps(targets);

    var step = try b.allocator.create(std.Build.Step);
    step.* = std.Build.Step.init(.{
        .id = .custom,
        .name = "cdb_file",
        .makeFn = makeCdb,
        .owner = b,
    });

    const cdb_step = b.step("cdb", "Create compile_commands.json");
    cdb_step.dependOn(step);
}
