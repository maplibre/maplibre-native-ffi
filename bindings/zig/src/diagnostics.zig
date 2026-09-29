const std = @import("std");

/// Copied diagnostic details from the most recent failing binding operation.
pub const Diagnostic = struct {
    raw_status: ?i32,
    message: []const u8,
};

/// Caller-owned storage for the latest native or binding diagnostic.
///
/// A function without a handle receiver takes the store as its last argument.
/// A handle keeps the store of the call that created it, passes it to the
/// handles that it creates, and reports its own failed calls into it. Callers
/// keep the store live for the handle lifetimes that use it. The store has no
/// lock, so give each thread that makes failing calls its own store.
pub const DiagnosticStore = struct {
    allocator: std.mem.Allocator,
    latest: ?Diagnostic = null,

    pub fn init(allocator: std.mem.Allocator) DiagnosticStore {
        return .{ .allocator = allocator };
    }

    pub fn deinit(self: *DiagnosticStore) void {
        self.clear();
    }

    pub fn clear(self: *DiagnosticStore) void {
        if (self.latest) |diagnostic| {
            self.allocator.free(diagnostic.message);
            self.latest = null;
        }
    }

    pub fn get(self: *const DiagnosticStore) ?*const Diagnostic {
        if (self.latest) |*diagnostic| return diagnostic;
        return null;
    }

    pub fn set(self: *DiagnosticStore, raw_status: ?i32, message: []const u8) std.mem.Allocator.Error!void {
        const copy = try self.allocator.dupe(u8, message);
        self.clear();
        self.latest = .{
            .raw_status = raw_status,
            .message = copy,
        };
    }
};
