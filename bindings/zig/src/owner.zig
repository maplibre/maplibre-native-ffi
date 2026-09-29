const std = @import("std");
const status = @import("status.zig");
const callback = @import("callback.zig");
const allocator = std.heap.smp_allocator;

pub const Anchor = struct {
    state: *State,
    pub fn retain(self: Anchor) Anchor {
        self.state.retain();
        return self;
    }
    pub fn release(self: Anchor) void {
        self.state.release();
    }
};

const State = struct {
    references: std.atomic.Value(usize) = .init(1),
    parent: ?Anchor,
    readers: usize = 0,
    closing: bool = false,
    dispose_pending: bool = false,
    finalize_started: bool = false,
    decision_pending: bool = false,
    claimed: bool = false,
    completed: bool = false,
    completing: bool = false,
    release_requested: bool = false,
    finalizer: callback.Finalizer = .{},
    raw: u64,
    dispose: *const fn (u64) status.Error!void,

    fn retain(self: *State) void {
        _ = self.references.fetchAdd(1, .monotonic);
    }
    fn release(self: *State) void {
        if (self.references.fetchSub(1, .acq_rel) != 1) return;
        if (self.parent) |parent| parent.release();
        allocator.destroy(self);
    }
    fn finalize(self: *State) void {
        self.finalizer = .{ .context = self, .run = runFinalizer };
        callback.finalize(&self.finalizer);
    }
    fn runFinalizer(context: *anyopaque) void {
        const self: *State = @ptrCast(@alignCast(context));
        // A failed disposal leaves the native handle to native cleanup; the
        // binding state and its parent anchor are still this finalizer's to free.
        self.dispose(self.raw) catch |err| {
            std.log.err("native owner disposal failed: {s}", .{@errorName(err)});
        };
        self.release();
    }
};

pub fn Handle(comptime name: []const u8, comptime dispose: *const fn (u64) status.Error!void) type {
    return struct {
        const Self = @This();
        pub const native_name = name;
        var mutex: std.Io.Mutex = .init;
        var registry: std.AutoHashMapUnmanaged(u64, *State) = .empty;
        raw: u64,

        fn lock() void {
            std.Io.Threaded.mutexLock(&mutex);
        }
        fn unlock() void {
            std.Io.Threaded.mutexUnlock(&mutex);
        }

        pub fn adopt(raw: u64, parent: ?Anchor) status.Error!Self {
            if (raw == 0) return error.InvalidArgument;
            errdefer dispose(raw) catch {};
            try callback.initialize();
            const state = try allocator.create(State);
            errdefer allocator.destroy(state);
            state.* = .{ .parent = if (parent) |p| p.retain() else null, .raw = raw, .dispose = dispose };
            errdefer if (state.parent) |p| p.release();
            lock();
            defer unlock();
            try registry.put(allocator, raw, state);
            return .{ .raw = raw };
        }

        pub fn beginDecision(raw: u64) status.Error!Self {
            try callback.initialize();
            const state = try allocator.create(State);
            errdefer allocator.destroy(state);
            state.* = .{ .parent = null, .raw = raw, .dispose = dispose, .decision_pending = true };
            lock();
            defer unlock();
            try registry.put(allocator, raw, state);
            return .{ .raw = raw };
        }
        pub fn finishDecision(self: Self, requested: bool) bool {
            lock();
            const state = registry.get(self.raw) orelse {
                unlock();
                return true;
            };
            const accepted = requested or state.claimed or state.completing or state.release_requested;
            state.decision_pending = false;
            const release = state.release_requested and !state.completing;
            if (!accepted or release) _ = registry.remove(self.raw);
            unlock();
            if (!accepted) state.release() else if (release) state.finalize();
            return accepted;
        }
        pub fn beginComplete(self: Self) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return error.InvalidState;
            if (state.closing or state.completing) return error.InvalidState;
            if (state.completed) return error.AlreadyCompleted;
            state.completing = true;
            state.retain();
            return .{ .state = state, .native = self.raw };
        }
        pub const Lease = struct {
            state: *State,
            native: u64,
            reserved: bool = false,
            pub fn finishComplete(self: Lease, accepted: bool) void {
                lock();
                self.state.completing = false;
                if (accepted) {
                    self.state.completed = true;
                    self.state.claimed = true;
                }
                const finalize = self.state.release_requested and !self.state.decision_pending;
                if (finalize) _ = registry.remove(self.native);
                unlock();
                if (finalize) self.state.finalize();
                self.release();
            }
            pub fn anchor(self: Lease) Anchor {
                return .{ .state = self.state };
            }
            pub fn release(self: Lease) void {
                lock();
                if (self.reserved) self.state.readers -= 1;
                const finalize = self.state.readers == 0 and self.state.dispose_pending and !self.state.finalize_started;
                if (finalize) self.state.finalize_started = true;
                unlock();
                if (finalize) self.state.finalize();
                self.state.release();
            }
        };

        pub fn lease(self: Self) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return error.InvalidState;
            if (state.closing) return error.InvalidState;
            state.retain();
            return .{ .state = state, .native = self.raw };
        }

        pub fn borrow(self: Self) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return error.InvalidState;
            if (state.closing) return error.InvalidState;
            state.readers += 1;
            state.retain();
            return .{ .state = state, .native = self.raw, .reserved = true };
        }

        pub const Close = struct {
            state: *State,
            native: u64,
            deferred: bool,
            pub fn rollback(self: Close) void {
                lock();
                self.state.closing = false;
                unlock();
            }
            pub fn commit(self: Close) void {
                lock();
                if (self.deferred) {
                    self.state.release_requested = true;
                    self.state.claimed = true;
                    unlock();
                    return;
                }
                _ = registry.remove(self.native);
                unlock();
                self.state.release();
            }
        };

        pub fn beginClose(self: Self) status.Error!?Close {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return null;
            if (state.release_requested) return null;
            if (state.closing) return error.InvalidState;
            if (state.readers != 0) return error.ActiveBorrow;
            state.closing = true;
            return .{ .state = state, .native = self.raw, .deferred = state.decision_pending or state.completing };
        }

        pub fn deinit(self: *Self) void {
            lock();
            const state = registry.get(self.raw) orelse {
                unlock();
                return;
            };
            if (state.closing) {
                unlock();
                return;
            }
            if (state.decision_pending or state.completing) {
                state.release_requested = true;
                state.claimed = true;
                state.closing = true;
                unlock();
                return;
            }
            _ = registry.remove(self.raw);
            state.closing = true;
            state.dispose_pending = true;
            const finalize = state.readers == 0;
            if (finalize) state.finalize_started = true;
            unlock();
            if (finalize) state.finalize();
        }
    };
}

