const std = @import("std");
const status = @import("status.zig");

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
        state.* = .{ .value = value };
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

pub fn Token(comptime T: type) type {
    return struct {
        var mutex: std.Io.Mutex = .init;
        var next: usize = 1;
        var entries: std.AutoHashMapUnmanaged(usize, *Registration(T)) = .empty;
        pub fn create(roots: *Roots, value: T) status.Error!?*anyopaque {
            const state = try roots.retain(T, value);
            std.Io.Threaded.mutexLock(&mutex);
            defer std.Io.Threaded.mutexUnlock(&mutex);
            const token = next;
            next = std.math.add(usize, next, 1) catch return error.OutOfMemory;
            try entries.put(std.heap.smp_allocator, token, state);
            state.token = token;
            roots.items.items[roots.items.items.len - 1].native_release = remove;
            return @ptrFromInt(token);
        }
        pub fn get(token: ?*anyopaque) ?*Registration(T) {
            std.Io.Threaded.mutexLock(&mutex);
            defer std.Io.Threaded.mutexUnlock(&mutex);
            const state = entries.get(@intFromPtr(token)) orelse return null;
            _ = state.refs.fetchAdd(1, .monotonic);
            return state;
        }
        fn remove(context: *anyopaque) void {
            const state: *Registration(T) = @ptrCast(@alignCast(context));
            std.Io.Threaded.mutexLock(&mutex);
            _ = entries.remove(state.token);
            std.Io.Threaded.mutexUnlock(&mutex);
            Registration(T).releaseNativeErased(context);
        }
    };
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

test "inline native release and token retirement preserve caller and dispatch roots" {
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
    {
        var roots: Roots = .{};
        const token = try Token(Probe).create(&roots, .{ .context = &count, .release_context = Probe.released });
        const entry = roots.items.items[0];
        roots.accept();
        roots.deinit();
        const dispatch = Token(Probe).get(token).?;
        entry.releaseNative();
        try std.testing.expect(Token(Probe).get(token) == null);
        try std.testing.expectEqual(@as(usize, 1), count);
        Registration(Probe).releaseErased(dispatch);
    }
    try std.testing.expectEqual(@as(usize, 2), count);
}
