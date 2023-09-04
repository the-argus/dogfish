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
        target.addIncludePath(std.Build.LazyPath{ .path = path });
    }
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
