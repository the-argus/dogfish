const std = @import("std");
const builtin = @import("builtin");
const app_name = "ode";
const version = "0.16.4";
const here = "./build/deps/";
const srcdir = here ++ "ode/";

const universal_flags = [_][]const u8{
    "-DODE_LIB", // always build statically
    "-DCCD_IDESINGLE", // single precision floating point
    "-DdIDESINGLE",
    "-DdBUILTIN_THREADING_IMPL_ENABLED",
    // OU flags
    "-DdOU_ENABLED",
    "-DdATOMICS_ENABLED",
    "-DdTLS_ENABLED",
    "-D_OU_FEATURE_SET=_OU_FEATURE_SET_TLS", // instead of ATOMICS
    "-D_OU_NAMESPACE=odeou",
    // "-D_OU_FEATURE_SET=_OU_FEATURE_SET_ATOMICS",
    "-DDS_LIB", // also build drawstuff statically
    "-fno-sanitize=undefined",
};

const release_flags = [_][]const u8{"-DNDEBUG"};
const debug_flags = [_][]const u8{};

const windows_flags = [_][]const u8{
    "-D_CRT_SECURE_NO_DEPRECATE",
    "-D_SCL_SECURE_NO_WARNINGS",
    "-D_USE_MATH_DEFINES",
};

const common = @import("./../common.zig");
const include = common.include;
const link = common.link;

const include_dirs = [_][]const u8{
    srcdir ++ "include",
    srcdir ++ "ou/include",
    srcdir ++ "ode/src",
    srcdir ++ "ode/src/joints",
    srcdir ++ "OPCODE",
    srcdir ++ "OPCODE/Ice",
    here ++ "dummyinclude",
};

const drawstuff_sources = [_][]const u8{
    srcdir ++ "drawstuff/src/drawstuff.cpp",
    srcdir ++ "drawstuff/src/x11.cpp",
};

const drawstuff_includes = [_][]const u8{
    srcdir ++ "drawstuff/src",
};

const demo_sources = [_][]const u8{
    srcdir ++ "ode/demo/demo_boxstack.cpp",
    srcdir ++ "ode/demo/demo_buggy.cpp",
    srcdir ++ "ode/demo/demo_cards.cpp",
    srcdir ++ "ode/demo/demo_chain1.c",
    srcdir ++ "ode/demo/demo_chain2.cpp",
    srcdir ++ "ode/demo/demo_collision.cpp",
    srcdir ++ "ode/demo/demo_convex.cpp",
    srcdir ++ "ode/demo/demo_crash.cpp",
    srcdir ++ "ode/demo/demo_cylvssphere.cpp",
    srcdir ++ "ode/demo/demo_dball.cpp",
    srcdir ++ "ode/demo/demo_dhinge.cpp",
    srcdir ++ "ode/demo/demo_feedback.cpp",
    srcdir ++ "ode/demo/demo_friction.cpp",
    srcdir ++ "ode/demo/demo_gyro2.cpp",
    srcdir ++ "ode/demo/demo_gyroscopic.cpp",
    srcdir ++ "ode/demo/demo_heightfield.cpp",
    srcdir ++ "ode/demo/demo_hinge.cpp",
    srcdir ++ "ode/demo/demo_I.cpp",
    srcdir ++ "ode/demo/demo_jointPR.cpp",
    srcdir ++ "ode/demo/demo_jointPU.cpp",
    srcdir ++ "ode/demo/demo_joints.cpp",
    // srcdir ++ "ode/demo/demo_kinematic.cpp", // uses mem_fun, should be std::mem_fn
    srcdir ++ "ode/demo/demo_motion.cpp",
    srcdir ++ "ode/demo/demo_motor.cpp",
    srcdir ++ "ode/demo/demo_ode.cpp",
    srcdir ++ "ode/demo/demo_piston.cpp",
    srcdir ++ "ode/demo/demo_plane2d.cpp",
    srcdir ++ "ode/demo/demo_rfriction.cpp",
    srcdir ++ "ode/demo/demo_slider.cpp",
    srcdir ++ "ode/demo/demo_space.cpp",
    srcdir ++ "ode/demo/demo_space_stress.cpp",
    srcdir ++ "ode/demo/demo_step.cpp",
    srcdir ++ "ode/demo/demo_transmission.cpp",
};

