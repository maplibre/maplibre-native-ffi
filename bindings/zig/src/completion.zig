const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");
const sync = @import("sync.zig");

pub const CommandDisposition = @import("maplibre_native_ffi.zig").CommandDisposition;
const Wake = @import("maplibre_native_ffi.zig").Wake;

/// Who holds the wake slot of a future: nobody yet, a registered wake that the
/// completion fires, or nobody ever again because the completion arrived or the
/// owner dropped the future.
const WakeSlot = enum(u8) { idle, armed, settled };

/// Runs a wake the future took, then releases its context.
fn fireWake(wake: Wake) void {
    // A wake only schedules work, so an error it returns has nowhere to go.
    if (wake.callback) |run| run(wake.context) catch {};
    if (wake.release_context) |release| release(wake.context);
}

pub const CommandCompletion = struct {
    disposition: CommandDisposition,
    generation: u64,
    raw_status: i32,

    /// Returns the native error carried by a failed or cancelled command.
    pub fn statusError(self: CommandCompletion) status.NativeStatusError!void {
        return status.errorFromRawStatus(self.raw_status);
    }
};

/// Releases a terminal value the future copied but never handed out.
fn disposeOwned(comptime Value: type, owned: *Value) void {
    switch (@typeInfo(Value)) {
        .optional => |optional| {
            if (owned.*) |*inner| disposeOwned(optional.child, inner);
        },
        .@"struct", .@"enum" => {
            if (@hasDecl(Value, "disposeAbandoned")) {
                owned.disposeAbandoned();
            } else if (@hasDecl(Value, "deinit")) owned.deinit();
        },
        else => {},
    }
}

