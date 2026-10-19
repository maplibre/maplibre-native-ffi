const std = @import("std");
const Translator = @import("translate_c").Translator;

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const fixtures: std.Build.LazyPath = .{ .cwd_relative = b.option([]const u8, "fixture-dir", "Protocol fixture directory").? };
    const translator: Translator = .init(b.dependency("translate_c", .{}), .{
        .c_source_file = b.path("api.h"),
        .target = target,
        .optimize = optimize,
    });
    const module = b.createModule(.{
        .root_source_file = b.path("src/probe_test.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    module.addImport("c_api", translator.mod);
    module.addCSourceFile(.{ .file = fixtures.path(b, "protocols_stub.c") });
    module.addIncludePath(fixtures);
    const tests = b.addTest(.{ .root_module = module, .filters = &.{"generated"} });
    b.step("test", "Run generated binding probes").dependOn(&b.addRunArtifact(tests).step);
}
