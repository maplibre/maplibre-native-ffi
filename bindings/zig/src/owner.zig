const std = @import("std");
const status = @import("status.zig");

const log = std.log.scoped(.maplibre_native_ffi);

/// A native generational id. Synchronize release with calls that copy borrowed
/// batch storage, and release each owned id once. An id without a dispose has
/// no deinit.
pub fn Handle(comptime name: []const u8, comptime dispose: ?*const fn (u64) status.Error!void) type {
    return struct {
        const Self = @This();
        pub const native_name = name;
        raw: u64,

        pub fn discarding(self: Self) @import("maplibre_native_ffi.zig").Discarding(Self) {
            return .{ .receiver = self };
        }

        pub fn adopt(raw: u64) Self {
            return .{ .raw = raw };
        }

        /// Disposes on this thread and logs a refusal. Native callback
        /// restrictions apply to disposals that return a status.
        pub fn deinit(self: *Self) void {
            const cleanup = dispose orelse @compileError(name ++ " has no scoped cleanup; release or discard it");
            cleanup(self.raw) catch |err| {
                log.warn("native owner disposal failed: {s}", .{@errorName(err)});
            };
        }
    };
}