test "copied owners defer disposal through borrowed copies exactly once" {
    const Probe = struct {
        var disposals: usize = 0;
        fn dispose(_: u64) status.Error!void {
            disposals += 1;
        }
    };
    const Owner = Handle("borrowed-owner-test", Probe.dispose);
    Probe.disposals = 0;
    var value = try Owner.adopt(71, null);
    const copy = value;
    const ordinary = try value.lease();
    const borrowed = try value.borrow();
    try std.testing.expectError(error.ActiveBorrow, copy.beginClose());
    value.deinit();
    try std.testing.expectError(error.InvalidState, copy.lease());
    try std.testing.expectEqual(@as(usize, 0), Probe.disposals);
    borrowed.release();
    ordinary.release();
    try std.testing.expectEqual(@as(usize, 1), Probe.disposals);
    value.deinit();
    try std.testing.expectEqual(@as(usize, 1), Probe.disposals);
}

test "rejected close restores owner and accepted provider actions force ownership" {
    const Probe = struct {
        var disposals: usize = 0;
        fn dispose(_: u64) status.Error!void {
            disposals += 1;
        }
    };
    const Owner = Handle("provider-owner-test", Probe.dispose);
    Probe.disposals = 0;
    var value = try Owner.adopt(81, null);
    const closing = (try value.beginClose()).?;
    try std.testing.expectError(error.InvalidState, value.beginClose());
    closing.rollback();
    (try value.lease()).release();
    value.deinit();
    var request = try Owner.beginDecision(82);
    const rejected = try request.beginComplete();
    rejected.finishComplete(false);
    try std.testing.expect(!request.finishDecision(false));
    try std.testing.expectEqual(@as(usize, 1), Probe.disposals);
    request = try Owner.beginDecision(83);
    const accepted = try request.beginComplete();
    accepted.finishComplete(true);
    try std.testing.expect(request.finishDecision(false));
    request.deinit();
    request = try Owner.beginDecision(84);
    const inline_close = (try request.beginClose()).?;
    try std.testing.expect(inline_close.deferred);
    inline_close.commit();
    try std.testing.expect((try request.beginClose()) == null);
    try std.testing.expect(request.finishDecision(false));
    try std.testing.expectEqual(@as(usize, 3), Probe.disposals);
}
