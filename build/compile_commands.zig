const std = @import("std");
const common = @import("./common.zig");
const toString = common.toString;

var compile_steps: ?std.ArrayList(*std.Build.CompileStep) = null;

const CSourceFiles = std.Build.CompileStep.CSourceFiles;

const CompileCommandEntry = struct {
    arguments: []const []const u8,
    directory: []const u8,
    file: []const u8,
    output: []const u8,
};

pub fn registerCompileSteps(input_steps: std.ArrayList(*std.Build.CompileStep)) void {
    compile_steps = input_steps;
}

/// Find all the header installation directories for a step.
/// returns an owned arraylist
fn extractHeaderDirsFromStep(allocator: std.mem.Allocator, step: *std.Build.CompileStep) std.ArrayList([]const u8) {
    var dirs = std.ArrayList([]const u8).init(allocator);
    for (step.installed_headers.items) |header_step| {
        if (header_step.id == .install_file) {
            const install_file = header_step.cast(std.Build.InstallFileStep) orelse @panic("Programmer error generating compile_commands.json");
            // path to specific file being installed
            const file = install_file.dest_builder.getInstallPath(
                install_file.dir,
                install_file.dest_rel_path,
            );
            // just add the directory of that file. often creates duplicates
            dirs.append(if (std.fs.path.dirname(file)) |dirname| dirname else {
                std.log.warn("Unable to get directory name for installed header file {s} ", .{file});
                continue;
            }) catch @panic("OOM");
        }
    }
    return dirs;
}

// NOTE: some of the CSourceFiles pointed at by the elements of the returned
// array are allocated with the allocator, some are not.
fn getCSources(b: *std.Build, allocator: std.mem.Allocator) ![]*CSourceFiles {
    var res = std.ArrayList(*CSourceFiles).init(allocator);

    var index: u32 = 0;

    // list may be appended to during the loop, so use a while
    while (index < compile_steps.?.items.len) {
        const step = compile_steps.?.items[index];

        var shared_flags = std.ArrayList([]const u8).init(allocator);
        defer shared_flags.deinit();

        // catch all the system libraries being linked, make flags out of them
        for (step.link_objects.items) |link_object| {
            switch (link_object) {
                .system_lib => |lib| try shared_flags.append(try common.linkFlag(allocator, lib.name)),
                else => {},
            }
        }

        if (step.is_linking_libc) {
            try shared_flags.append(try common.linkFlag(allocator, "c"));
        }
        if (step.is_linking_libcpp) {
            try shared_flags.append(try common.linkFlag(allocator, "c++"));
        }

        // do the same for include directories
        for (step.include_dirs.items) |include_dir| {
            switch (include_dir) {
                .other_step => |other_step| {
                    const arraylist_header_dirs = extractHeaderDirsFromStep(allocator, other_step);
                    for (arraylist_header_dirs.items) |header_dir| {
                        try shared_flags.append(try common.includeFlag(allocator, header_dir));
                    }
                },
                .path => |path| try shared_flags.append(try common.includeFlag(allocator, path.getPath(b))),
                .path_system => |path| try shared_flags.append(try common.includeFlag(allocator, path.getPath(b))),
                // TODO: support this
                .config_header_step => {},
            }
        }

        for (step.link_objects.items) |link_object| {
            switch (link_object) {
                .static_path => {
                    continue;
                },
                .other_step => {
                    try compile_steps.?.append(link_object.other_step);
                },
                .system_lib => {
                    continue;
                },
                .assembly_file => {
                    continue;
                },
                .c_source_file => {
                    // convert C source file into c source fileS
                    const path = link_object.c_source_file.file.getPath(b);
                    var files_mem = try allocator.alloc([]const u8, 1);
                    files_mem[0] = path;

                    var source_file = try allocator.create(CSourceFiles);

                    var flags = std.ArrayList([]const u8).init(allocator);
                    try flags.appendSlice(link_object.c_source_file.flags);
                    try flags.appendSlice(shared_flags.items);

                    source_file.* = CSourceFiles{
                        .files = files_mem,
                        .flags = try flags.toOwnedSlice(),
                    };

                    try res.append(source_file);
                },
                .c_source_files => {
                    var source_files = link_object.c_source_files;
                    var flags = std.ArrayList([]const u8).init(allocator);
                    try flags.appendSlice(source_files.flags);
                    try flags.appendSlice(shared_flags.items);
                    source_files.flags = try flags.toOwnedSlice();

                    try res.append(source_files);
                },
            }
        }
        index += 1;
    }

    return try res.toOwnedSlice();
}

pub fn makeCdb(step: *std.Build.Step, prog_node: *std.Progress.Node) anyerror!void {
    if (compile_steps == null) {
        @panic("No compile steps registered. Call registerCompileSteps before creating the makeCdb step.");
    }
    _ = prog_node;
    var allocator = step.owner.allocator;

    var compile_commands = std.ArrayList(CompileCommandEntry).init(allocator);
    defer compile_commands.deinit();

    // initialize file and struct containing its future contents
    const cwd: std.fs.Dir = std.fs.cwd();
    var file = try cwd.createFile("compile_commands.json", .{});
    defer file.close();

    const cwd_string = try toString(cwd, allocator);
    const c_sources = try getCSources(step.owner, allocator);

    // fill compile command entries, one for each file
    for (c_sources) |c_source_file_set| {
        const flags = c_source_file_set.flags;
        for (c_source_file_set.files) |c_file| {
            const file_str = std.fs.path.join(allocator, &[_][]const u8{ cwd_string, c_file }) catch @panic("OOM");
            const output_str = std.fmt.allocPrint(allocator, "{s}.o", .{file_str}) catch @panic("OOM");

            var arguments = std.ArrayList([]const u8).init(allocator);
            try arguments.append("clang"); // pretend this is clang compiling
            try arguments.append(c_file);
            try arguments.appendSlice(&.{ "-o", std.fmt.allocPrint(allocator, "{s}.o", .{c_file}) catch @panic("OOM") });
            try arguments.appendSlice(flags);

            // add host native include dirs and libs
            // (doesn't really help unless your include dirs change after generating this)
            // {
            //     var native_paths = try std.zig.system.NativePaths.detect(allocator, step.owner.host);
            //     defer native_paths.deinit();
            //     // native_paths also has lib_dirs. probably not relevant to clangd and compile_commands.json
            //     for (native_paths.include_dirs.items) |include_dir| {
            //         try arguments.append(try common.includeFlag(allocator, include_dir));
            //     }
            // }

            const entry = CompileCommandEntry{
                .arguments = try arguments.toOwnedSlice(),
                .output = output_str,
                .file = file_str,
                .directory = cwd_string,
            };
            try compile_commands.append(entry);
        }
    }

    try writeCompileCommands(&file, compile_commands.items);
}

fn writeCompileCommands(file: *std.fs.File, compile_commands: []CompileCommandEntry) !void {
    const options = std.json.StringifyOptions{
        .whitespace = .indent_tab,
        .emit_null_optional_fields = false,
    };

    try std.json.stringify(compile_commands, options, file.*.writer());
}