/// One pending native completion and its copied result. A future has unique
/// ownership: move it rather than copy it, since deinit on two copies releases
/// the native completion twice.
pub fn Future(comptime T: type) type {
    return struct {
        const Self = @This();
        pub const Value = T;
        const Copy = *const fn (*const c.mln_completion_result, ?*anyopaque) status.Error!T;

        const State = struct {
            refs: std.atomic.Value(usize) = .init(2),
            ready: sync.Latch = .{},
            completed: std.atomic.Value(u32) = .init(0),
            consumed: std.atomic.Value(bool) = .init(false),
            raw_status: i32 = c.MLN_STATUS_OK,
            diagnostic: []u8 = &.{},
            value: ?T = null,
            conversion_error: ?status.Error = null,
            copy: Copy,
            copy_context: ?*anyopaque = null,
            release_copy_context: ?*const fn (?*anyopaque) void = null,
            /// Set by the owner's first `notify`, which only the owner calls.
            notified: bool = false,
            wake: Wake = .{},
            wake_slot: std.atomic.Value(WakeSlot) = .init(.idle),

            fn release(state: *State) void {
                if (state.refs.fetchSub(1, .acq_rel) != 1) return;
                if (!state.consumed.load(.acquire)) {
                    if (state.value) |*owned| disposeOwned(T, owned);
                }
                if (state.diagnostic.len != 0) std.heap.smp_allocator.free(state.diagnostic);
                if (state.release_copy_context) |release_context| release_context(state.copy_context);
                std.heap.smp_allocator.destroy(state);
            }

            fn finish(state: *State) void {
                state.completed.store(1, .release);
                state.ready.set();
                if (state.wake_slot.swap(.settled, .acq_rel) == .armed) fireWake(state.wake);
            }
        };

        state: ?*State,

        pub fn poll(self: *const Self) status.BindingError!bool {
            const state = self.state orelse return error.InvalidState;
            return state.completed.load(.acquire) != 0;
        }

        /// Blocks until the completion arrives and hands out its terminal value.
        ///
        /// A failed completion writes its status and message into
        /// `diagnostic`, including a failed command, whose completion `wait`
        /// still returns. A value the caller never takes is released by
        /// `deinit`, including an owned handle returned by a creation
        /// operation.
        pub fn wait(self: *Self, diagnostic: ?*diagnostics.Diagnostic) status.Error!T {
            status.begin(diagnostic);
            errdefer |err| status.fail(diagnostic, err);
            const state = self.state orelse {
                status.record(diagnostic, null, "Future is closed");
                return error.InvalidState;
            };
            state.ready.wait();
            if (state.consumed.swap(true, .acq_rel)) return error.AlreadyCompleted;
            errdefer if (state.value) |*owned| {
                disposeOwned(T, owned);
                state.value = null;
            };
            if (state.raw_status != c.MLN_STATUS_OK) status.record(diagnostic, state.raw_status, state.diagnostic);
            if (state.conversion_error) |conversion_error| return @as(status.Error!T, conversion_error);
            if (T != CommandCompletion) try status.errorFromRawStatus(state.raw_status);
            return state.value orelse unreachable;
        }

        /// Runs `wake` once the completion arrives, without blocking, then
        /// releases its context. A future that is already ready runs and
        /// releases `wake` on the calling thread before `notify` returns.
        /// Otherwise it runs on the thread that delivers the completion, where
        /// it should only schedule work, such as waking a loop that then calls
        /// `wait`, which returns at once.
        ///
        /// A future dropped by `deinit` before its completion arrives releases
        /// the context without running the callback. A future accepts one
        /// wake; a second `notify`, or a `notify` after `deinit`, returns
        /// `error.InvalidState` and leaves the context with the caller.
        pub fn notify(self: *Self, wake: Wake) status.BindingError!void {
            const state = self.state orelse return error.InvalidState;
            if (state.notified) return error.InvalidState;
            state.notified = true;
            state.wake = wake;
            if (state.wake_slot.cmpxchgStrong(.idle, .armed, .acq_rel, .acquire) != null) fireWake(wake);
        }

        pub fn deinit(self: *Self) void {
            const state = self.state orelse return;
            self.state = null;
            if (state.wake_slot.cmpxchgStrong(.armed, .settled, .acq_rel, .acquire) == null) {
                if (state.wake.release_context) |release_context| release_context(state.wake.context);
            }
            state.release();
        }

        fn callback(user_data: ?*anyopaque, raw: [*c]const c.mln_completion_result) callconv(.c) void {
            const state: *State = @ptrCast(@alignCast(user_data orelse return));
            const result: *const c.mln_completion_result = @ptrCast(raw orelse return);
            state.raw_status = result.status;
            if (result.status == c.MLN_STATUS_OK or T == CommandCompletion) {
                state.value = state.copy(result, state.copy_context) catch |err| {
                    state.conversion_error = err;
                    state.finish();
                    return;
                };
            }
            if (result.diagnostic.data != null and result.diagnostic.size != 0) {
                const bytes = @as([*]const u8, @ptrCast(result.diagnostic.data))[0..result.diagnostic.size];
                state.diagnostic = std.heap.smp_allocator.dupe(u8, bytes) catch {
                    state.conversion_error = error.OutOfMemory;
                    state.finish();
                    return;
                };
            }
            state.finish();
        }

        fn releaseUserData(user_data: ?*anyopaque) callconv(.c) void {
            const state: *State = @ptrCast(@alignCast(user_data orelse return));
            state.release();
        }

        fn descriptor(state: *State) c.mln_completion {
            return .{
                .size = @sizeOf(c.mln_completion),
                .callback = callback,
                .user_data = state,
                .release_user_data = releaseUserData,
            };
        }
    };
}

/// Returns a future that already carries `value`, for work the binding
/// satisfied without a native submission. The caller deinitializes it like any
/// other future.
pub fn completed(comptime T: type, terminal_value: T) std.mem.Allocator.Error!Future(T) {
    const FutureType = Future(T);
    const state = try std.heap.smp_allocator.create(FutureType.State);
    state.* = .{
        .refs = .init(1),
        .completed = .init(1),
        .wake_slot = .init(.settled),
        .value = terminal_value,
        .copy = struct {
            fn copyResult(_: *const c.mln_completion_result, _: ?*anyopaque) status.Error!T {
                unreachable;
            }
        }.copyResult,
    };
    state.ready.set();
    return .{ .state = state };
}

