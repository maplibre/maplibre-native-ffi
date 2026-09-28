const std = @import("std");

/// One-shot latch: `wait` blocks until `set` runs, and returns immediately
/// afterwards.
///
/// Zig's standard library reaches its blocking primitives through an `Io`
/// instance, which a native callback thread does not carry, so this builds the
/// latch from the two vtable-free mutex operations `std.Io.Threaded` exposes.
/// The gate starts locked; `set` releases it, and each waiter releases it again
/// so the next waiter proceeds.
pub const Latch = struct {
    gate: std.Io.Mutex = .{ .state = .init(.locked_once) },

    /// Releases every waiter. Runs at most once per latch.
    pub fn set(self: *Latch) void {
        std.Io.Threaded.mutexUnlock(&self.gate);
    }

    /// Blocks the calling thread until `set` runs.
    pub fn wait(self: *Latch) void {
        std.Io.Threaded.mutexLock(&self.gate);
        std.Io.Threaded.mutexUnlock(&self.gate);
    }
};
