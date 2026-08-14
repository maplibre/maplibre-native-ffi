const std = @import("std");
const testing = std.testing;
const maplibre = @import("maplibre_native_ffi");

test "Phase 3 render driver values are exposed without a binding scheduler" {
    const core = maplibre.RenderSessionAttachOptions{
        .driver = .core_worker,
        .requested_texture_ring_depth = 3,
    };
    const caller = maplibre.RenderSessionAttachOptions{
        .driver = .caller_graphics_thread,
        .requested_texture_ring_depth = 1,
    };
    try testing.expectEqual(maplibre.RenderDriver.core_worker, core.driver);
    try testing.expectEqual(@as(u32, 3), core.requested_texture_ring_depth);
    try testing.expectEqual(maplibre.RenderDriver.caller_graphics_thread, caller.driver);
}

test "frame demand carries cadence identity and deadline" {
    const demand = maplibre.FrameDemand{
        .if_needed = false,
        .present = true,
        .token = 42,
        .coalescing_boundary = 7,
        .presentation_time_ns = 1_000,
        .deadline_ns = 2_000,
    };
    try testing.expect(!demand.if_needed);
    try testing.expect(demand.present);
    try testing.expectEqual(@as(u64, 42), demand.token);
    try testing.expectEqual(@as(i64, 2_000), demand.deadline_ns);
}

test "all terminal frame dispositions remain distinct" {
    const values = [_]maplibre.RenderResult{
        .rendered,
        .no_update,
        .size_pending,
        .target_not_ready,
        .superseded,
        .deadline_missed,
    };
    inline for (values, 0..) |left, left_index| inline for (values, 0..) |right, right_index| {
        if (left_index == right_index) continue;
        try testing.expect(std.meta.activeTag(left) != std.meta.activeTag(right));
    };
}

test "CPU-complete synchronization is the explicit baseline" {
    const sync: maplibre.GpuSync = .cpu_complete;
    try testing.expectEqual(.cpu_complete, std.meta.activeTag(sync));
}

test "WebGL exposes existing and transferred-canvas context placement" {
    const existing: maplibre.OpenGLContextDescriptor = .{ .webgl = .{ .existing = 5 } };
    const transferred: maplibre.OpenGLContextDescriptor = .{ .webgl = .{ .transferred_canvas = "#map" } };
    try testing.expectEqual(.webgl, std.meta.activeTag(existing));
    try testing.expectEqual(.webgl, std.meta.activeTag(transferred));
}