pub fn submit(
    comptime T: type,
    diagnostic: ?*diagnostics.Diagnostic,
    comptime copy: *const fn (*const c.mln_completion_result) status.Error!T,
    comptime start: anytype,
    arguments: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    const state = try std.heap.smp_allocator.create(FutureType.State);
    state.* = .{
        .copy = struct {
            fn copyResult(result: *const c.mln_completion_result, _: ?*anyopaque) status.Error!T {
                return copy(result);
            }
        }.copyResult,
    };
    return finishSubmission(T, state, diagnostic, start, arguments);
}

pub fn submitWithCopyContext(
    comptime T: type,
    comptime CopyContext: type,
    diagnostic: ?*diagnostics.Diagnostic,
    comptime copy: *const fn (*const c.mln_completion_result, *CopyContext) status.Error!T,
    copy_context: CopyContext,
    comptime start: anytype,
    arguments: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    const owned_copy_context = std.heap.smp_allocator.create(CopyContext) catch |err| {
        var rejected_context = copy_context;
        disposeOwned(CopyContext, &rejected_context);
        return err;
    };
    owned_copy_context.* = copy_context;
    const state = std.heap.smp_allocator.create(FutureType.State) catch |err| {
        disposeOwned(CopyContext, owned_copy_context);
        std.heap.smp_allocator.destroy(owned_copy_context);
        return err;
    };
    state.* = .{
        .copy = struct {
            fn copyResult(result: *const c.mln_completion_result, erased: ?*anyopaque) status.Error!T {
                const typed: *CopyContext = @ptrCast(@alignCast(erased orelse return error.NativeError));
                return copy(result, typed);
            }
        }.copyResult,
        .copy_context = @ptrCast(owned_copy_context),
        .release_copy_context = struct {
            fn release(erased: ?*anyopaque) void {
                const typed: *CopyContext = @ptrCast(@alignCast(erased orelse return));
                disposeOwned(CopyContext, typed);
                std.heap.smp_allocator.destroy(typed);
            }
        }.release,
    };
    return finishSubmission(T, state, diagnostic, start, arguments);
}

fn finishSubmission(
    comptime T: type,
    state: *Future(T).State,
    diagnostic: ?*diagnostics.Diagnostic,
    comptime start: anytype,
    arguments: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    var future = FutureType{ .state = state };
    const completion_descriptor = FutureType.descriptor(state);
    status.call(start, arguments ++ .{&completion_descriptor}, diagnostic) catch |err| {
        state.release();
        future.deinit();
        return err;
    };
    return future;
}

pub fn unit(result: *const c.mln_completion_result) status.Error!void {
    if (result.value != null or result.value_count != 0) return error.NativeError;
}

pub fn command(result: *const c.mln_completion_result) status.Error!CommandCompletion {
    if (result.value != null or result.value_count != 0) return error.NativeError;
    return .{
        .disposition = CommandDisposition.fromNative(result.disposition),
        .generation = result.generation,
        .raw_status = result.status,
    };
}

pub fn value(comptime T: type) *const fn (*const c.mln_completion_result) status.Error!T {
    return struct {
        fn copy(result: *const c.mln_completion_result) status.Error!T {
            if (result.value == null or result.value_count != 1) return error.NativeError;
            const pointer: *align(1) const T = @ptrCast(result.value.?);
            return pointer.*;
        }
    }.copy;
}

/// Stands in for a native submission: it keeps the completion descriptor it
/// receives, so a test delivers the result and retires the user data itself,
/// in the order it chooses.
const FakeSubmission = struct {
    var descriptor: c.mln_completion = undefined;
    var refuse = false;

    fn start(completion: [*c]const c.mln_completion, _: [*c]c.mln_diagnostic) callconv(.c) c.mln_status {
        if (refuse) return c.MLN_STATUS_INVALID_ARGUMENT;
        descriptor = completion[0];
        return c.MLN_STATUS_OK;
    }

    fn deliver(value_count: usize) void {
        const payload: u32 = 7;
        const result = std.mem.zeroInit(c.mln_completion_result, .{
            .size = @sizeOf(c.mln_completion_result),
            .status = c.MLN_STATUS_OK,
            .value = @as(?*const anyopaque, &payload),
            .value_count = value_count,
        });
        descriptor.callback.?(descriptor.user_data, &result);
    }

    fn retire() void {
        descriptor.release_user_data.?(descriptor.user_data);
    }
};

