const std = @import("std");
const testing = std.testing;

const build_options = @import("build_options");
const c = @import("support.zig").c;

test "supported render backend mask matches selected build backend" {
    const mask = c.mln_supported_render_backend_mask();

    if (build_options.supports_metal) {
        try testing.expectEqual(@as(u32, c.MLN_RENDER_BACKEND_FLAG_METAL), mask);
    } else if (build_options.supports_vulkan) {
        try testing.expectEqual(@as(u32, c.MLN_RENDER_BACKEND_FLAG_VULKAN), mask);
    } else {
        try testing.expectEqual(@as(u32, 0), mask);
    }
}
