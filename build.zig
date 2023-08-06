const std = @import("std");
const builtin = @import("builtin");
const app_name = "dogfish";

const release_flags = [_][]const u8{ "-std=c11", "-DNDEBUG" };
const debug_flags = [_][]const u8{"-std=c11"};
var chosen_flags: ?[]const []const u8 = null;

const common = @import("./build/common.zig");
const include = common.include;
const link = common.link;
const linkFlag = common.linkFlag;
const includeFlag = common.includeFlag;
const linkPrefixFlag = common.linkPrefixFlag;
const includePrefixFlag = common.includePrefixFlag;
const optionalPrefixToLibrary = common.optionalPrefixToLibrary;

const cdb = @import("./build/compile_commands.zig");
const makeCdb = cdb.makeCdb;

const c_sources = [_][]const u8{
    "src/airplane.c",
    "src/bullet.c",
    "src/camera_manager.c",
    "src/debug.c",
    "src/dynarray_implementation.c",
    "src/fps_camera.c",
    "src/gameobject.c",
    "src/input.c",
    "src/main.c",
    "src/object_structure.c",
    "src/physics.c",
    "src/render_pipeline.c",
    "src/skybox.c",
    "src/terrain.c",
};

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    // keep track of any targets we create
    var targets = std.ArrayList(*std.Build.CompileStep).init(b.allocator);

    // create executable
    var exe: ?*std.Build.CompileStep = null;

    const libraries = try common.getLibraries(b);
    var library_compile_steps = std.ArrayList(*std.Build.CompileStep).init(b.allocator);
    for (libraries) |library| {
        if (library.buildFn) |fun| {
            const compilestep = try fun(b, target, mode);
            try library_compile_steps.append(compilestep);
            // get compile_commands.json for the lib
            try targets.append(compilestep);
        }
    }

    exe = b.addExecutable(.{
        .name = app_name,
        .optimize = mode,
        .target = target,
    });
    try targets.append(exe.?);

    chosen_flags = if (mode == .Debug) &debug_flags else &release_flags;

    var flags = std.ArrayList([]const u8).init(b.allocator);
    try flags.appendSlice(chosen_flags.?);
    for (libraries) |library| {
        try flags.appendSlice(&library.allFlags());
    }

    // this adds intellisense for any headers which are not present in
    // the source of dependencies, but are built and installed
    try flags.append(try includePrefixFlag(b.allocator, b.install_prefix));

    exe.?.addCSourceFiles(&c_sources, try flags.toOwnedSlice());

    // always link libc
    for (targets.items) |t| {
        t.linkLibC();
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

    const run_cmd = b.addRunArtifact(exe.?);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // executable needs to depend on the libraries
    if (exe) |cstep| {
        for (library_compile_steps.items) |lib_cstep| {
            cstep.step.dependOn(&lib_cstep.step);
            cstep.linkLibrary(lib_cstep);
        }
    }

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
