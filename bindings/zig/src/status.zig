const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");

pub const expected_c_abi_version: u32 = 0;

pub const NativeStatusError = error{
    InvalidArgument,
    InvalidState,
    WrongThread,
    Unsupported,
    Cancelled,
    Busy,
    TargetLost,
    NotReady,
    NotFound,
    NativeError,
    UnknownStatus,
};

pub const BindingError = NativeStatusError || error{
    ClosedHandle,
    ActiveBorrow,
    InvalidString,
    AbiVersionMismatch,
    AlreadyCompleted,
};

pub const Error = BindingError || std.mem.Allocator.Error;

/// Calls a status-returning native function with `arguments` and a trailing
/// diagnostic, and converts its status. A failure copies the raw status and
/// the native message into `diagnostic_store`.
pub fn call(
    comptime function: anytype,
    arguments: anytype,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) Error!void {
    var diagnostic: c.mln_diagnostic = undefined;
    diagnostic.size = @sizeOf(c.mln_diagnostic);
    // Without a store the message has nowhere to go, so native skips it.
    const out_diagnostic: [*c]c.mln_diagnostic = if (diagnostic_store != null) &diagnostic else null;
    const raw_status: i32 = @call(.auto, function, arguments ++ .{out_diagnostic});
    if (raw_status == c.MLN_STATUS_OK) return;
    if (diagnostic_store) |store| try store.set(raw_status, std.mem.sliceTo(&diagnostic.message, 0));
    return nativeStatusError(raw_status);
}

pub fn validateAbiVersion(diagnostic_store: ?*diagnostics.DiagnosticStore) Error!void {
    return validateAbiVersionValue(c.mln_c_version(), expected_c_abi_version, diagnostic_store);
}

pub fn validateAbiVersionValue(
    actual: u32,
    expected: u32,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) Error!void {
    if (actual == expected) return;
    if (diagnostic_store) |store| {
        var buffer: [96]u8 = undefined;
        const message = std.fmt.bufPrint(
            &buffer,
            "unsupported MapLibre Native C ABI version: expected {d}, got {d}",
            .{ expected, actual },
        ) catch "unsupported MapLibre Native C ABI version";
        try store.set(null, message);
    }
    return error.AbiVersionMismatch;
}

pub fn setBindingDiagnostic(
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    message: []const u8,
) std.mem.Allocator.Error!void {
    if (diagnostic_store) |store| try store.set(null, message);
}

/// Converts a raw native status into this binding's error set without touching
/// diagnostics: void for MLN_STATUS_OK, the mapped error otherwise.
pub fn errorFromRawStatus(raw_status: i32) NativeStatusError!void {
    if (raw_status == c.MLN_STATUS_OK) return;
    return nativeStatusError(raw_status);
}

fn nativeStatusError(raw_status: i32) NativeStatusError {
    return switch (raw_status) {
        c.MLN_STATUS_INVALID_ARGUMENT => error.InvalidArgument,
        c.MLN_STATUS_INVALID_STATE => error.InvalidState,
        c.MLN_STATUS_WRONG_THREAD => error.WrongThread,
        c.MLN_STATUS_UNSUPPORTED => error.Unsupported,
        c.MLN_STATUS_BUSY => error.Busy,
        c.MLN_STATUS_TARGET_LOST => error.TargetLost,
        c.MLN_STATUS_NOT_READY => error.NotReady,
        c.MLN_STATUS_NOT_FOUND => error.NotFound,
        c.MLN_STATUS_NATIVE_ERROR => error.NativeError,
        c.MLN_STATUS_CANCELLED => error.Cancelled,
        else => error.UnknownStatus,
    };
}

test "call copies the native diagnostic of a failed call" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    const descriptor = c.mln_completion{
        .size = @sizeOf(c.mln_completion),
        .callback = struct {
            fn callback(_: ?*anyopaque, _: [*c]const c.mln_completion_result) callconv(.c) void {}
        }.callback,
        .user_data = null,
        .release_user_data = null,
    };
    try std.testing.expectError(error.InvalidArgument, call(c.mln_runtime_release, .{ 0, &descriptor }, &store));
    const diagnostic = store.get().?;
    try std.testing.expectEqual(@as(?i32, c.MLN_STATUS_INVALID_ARGUMENT), diagnostic.raw_status);
    try std.testing.expect(diagnostic.message.len > 0);
}

test "unknown status preserves raw status" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    const unknown_raw_status: i32 = -9999;
    const unknown = struct {
        fn function(out_diagnostic: [*c]c.mln_diagnostic) callconv(.c) c.mln_status {
            out_diagnostic[0].message[0] = 0;
            return unknown_raw_status;
        }
    }.function;
    try std.testing.expectError(error.UnknownStatus, call(unknown, .{}, &store));
    try std.testing.expectEqual(@as(?i32, unknown_raw_status), store.get().?.raw_status);
}

test "ABI version validation reports mismatch diagnostics" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    try std.testing.expectError(error.AbiVersionMismatch, validateAbiVersionValue(1, 0, &store));
    const diagnostic = store.get().?;
    try std.testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try std.testing.expect(std.mem.indexOf(u8, diagnostic.message, "expected 0, got 1") != null);
}

pub fn rawStatus(err: Error) c.mln_status {
    return switch (err) {
        error.InvalidArgument => c.MLN_STATUS_INVALID_ARGUMENT,
        error.InvalidState => c.MLN_STATUS_INVALID_STATE,
        error.WrongThread => c.MLN_STATUS_WRONG_THREAD,
        error.Unsupported => c.MLN_STATUS_UNSUPPORTED,
        error.Cancelled => c.MLN_STATUS_CANCELLED,
        error.Busy => c.MLN_STATUS_BUSY,
        error.TargetLost => c.MLN_STATUS_TARGET_LOST,
        error.NotReady => c.MLN_STATUS_NOT_READY,
        error.NotFound => c.MLN_STATUS_NOT_FOUND,
        else => c.MLN_STATUS_NATIVE_ERROR,
    };
}
