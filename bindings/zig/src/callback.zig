const std = @import("std");
const status = @import("status.zig");

/// Receives an error that a host callback returned, or that the binding met
/// while decoding the callback's arguments, along with the C callback type
/// that failed. Native cannot receive the error, so it gets the callback's
/// failure value instead. A reporter may run on any native thread and must
/// return quickly. It runs on the native callback's stack, where the binding
/// refuses every native call with `error.InvalidState`.
pub const ErrorReporter = *const fn (callback: []const u8, err: anyerror) void;

// Native threads read this while the host may replace it, so every access is
// atomic.
var error_reporter: ErrorReporter = logError;

/// Installs `reporter` for every later callback error, or restores the default,
/// which logs a warning in the `maplibre_native_ffi` scope. Returns the reporter
/// it replaced.
pub fn setErrorReporter(reporter: ?ErrorReporter) ErrorReporter {
    return @atomicRmw(ErrorReporter, &error_reporter, .Xchg, reporter orelse logError, .acq_rel);
}

/// Reports an error that the trampoline of `name` contained. The reporter
/// admits no native call, whatever the callback itself may call.
pub fn reportError(name: []const u8, err: anyerror) void {
    var scope: Scope = .{};
    scope.enter(&.{}, 0);
    defer scope.leave();
    @atomicLoad(ErrorReporter, &error_reporter, .acquire)(name, err);
}

fn logError(name: []const u8, err: anyerror) void {
    // A warning rather than an error, since the test runner fails any test that
    // logs an error.
    std.log.scoped(.maplibre_native_ffi).warn("{s} failed and native received its fallback: {s}", .{ name, @errorName(err) });
}

threadlocal var current: ?*Scope = null;
pub const Scope = struct {
    previous: ?*Scope = null,
    operations: []const []const u8 = &.{},
    owner: u64 = 0,
    pub fn enter(self: *Scope, operations: []const []const u8, owner: u64) void {
        self.* = .{ .previous = current, .operations = operations, .owner = owner };
        current = self;
    }
    pub fn leave(self: *Scope) void {
        current = self.previous;
    }
};
pub fn active() bool {
    return current != null;
}
pub fn checkScoped(operation: []const u8, owner: u64) status.Error!void {
    if (current == null) return error.InvalidState;
    return check(operation, owner);
}
pub fn check(operation: []const u8, owner: u64) status.Error!void {
    var scope = current;
    while (scope) |item| : (scope = item.previous) {
        if (owner != item.owner) return error.InvalidState;
        for (item.operations) |allowed| {
            if (std.mem.eql(u8, operation, allowed)) break;
        } else return error.InvalidState;
    }
}

pub const Roots = struct {
    items: std.ArrayList(Entry) = .empty,
    accepted: bool = false,
    /// The receiver of the call that retains these roots. A callback without
    /// an owner parameter calls back only into this receiver.
    owner: u64 = 0,
    pub const Entry = struct {
        context: *anyopaque,
        accept: *const fn (*anyopaque) void,
        release: *const fn (*anyopaque) void,
        native_release: ?*const fn (*anyopaque) void = null,
        pub fn releaseNative(self: Entry) void {
            (self.native_release orelse self.release)(self.context);
        }
    };
    pub fn retain(self: *Roots, comptime T: type, value: T) status.Error!*Registration(T) {
        const RegistrationType = Registration(T);
        const state = try std.heap.smp_allocator.create(RegistrationType);
        errdefer std.heap.smp_allocator.destroy(state);
        state.* = .{ .value = value, .owner = self.owner };
        try self.items.append(std.heap.smp_allocator, .{ .context = state, .accept = RegistrationType.accept, .release = RegistrationType.releaseErased, .native_release = RegistrationType.releaseNativeErased });
        return state;
    }
    pub fn accept(self: *Roots) void {
        self.accepted = true;
        for (self.items.items) |item| item.accept(item.context);
    }
    pub fn deinit(self: *Roots) void {
        for (self.items.items) |item| {
            if (!self.accepted) item.releaseNative();
            item.release(item.context);
        }
        self.items.deinit(std.heap.smp_allocator);
    }
};

pub fn Registration(comptime T: type) type {
    return struct {
        refs: std.atomic.Value(usize) = .init(2),
        accepted: bool = false,
        native_released: std.atomic.Value(bool) = .init(false),
        value: T,
        owner: u64 = 0,
        token: usize = 0,
        fn accept(context: *anyopaque) void {
            const self: *@This() = @ptrCast(@alignCast(context));
            self.accepted = true;
        }
        pub fn releaseErased(context: *anyopaque) void {
            const self: *@This() = @ptrCast(@alignCast(context));
            if (self.refs.fetchSub(1, .acq_rel) != 1) return;
            if (self.accepted) if (self.value.release_context) |release_context| {
                var scope: Scope = .{};
                scope.enter(&.{}, 0);
                defer scope.leave();
                release_context(self.value.context);
            };
            std.heap.smp_allocator.destroy(self);
        }
        pub fn releaseNativeErased(context: *anyopaque) void {
            const self: *@This() = @ptrCast(@alignCast(context));
            if (!self.native_released.swap(true, .acq_rel)) releaseErased(context);
        }
        pub fn releaseNative(context: ?*anyopaque) callconv(.c) void {
            releaseNativeErased(context orelse return);
        }
        pub fn get(context: ?*anyopaque) *@This() {
            return @ptrCast(@alignCast(context.?));
        }
    };
}

