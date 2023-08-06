const std = @import("std");

pub const gitDepDir = "./build/deps/";

pub fn link(
    targets: std.ArrayList(*std.Build.CompileStep),
    lib: []const u8,
) !void {
    for (targets.items) |target| {
        target.linkSystemLibrary(lib);
    }
}

pub fn include(
    targets: std.ArrayList(*std.Build.CompileStep),
    path: []const u8,
) !void {
    for (targets.items) |target| {
        target.addIncludePath(path);
    }
}

const BuildFunction = *const fn (b: *std.Build, target: std.zig.CrossTarget, mode: std.builtin.OptimizeMode) anyerror!*std.Build.CompileStep;

const LibraryInfo = struct {
    name: []const u8,
    buildFn: BuildFunction,
    fallbackIncludePath: []const u8,
};

const libraries = [_]LibraryInfo{
    LibraryInfo{
        .name = "raylib",
        .buildFn = @import("./deps/raylib.zig").addLib,
        .fallbackIncludePath = "src",
    },
    LibraryInfo{
        .name = "ode",
        .buildFn = @import("./deps/ode.zig").addLib,
        .fallbackIncludePath = "include",
    },
};

pub const Library = struct {
    link_flag: []const u8,
    linker_flag: []const u8,
    include_flag: []const u8,
    buildFn: ?BuildFunction,

    pub fn allFlags(self: @This()) [3][]const u8 {
        return .{ self.link_flag, self.linker_flag, self.include_flag };
    }
};

pub fn getLibraries(b: *std.Build) ![]const Library {
    var outputmem = try b.allocator.alloc(Library, libraries.len);
    for (libraries, 0..) |lib, index| {
        outputmem[index] = try getLibraryFromOption(b, lib);
    }
    return outputmem;
}

pub fn getLibraryFromOption(b: *std.Build, info: LibraryInfo) !Library {
    const prefix = b.option(
        []const u8,
        try std.fmt.allocPrint(b.allocator, "{s}-prefix", .{info.name}),
        try std.fmt.allocPrint(b.allocator, "Location where {s} include and lib directories can be found", .{info.name}),
    ) orelse null;

    const include_path = if (prefix == null) try std.fs.path.join(b.allocator, &[_][]const u8{
        try toString(std.fs.cwd(), b.allocator), "build", "deps", info.name, info.fallbackIncludePath,
    }) else null;

    return Library{
        .link_flag = if (prefix != null) try linkFlag(b.allocator, info.name) else "",
        .linker_flag = if (prefix != null) try linkPrefixFlag(b.allocator, prefix.?) else "",
        .include_flag = if (prefix != null) try includePrefixFlag(b.allocator, prefix.?) else try includeFlag(b.allocator, include_path.?),
        .buildFn = if (prefix != null) null else info.buildFn,
    };
}

pub fn includeFlag(ally: std.mem.Allocator, path: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(ally, "-I{s}", .{path});
}

pub fn linkFlag(ally: std.mem.Allocator, lib: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(ally, "-l{s}", .{lib});
}

pub fn includePrefixFlag(ally: std.mem.Allocator, path: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(ally, "-I{s}/include", .{path});
}

pub fn linkPrefixFlag(ally: std.mem.Allocator, lib: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(ally, "-L{s}/lib", .{lib});
}

pub fn toString(dir: std.fs.Dir, allocator: std.mem.Allocator) ![]const u8 {
    var real_dir = try dir.openDir(".", .{});
    defer real_dir.close();
    return std.fs.realpathAlloc(allocator, ".") catch |err| {
        std.debug.print("error encountered in converting directory to string.\n", .{});
        return err;
    };
}