/// A terminal value that counts its disposals.
const Tracked = struct {
    var disposals: usize = 0;
    payload: u32,

    pub fn deinit(_: *Tracked) void {
        disposals += 1;
    }

    fn copy(result: *const c.mln_completion_result) status.Error!Tracked {
        if (result.value == null or result.value_count != 1) return error.NativeError;
        return .{ .payload = @as(*const u32, @ptrCast(@alignCast(result.value.?))).* };
    }
};

// A completion hands out its value once. A value that fails to convert never
// becomes a binding value: the conversion runs in the completion callback and
// disposes what it built (a generated copy adopts a native handle through
// owner.adopt, which disposes the handle when adoption fails), so the future
// reports the error once and has nothing of its own to dispose.
test "a completion delivers once, and a value that fails to convert reports the error once" {
    Tracked.disposals = 0;
    FakeSubmission.refuse = false;
    var future = try submit(Tracked, null, Tracked.copy, FakeSubmission.start, .{});
    try std.testing.expect(!try future.poll());
    FakeSubmission.deliver(1);
    FakeSubmission.retire();
    try std.testing.expect(try future.poll());
    try std.testing.expectEqual(@as(u32, 7), (try future.wait(null)).payload);
    try std.testing.expectError(error.AlreadyCompleted, future.wait(null));
    // The caller took the value, so deinit leaves it to the caller.
    future.deinit();
    try std.testing.expectEqual(@as(usize, 0), Tracked.disposals);
    var closed: diagnostics.Diagnostic = .{};
    try std.testing.expectError(error.InvalidState, future.wait(&closed));
    try std.testing.expectEqualStrings("Future is closed", closed.message());

    var failed = try submit(Tracked, null, Tracked.copy, FakeSubmission.start, .{});
    defer failed.deinit();
    FakeSubmission.deliver(2);
    FakeSubmission.retire();
    var diagnostic: diagnostics.Diagnostic = .{};
    try std.testing.expectError(error.NativeError, failed.wait(&diagnostic));
    try std.testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try std.testing.expectEqualStrings("native returned a value that the binding cannot read", diagnostic.message());
    try std.testing.expectError(error.AlreadyCompleted, failed.wait(null));
}

/// A copy context that counts how often the binding releases it.
const CountedContext = struct {
    var releases: usize = 0;
    unused: u8 = 0,

    pub fn deinit(_: *CountedContext) void {
        releases += 1;
    }

    fn copy(result: *const c.mln_completion_result, _: *CountedContext) status.Error!Tracked {
        return Tracked.copy(result);
    }
};

test "a refused submission frees the future and its copy context" {
    CountedContext.releases = 0;
    FakeSubmission.refuse = true;
    defer FakeSubmission.refuse = false;
    var diagnostic: diagnostics.Diagnostic = .{};
    try std.testing.expectError(error.InvalidArgument, submitWithCopyContext(Tracked, CountedContext, &diagnostic, CountedContext.copy, .{}, FakeSubmission.start, .{}));
    try std.testing.expectEqual(@as(?i32, c.MLN_STATUS_INVALID_ARGUMENT), diagnostic.raw_status);
    try std.testing.expectEqual(@as(usize, 1), CountedContext.releases);
}

// Zig futures have no timed wait: `poll` checks without blocking, and dropping
// the future abandons the wait. A value that native delivers afterwards has no
// taker, so the binding disposes it.
test "a pending future polls without blocking and deinit abandons it" {
    Tracked.disposals = 0;
    FakeSubmission.refuse = false;
    var future = try submit(Tracked, null, Tracked.copy, FakeSubmission.start, .{});
    try std.testing.expect(!try future.poll());
    future.deinit();
    try std.testing.expectError(error.InvalidState, future.poll());
    FakeSubmission.deliver(1);
    FakeSubmission.retire();
    try std.testing.expectEqual(@as(usize, 1), Tracked.disposals);
}