const c_sources = [_][]const u8{
    srcdir ++ "ode/src/array.cpp",
    srcdir ++ "ode/src/box.cpp",
    srcdir ++ "ode/src/capsule.cpp",
    srcdir ++ "ode/src/collision_cylinder_box.cpp",
    srcdir ++ "ode/src/collision_cylinder_plane.cpp",
    srcdir ++ "ode/src/collision_cylinder_sphere.cpp",
    srcdir ++ "ode/src/collision_kernel.cpp",
    srcdir ++ "ode/src/collision_quadtreespace.cpp",
    srcdir ++ "ode/src/collision_sapspace.cpp",
    srcdir ++ "ode/src/collision_space.cpp",
    srcdir ++ "ode/src/collision_transform.cpp",
    srcdir ++ "ode/src/collision_trimesh_disabled.cpp",
    srcdir ++ "ode/src/collision_util.cpp",
    srcdir ++ "ode/src/convex.cpp",
    srcdir ++ "ode/src/cylinder.cpp",
    srcdir ++ "ode/src/default_threading.cpp",
    srcdir ++ "ode/src/error.cpp",
    srcdir ++ "ode/src/export-dif.cpp",
    srcdir ++ "ode/src/fastdot.cpp",
    srcdir ++ "ode/src/fastldltfactor.cpp",
    srcdir ++ "ode/src/fastldltsolve.cpp",
    srcdir ++ "ode/src/fastlsolve.cpp",
    srcdir ++ "ode/src/fastltsolve.cpp",
    srcdir ++ "ode/src/fastvecscale.cpp",
    srcdir ++ "ode/src/heightfield.cpp",
    srcdir ++ "ode/src/lcp.cpp",
    srcdir ++ "ode/src/mass.cpp",
    srcdir ++ "ode/src/mat.cpp",
    srcdir ++ "ode/src/matrix.cpp",
    srcdir ++ "ode/src/memory.cpp",
    srcdir ++ "ode/src/misc.cpp",
    srcdir ++ "ode/src/nextafterf.c",
    srcdir ++ "ode/src/objects.cpp",
    srcdir ++ "ode/src/obstack.cpp",
    srcdir ++ "ode/src/ode.cpp",
    srcdir ++ "ode/src/odeinit.cpp",
    srcdir ++ "ode/src/odemath.cpp",
    srcdir ++ "ode/src/plane.cpp",
    srcdir ++ "ode/src/quickstep.cpp",
    srcdir ++ "ode/src/ray.cpp",
    srcdir ++ "ode/src/resource_control.cpp",
    srcdir ++ "ode/src/rotation.cpp",
    srcdir ++ "ode/src/simple_cooperative.cpp",
    srcdir ++ "ode/src/sphere.cpp",
    srcdir ++ "ode/src/step.cpp",
    srcdir ++ "ode/src/threading_base.cpp",
    srcdir ++ "ode/src/threading_impl.cpp",
    srcdir ++ "ode/src/threading_pool_posix.cpp",
    srcdir ++ "ode/src/threading_pool_win.cpp",
    srcdir ++ "ode/src/timer.cpp",
    srcdir ++ "ode/src/util.cpp",
    srcdir ++ "ode/src/joints/amotor.cpp",
    srcdir ++ "ode/src/joints/ball.cpp",
    srcdir ++ "ode/src/joints/contact.cpp",
    srcdir ++ "ode/src/joints/dball.cpp",
    srcdir ++ "ode/src/joints/dhinge.cpp",
    srcdir ++ "ode/src/joints/fixed.cpp",
    srcdir ++ "ode/src/joints/hinge.cpp",
    srcdir ++ "ode/src/joints/hinge2.cpp",
    srcdir ++ "ode/src/joints/joint.cpp",
    srcdir ++ "ode/src/joints/lmotor.cpp",
    srcdir ++ "ode/src/joints/null.cpp",
    srcdir ++ "ode/src/joints/piston.cpp",
    srcdir ++ "ode/src/joints/plane2d.cpp",
    srcdir ++ "ode/src/joints/pr.cpp",
    srcdir ++ "ode/src/joints/pu.cpp",
    srcdir ++ "ode/src/joints/slider.cpp",
    srcdir ++ "ode/src/joints/transmission.cpp",
    srcdir ++ "ode/src/joints/universal.cpp",

    // now the OPCODE sources
    srcdir ++ "ode/src/collision_convex_trimesh.cpp",
    srcdir ++ "ode/src/collision_cylinder_trimesh.cpp",
    srcdir ++ "ode/src/collision_trimesh_box.cpp",
    srcdir ++ "ode/src/collision_trimesh_ccylinder.cpp",
    srcdir ++ "ode/src/collision_trimesh_internal.cpp",
    srcdir ++ "ode/src/collision_trimesh_opcode.cpp",
    srcdir ++ "ode/src/collision_trimesh_plane.cpp",
    srcdir ++ "ode/src/collision_trimesh_ray.cpp",
    srcdir ++ "ode/src/collision_trimesh_sphere.cpp",
    srcdir ++ "ode/src/collision_trimesh_trimesh.cpp",
    srcdir ++ "ode/src/collision_trimesh_trimesh_old.cpp",
    srcdir ++ "OPCODE/OPC_AABBCollider.cpp",
    srcdir ++ "OPCODE/OPC_AABBTree.cpp",
    srcdir ++ "OPCODE/OPC_BaseModel.cpp",
    srcdir ++ "OPCODE/OPC_Collider.cpp",
    srcdir ++ "OPCODE/OPC_Common.cpp",
    srcdir ++ "OPCODE/OPC_HybridModel.cpp",
    srcdir ++ "OPCODE/OPC_LSSCollider.cpp",
    srcdir ++ "OPCODE/OPC_MeshInterface.cpp",
    srcdir ++ "OPCODE/OPC_Model.cpp",
    srcdir ++ "OPCODE/OPC_OBBCollider.cpp",
    srcdir ++ "OPCODE/OPC_OptimizedTree.cpp",
    srcdir ++ "OPCODE/OPC_Picking.cpp",
    srcdir ++ "OPCODE/OPC_PlanesCollider.cpp",
    srcdir ++ "OPCODE/OPC_RayCollider.cpp",
    srcdir ++ "OPCODE/OPC_SphereCollider.cpp",
    srcdir ++ "OPCODE/OPC_TreeBuilders.cpp",
    srcdir ++ "OPCODE/OPC_TreeCollider.cpp",
    srcdir ++ "OPCODE/OPC_VolumeCollider.cpp",
    srcdir ++ "OPCODE/Opcode.cpp",
    srcdir ++ "OPCODE/Ice/IceAABB.cpp",
    srcdir ++ "OPCODE/Ice/IceContainer.cpp",
    srcdir ++ "OPCODE/Ice/IceHPoint.cpp",
    srcdir ++ "OPCODE/Ice/IceIndexedTriangle.cpp",
    srcdir ++ "OPCODE/Ice/IceMatrix3x3.cpp",
    srcdir ++ "OPCODE/Ice/IceMatrix4x4.cpp",
    srcdir ++ "OPCODE/Ice/IceOBB.cpp",
    srcdir ++ "OPCODE/Ice/IcePlane.cpp",
    srcdir ++ "OPCODE/Ice/IcePoint.cpp",
    srcdir ++ "OPCODE/Ice/IceRandom.cpp",
    srcdir ++ "OPCODE/Ice/IceRay.cpp",
    srcdir ++ "OPCODE/Ice/IceRevisitedRadix.cpp",
    srcdir ++ "OPCODE/Ice/IceSegment.cpp",
    srcdir ++ "OPCODE/Ice/IceTriangle.cpp",
    srcdir ++ "OPCODE/Ice/IceUtils.cpp",

    srcdir ++ "ou/src/ou/atomic.cpp",
    srcdir ++ "ou/src/ou/customization.cpp",
    srcdir ++ "ou/src/ou/malloc.cpp",
    srcdir ++ "ou/src/ou/threadlocalstorage.cpp",
    srcdir ++ "ode/src/odeou.cpp",
    srcdir ++ "ode/src/odetls.cpp",
};

