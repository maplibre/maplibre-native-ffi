const std = @import("std");
const status = @import("status.zig");

const log = std.log.scoped(.maplibre_native_ffi);

/// A native generational id. Synchronize release with calls that copy borrowed
/// batch storage, and release each owned id once. An id without a dispose has
/// no deinit.
///
/// The closed state belongs to this value, not to the native handle: the
/// consuming calls take a pointer and close the value they are given. A copy
/// made before then stays open, and native reports its later calls as stale.
pub fn Handle(comptime public_name: []const u8, comptime name: []const u8, comptime dispose: ?*const fn (u64) status.Error!void) type {
    return struct {
        const Self = @This();
        pub const native_name = name;
        raw: u64,
        closed: bool = false,

        pub fn discarding(self: Self) @import("maplibre_native_ffi.zig").Discarding(Self) {
            return .{ .receiver = self };
        }

        pub fn adopt(raw: u64) Self {
            return .{ .raw = raw };
        }

        /// Returns the native handle value, which stays in place after close.
        pub fn id(self: Self) u64 {
            return self.raw;
        }

        /// Returns whether a release, dispose, discard, or deinit of this value
        /// has been accepted. A synchronous release or dispose, or a deinit, of
        /// a closed value does nothing.
        pub fn isClosed(self: Self) bool {
            return self.closed;
        }

        /// Writes `Name(id=0x<hex>, closed=<bool>)` for the `{f}` specifier.
        pub fn format(self: Self, writer: *std.Io.Writer) std.Io.Writer.Error!void {
            try writer.print("{s}(id=0x{x}, closed={})", .{ public_name, self.raw, self.closed });
        }

        /// Disposes on this thread and logs a refusal, unless the value is
        /// already closed. Native callback restrictions apply to disposals
        /// that return a status; a refused disposal leaves the value open, so
        /// a later deinit retries it.
        pub fn deinit(self: *Self) void {
            const cleanup = dispose orelse @compileError(name ++ " has no scoped cleanup; release or discard it");
            if (self.closed) return;
            cleanup(self.raw) catch |err| {
                log.warn("native owner disposal failed: {s}", .{@errorName(err)});
                return;
            };
            self.closed = true;
        }
    };
}
