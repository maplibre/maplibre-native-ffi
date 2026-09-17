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

/// Counts callbacks running inside the binding and lets one thread block until
/// they finish.
///
/// `waitUntilIdle` is the quiescence half of a callback teardown: the caller
/// first stops new invocations from starting, then blocks here until the ones
/// already inside return.
pub const UpcallGate = struct {
    active: std.atomic.Value(usize) = .init(0),
    draining: std.atomic.Value(bool) = .init(false),
    idle: Latch = .{},

    pub fn begin(self: *UpcallGate) void {
        _ = self.active.fetchAdd(1, .seq_cst);
    }

    pub fn end(self: *UpcallGate) void {
        if (self.active.fetchSub(1, .seq_cst) != 1) return;
        if (self.draining.load(.seq_cst)) self.idle.set();
    }

    pub fn activeCount(self: *const UpcallGate) usize {
        return self.active.load(.seq_cst);
    }

    /// Blocks until no invocation is running. The caller must already have
    /// stopped new invocations, because a gate that goes idle once never waits
    /// again.
    pub fn waitUntilIdle(self: *UpcallGate) void {
        self.draining.store(true, .seq_cst);
        if (self.active.load(.seq_cst) == 0) return;
        self.idle.wait();
    }
};