pub const Finalizer = struct {
    next: ?*Finalizer = null,
    context: *anyopaque = undefined,
    run: *const fn (*anyopaque) void = undefined,
};
var finalizer_mutex: std.Io.Mutex = .init;
var finalizer_ready: std.Io.Condition = .init;
var finalizer_io: std.Io.Threaded = .init_single_threaded;
var finalizer_started = false;
var finalizer_head: ?*Finalizer = null;
var finalizer_tail: ?*Finalizer = null;

pub fn initialize() status.Error!void {
    if (@import("builtin").target.os.tag == .emscripten) return;
    std.Io.Threaded.mutexLock(&finalizer_mutex);
    defer std.Io.Threaded.mutexUnlock(&finalizer_mutex);
    if (finalizer_started) return;
    const worker = std.Thread.spawn(.{}, runFinalizers, .{}) catch return error.OutOfMemory;
    worker.detach();
    finalizer_started = true;
}
fn runFinalizers() void {
    while (true) {
        std.Io.Threaded.mutexLock(&finalizer_mutex);
        while (finalizer_head == null) finalizer_ready.waitUncancelable(finalizer_io.io(), &finalizer_mutex);
        const task = finalizer_head.?;
        finalizer_head = task.next;
        if (finalizer_head == null) finalizer_tail = null;
        std.Io.Threaded.mutexUnlock(&finalizer_mutex);
        task.run(task.context);
    }
}
extern fn emscripten_async_call(*const fn (?*anyopaque) callconv(.c) void, ?*anyopaque, c_int) void;
pub fn finalize(task: *Finalizer) void {
    if (!active()) {
        task.run(task.context);
        return;
    }
    if (@import("builtin").target.os.tag == .emscripten) {
        emscripten_async_call(struct {
            fn run(context: ?*anyopaque) callconv(.c) void {
                const item: *Finalizer = @ptrCast(@alignCast(context.?));
                item.run(item.context);
            }
        }.run, task, 0);
        return;
    }
    std.Io.Threaded.mutexLock(&finalizer_mutex);
    if (finalizer_tail) |tail| tail.next = task else finalizer_head = task;
    finalizer_tail = task;
    task.next = null;
    finalizer_ready.signal(finalizer_io.io());
    std.Io.Threaded.mutexUnlock(&finalizer_mutex);
}

test "a callback error reporter admits no native call" {
    const Probe = struct {
        var admitted: ?bool = null;
        fn report(_: []const u8, _: anyerror) void {
            admitted = if (check("mln_network_status_get", 0)) |_| true else |_| false;
        }
    };
    const previous = setErrorReporter(Probe.report);
    defer _ = setErrorReporter(previous);
    // The wake callback admits every native call, but its reporter does not.
    reportError("mln_wake_callback", error.NativeError);
    try std.testing.expectEqual(@as(?bool, false), Probe.admitted);
    try check("mln_network_status_get", 0);
}

test "callback policies intersect nested scopes and restore outer admission" {
    var outer: Scope = .{};
    outer.enter(&.{ "complete", "release" }, 7);
    defer outer.leave();
    try check("complete", 7);
    try std.testing.expectError(error.InvalidState, check("complete", 8));
    {
        var inner: Scope = .{};
        inner.enter(&.{"release"}, 7);
        defer inner.leave();
        try check("release", 7);
        try std.testing.expectError(error.InvalidState, check("complete", 7));
    }
    try check("complete", 7);
}

test "inline native release preserves the caller root until acceptance" {
    const Probe = struct {
        context: ?*anyopaque,
        release_context: ?*const fn (?*anyopaque) void,
        fn released(context: ?*anyopaque) void {
            const count: *usize = @ptrCast(@alignCast(context.?));
            count.* += 1;
        }
    };
    var count: usize = 0;
    {
        var roots: Roots = .{};
        const state = try roots.retain(Probe, .{ .context = &count, .release_context = Probe.released });
        Registration(Probe).releaseNative(state);
        try std.testing.expectEqual(@as(usize, 0), count);
        roots.accept();
        roots.deinit();
    }
    try std.testing.expectEqual(@as(usize, 1), count);
}
