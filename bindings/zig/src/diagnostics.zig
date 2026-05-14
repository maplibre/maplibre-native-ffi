const std = @import("std");

/// Copied diagnostic details from the most recent failing binding operation.
pub const Diagnostic = struct {
    raw_status: ?i32,
    message: []const u8,
};

/// Caller-owned storage for the latest native or binding diagnostic.
///
/// Handles and helper functions borrow this store; callers keep it live for the
/// operation or handle lifetime that uses it.
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
