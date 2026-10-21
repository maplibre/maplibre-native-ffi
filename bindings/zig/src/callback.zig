const std = @import("std");
const status = @import("status.zig");

pub fn reportError(name: []const u8, err: anyerror) void {
    std.log.scoped(.maplibre_native_ffi).warn("{s} failed and native received its fallback: {s}", .{ name, @errorName(err) });
}

// Callback response pointers are valid only on their callback's thread.
threadlocal var current: ?*Scope = null;
pub const Scope = struct {
    previous: ?*Scope = null,
    response: usize = 0,
    pub fn enter(self: *Scope, response: usize) void {
        self.* = .{ .previous = current, .response = response };
        current = self;
    }
    pub fn leave(self: *Scope) void {
        current = self.previous;
    }
};
pub fn checkScoped(response: usize) status.Error!void {
    var scope = current;
    while (scope) |item| : (scope = item.previous) {
        if (response == item.response) return;
    }
    return error.InvalidState;
}

pub const Roots = struct {
    items: std.ArrayList(Entry) = .empty,
    accepted: bool = false,
    pub const Entry = struct {
        context: *anyopaque,
        accept: *const fn (*anyopaque) void,
        release: *const fn (*anyopaque) void,
        native_release: *const fn (*anyopaque) void,
        pub fn releaseNative(self: Entry) void {
            self.native_release(self.context);
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
        fn accept(context: *anyopaque) void {
            const self: *@This() = @ptrCast(@alignCast(context));
            self.accepted = true;
        }
        pub fn releaseErased(context: *anyopaque) void {
            const self: *@This() = @ptrCast(@alignCast(context));
            if (self.refs.fetchSub(1, .acq_rel) != 1) return;
            if (self.accepted) if (self.value.release_context) |release_context| {
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