pub fn addLib(b: *std.Build, target: std.zig.CrossTarget, mode: std.builtin.OptimizeMode) !*std.Build.CompileStep {
    var targets = std.ArrayList(*std.Build.CompileStep).init(b.allocator);

    const drawstuff = b.option(bool, "ODE-demos", "Build the ODE \"drawstuff\" library.") orelse false;
    const dslib = if (drawstuff) b.addStaticLibrary(.{
        .name = "drawstuff",
        .optimize = mode,
        .target = target,
    }) else null;
    if (dslib) |ds| try targets.append(ds);

    if (drawstuff and (try std.zig.system.NativeTargetInfo.detect(target)).target.os.tag != .linux) {
        std.log.err("This drawstuff build is unfinished, and only supports linux.", .{});
        return error.UnsupportedOs;
    }

    const lib = b.addStaticLibrary(.{
        .name = app_name,
        .optimize = mode,
        .target = target,
    });
    try targets.append(lib);

    // copied from chipmunk cmake. may be redundant with zig default flags
    // also the compiler is obviously never msvc so idk if the if is necessary
    var flags = std.ArrayList([]const u8).init(b.allocator);

    try flags.appendSlice(&universal_flags);

    if (mode != .Debug) {
        try flags.appendSlice(&debug_flags);
    } else {
        try flags.appendSlice(&release_flags);
    }

    {
        const os_number: i32 = switch ((try std.zig.system.NativeTargetInfo.detect(target)).target.os.tag) {
            .linux => 1,
            .windows => 2,
            // .qnx => 3, // not supported by zig
            .macos => 4,
            .aix => 5,
            // .sunos => 6, //also not supported
            .ios => 7,
            .emscripten, .wasi => 1, // pretend to be linux?
            else => {
                return error.UnsupportedOs;
            },
        };
        try flags.append(try std.fmt.allocPrint(b.allocator, "-D_OU_TARGET_OS={any}", .{os_number}));
    }

    lib.addCSourceFiles(&c_sources, flags.items);

    if (dslib) |ds| {
        for (drawstuff_includes) |include_dir| {
            ds.addIncludePath(include_dir);
        }

        ds.linkLibrary(lib);

        switch (target.getOsTag()) {
            .windows => {},
            .macos => {},
            .linux => {
                ds.linkSystemLibrary("GL");
                ds.linkSystemLibrary("GLU");
                ds.linkSystemLibrary("X11");
            },
            else => {},
        }

        ds.addCSourceFiles(&drawstuff_sources, flags.items);

        for (demo_sources) |demo_source| {
            const demo = b.addExecutable(.{
                .name = std.fs.path.stem(demo_source),
                .optimize = mode,
                .target = target,
            });
            demo.addCSourceFile(demo_source, flags.items);
            demo.linkLibrary(ds);
            try targets.append(demo);
        }
    }

    for (include_dirs) |include_dir| {
        try include(targets, include_dir);
    }

    for (targets.items) |t| {
        t.linkLibCpp();
    }

    {
        const dir = try b.cache_root.join(b.allocator, &[_][]const u8{"ode"});
        const file = try std.fs.path.join(b.allocator, &[_][]const u8{ dir, "precision.h" });
        std.fs.cwd().makePath(dir) catch {};
        const precision_h = try std.fs.cwd().createFile(file, .{});

        const writer = precision_h.writer();
        try writer.print(
            \\#ifndef _ODE_PRECISION_H_
            \\#define _ODE_PRECISION_H_
            \\//#define dDOUBLE
            \\#define dSINGLE
            \\#endif
        , .{});
        defer precision_h.close();

        b.installFile(file, "include/ode/precision.h");
        try include(targets, if (b.cache_root.path) |path| path else return error.RefuseToAddCWDToInclude);
    }

    // always install ode headers
    b.installDirectory(.{
        .source_dir = std.Build.FileSource{ .path = srcdir ++ "include" },
        .install_dir = .header,
        .install_subdir = "",
    });

    b.installDirectory(.{
        .source_dir = std.Build.FileSource{ .path = here ++ "dummyinclude" },
        .install_dir = .header,
        .install_subdir = "",
    });

    for (targets.items) |t| {
        b.installArtifact(t);
    }

    return lib;
}
