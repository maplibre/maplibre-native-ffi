const std = @import("std");
const status = @import("status.zig");
const callback = @import("callback.zig");
const sync = @import("sync.zig");
const Diagnostic = @import("diagnostics.zig").Diagnostic;
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

/// The owner of one native handle type. `name` is the C type and `public`
/// the generated type that lifecycle errors name.
pub fn Handle(comptime name: []const u8, comptime public: []const u8, comptime dispose: *const fn (u64) status.Error!void) type {
    return struct {
        const Self = @This();
        pub const native_name = name;
        /// The public type name that lifecycle errors name.
        pub const type_name = public;
        var mutex: std.Io.Mutex = .init;
        var registry: std.AutoHashMapUnmanaged(u64, *State) = .empty;
        raw: u64,

        fn lock() void {
            std.Io.Threaded.mutexLock(&mutex);
        }
        fn unlock() void {
            std.Io.Threaded.mutexUnlock(&mutex);
        }

        /// Records that this handle is `state` (closed, closing, or in use)
        /// and returns the invalid-state error, which carries no native
        /// status.
        fn unavailable(diagnostic: ?*Diagnostic, comptime state: []const u8) status.Error {
            status.record(diagnostic, null, type_name ++ " " ++ state);
            return error.InvalidState;
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
        pub fn beginComplete(self: Self, diagnostic: ?*Diagnostic) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return unavailable(diagnostic, "is closed");
            if (state.closing) return unavailable(diagnostic, "is closing");
            if (state.completing) return unavailable(diagnostic, "is in use");
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

        pub fn lease(self: Self, diagnostic: ?*Diagnostic) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return unavailable(diagnostic, "is closed");
            if (state.closing) return unavailable(diagnostic, "is closing");
            state.retain();
            return .{ .state = state, .native = self.raw };
        }

        pub fn borrow(self: Self, diagnostic: ?*Diagnostic) status.Error!Lease {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return unavailable(diagnostic, "is closed");
            if (state.closing) return unavailable(diagnostic, "is closing");
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

        pub fn beginClose(self: Self, diagnostic: ?*Diagnostic) status.Error!?Close {
            lock();
            defer unlock();
            const state = registry.get(self.raw) orelse return null;
            if (state.release_requested) return null;
            if (state.closing) return unavailable(diagnostic, "is closing");
            if (state.readers != 0) return unavailable(diagnostic, "is in use");
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
    const Owner = Handle("mln_borrowed_owner_test", "BorrowedOwnerTest", Probe.dispose);
    Probe.disposals = 0;
    var value = try Owner.adopt(71, null);
    const copy = value;
    const ordinary = try value.lease(null);
    const borrowed = try value.borrow(null);
    var diagnostic: Diagnostic = .{};
    try std.testing.expectError(error.InvalidState, copy.beginClose(&diagnostic));
    try std.testing.expectEqualStrings("BorrowedOwnerTest is in use", diagnostic.message());
    try std.testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    value.deinit();
    try std.testing.expectError(error.InvalidState, copy.lease(&diagnostic));
    try std.testing.expectEqualStrings("BorrowedOwnerTest is closed", diagnostic.message());
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
    const Owner = Handle("mln_provider_owner_test", "ProviderOwnerTest", Probe.dispose);
    Probe.disposals = 0;
    var value = try Owner.adopt(81, null);
    const closing = (try value.beginClose(null)).?;
    var diagnostic: Diagnostic = .{};
    try std.testing.expectError(error.InvalidState, value.beginClose(&diagnostic));
    try std.testing.expectEqualStrings("ProviderOwnerTest is closing", diagnostic.message());
    try std.testing.expectError(error.InvalidState, value.lease(&diagnostic));
    try std.testing.expectEqualStrings("ProviderOwnerTest is closing", diagnostic.message());
    closing.rollback();
    (try value.lease(null)).release();
    value.deinit();
    var request = try Owner.beginDecision(82);
    const rejected = try request.beginComplete(null);
    rejected.finishComplete(false);
    try std.testing.expect(!request.finishDecision(false));
    try std.testing.expectEqual(@as(usize, 1), Probe.disposals);
    request = try Owner.beginDecision(83);
    const accepted = try request.beginComplete(null);
    accepted.finishComplete(true);
    try std.testing.expect(request.finishDecision(false));
    request.deinit();
    request = try Owner.beginDecision(84);
    const inline_close = (try request.beginClose(null)).?;
    try std.testing.expect(inline_close.deferred);
    inline_close.commit();
    try std.testing.expect((try request.beginClose(null)) == null);
    try std.testing.expect(request.finishDecision(false));
    try std.testing.expectEqual(@as(usize, 3), Probe.disposals);
}

/// Records the thread each disposal runs on.
const ThreadProbe = struct {
    var disposals: std.atomic.Value(usize) = .init(0);
    var thread: std.atomic.Value(std.Thread.Id) = .init(0);
    var disposed: sync.Latch = .{};

    fn reset() void {
        disposals.store(0, .release);
        thread.store(0, .release);
        disposed = .{};
    }

    fn dispose(_: u64) status.Error!void {
        thread.store(std.Thread.getCurrentId(), .release);
        _ = disposals.fetchAdd(1, .acq_rel);
        disposed.set();
    }
};

test "a borrow on another thread holds off disposal until it ends" {
    ThreadProbe.reset();
    const Owner = Handle("mln_threaded_borrow_test", "ThreadedBorrowTest", ThreadProbe.dispose);
    var value = try Owner.adopt(91, null);
    var borrowed: sync.Latch = .{};
    var finish: sync.Latch = .{};
    const Borrower = struct {
        fn run(owner: Owner, entered: *sync.Latch, done: *sync.Latch) void {
            const lease = owner.borrow(null) catch return entered.set();
            entered.set();
            done.wait();
            lease.release();
        }
    };
    const thread = try std.Thread.spawn(.{}, Borrower.run, .{ value, &borrowed, &finish });
    borrowed.wait();
    try std.testing.expectError(error.InvalidState, value.beginClose(null));
    value.deinit();
    try std.testing.expectEqual(@as(usize, 0), ThreadProbe.disposals.load(.acquire));
    finish.set();
    thread.join();
    // The borrow's release is the last reference, so the dispose runs on the
    // borrowing thread as it ends.
    try std.testing.expectEqual(@as(usize, 1), ThreadProbe.disposals.load(.acquire));
    try std.testing.expect(ThreadProbe.thread.load(.acquire) != std.Thread.getCurrentId());
}

// A handle abandoned inside a native callback must not dispose on that
// callback's stack, where native may hold locks the disposal needs. The binding
// queues it for its finalizer thread instead, and disposes inline elsewhere.
// Zig has no garbage collector, so `deinit` is always an explicit abandonment
// rather than a leak, and there is no leak for the binding to report.
test "an owner abandoned inside a callback scope disposes off the callback stack" {
    ThreadProbe.reset();
    const Owner = Handle("mln_finalizer_owner_test", "FinalizerOwnerTest", ThreadProbe.dispose);
    var inline_value = try Owner.adopt(101, null);
    inline_value.deinit();
    try std.testing.expectEqual(std.Thread.getCurrentId(), ThreadProbe.thread.load(.acquire));

    ThreadProbe.reset();
    var queued_value = try Owner.adopt(102, null);
    {
        var scope: callback.Scope = .{};
        scope.enter(&.{}, 0);
        defer scope.leave();
        queued_value.deinit();
    }
    ThreadProbe.disposed.wait();
    try std.testing.expectEqual(@as(usize, 1), ThreadProbe.disposals.load(.acquire));
    try std.testing.expect(ThreadProbe.thread.load(.acquire) != std.Thread.getCurrentId());
}