/// A wake context that counts its runs and releases, and records whether the
/// future it watches was ready when the wake ran.
const CountedWake = struct {
    watched: ?*const Future(u32) = null,
    runs: std.atomic.Value(usize) = .init(0),
    releases: std.atomic.Value(usize) = .init(0),
    ready_when_run: bool = false,

    fn wake(self: *CountedWake) Wake {
        return .{ .context = self, .callback = run, .release_context = release };
    }

    fn run(context: ?*anyopaque) status.Error!void {
        const self: *CountedWake = @ptrCast(@alignCast(context.?));
        if (self.watched) |watched| self.ready_when_run = try watched.poll();
        _ = self.runs.fetchAdd(1, .acq_rel);
    }

    fn release(context: ?*anyopaque) void {
        const self: *CountedWake = @ptrCast(@alignCast(context.?));
        _ = self.releases.fetchAdd(1, .acq_rel);
    }

    fn expectCounts(self: *const CountedWake, runs: usize, releases: usize) !void {
        try std.testing.expectEqual(runs, self.runs.load(.acquire));
        try std.testing.expectEqual(releases, self.releases.load(.acquire));
    }
};

test "a wake registered before the completion runs once when it arrives, then releases" {
    FakeSubmission.refuse = false;
    var future = try submit(u32, null, value(u32), FakeSubmission.start, .{});
    defer future.deinit();
    var counted: CountedWake = .{ .watched = &future };
    try future.notify(counted.wake());
    var second: CountedWake = .{};
    try std.testing.expectError(error.InvalidState, future.notify(second.wake()));
    try counted.expectCounts(0, 0);

    FakeSubmission.deliver(1);
    try counted.expectCounts(1, 1);
    try std.testing.expect(counted.ready_when_run);
    FakeSubmission.retire();
    try std.testing.expectEqual(@as(u32, 7), try future.wait(null));
    try counted.expectCounts(1, 1);
    try second.expectCounts(0, 0);
}

test "a wake registered after the completion runs and releases before notify returns" {
    FakeSubmission.refuse = false;
    var future = try submit(u32, null, value(u32), FakeSubmission.start, .{});
    defer future.deinit();
    FakeSubmission.deliver(1);
    FakeSubmission.retire();
    var counted: CountedWake = .{ .watched = &future };
    try future.notify(counted.wake());
    try counted.expectCounts(1, 1);
    try std.testing.expect(counted.ready_when_run);

    var ready = try completed(u32, 3);
    defer ready.deinit();
    var on_ready: CountedWake = .{};
    try ready.notify(on_ready.wake());
    try on_ready.expectCounts(1, 1);
}

test "deinit before the completion releases the wake without running it" {
    FakeSubmission.refuse = false;
    var future = try submit(u32, null, value(u32), FakeSubmission.start, .{});
    var counted: CountedWake = .{};
    try future.notify(counted.wake());
    future.deinit();
    try counted.expectCounts(0, 1);
    var late: CountedWake = .{};
    try std.testing.expectError(error.InvalidState, future.notify(late.wake()));

    FakeSubmission.deliver(1);
    FakeSubmission.retire();
    try counted.expectCounts(0, 1);
    try late.expectCounts(0, 0);
}

// The completion and the owner race for the wake on different threads. Exactly
// one side takes it, so the context is released once, and it runs only when
// the completion took it.
test "a completion racing notify or deinit releases the wake exactly once" {
    FakeSubmission.refuse = false;
    const Deliverer = struct {
        fn run() void {
            FakeSubmission.deliver(1);
            FakeSubmission.retire();
        }
    };
    for (0..200) |round| {
        var future = try submit(u32, null, value(u32), FakeSubmission.start, .{});
        var counted: CountedWake = .{};
        const deliverer = try std.Thread.spawn(.{}, Deliverer.run, .{});
        try future.notify(counted.wake());
        if (round % 2 == 0) {
            deliverer.join();
            try counted.expectCounts(1, 1);
            future.deinit();
        } else {
            future.deinit();
            deliverer.join();
            try std.testing.expectEqual(@as(usize, 1), counted.releases.load(.acquire));
            try std.testing.expect(counted.runs.load(.acquire) <= 1);
        }
    }
}
