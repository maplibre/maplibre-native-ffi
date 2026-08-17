const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");

pub const expected_c_abi_version: u32 = 1;

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

pub fn checkStatus(status: c.mln_status, diagnostic_store: ?*diagnostics.DiagnosticStore) Error!void {
    const raw_status: i32 = status;
    if (raw_status == c.MLN_STATUS_OK) return;

    const message = copyThreadLastErrorMessage(diagnostic_store, raw_status);
    if (message) |err| return err;

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

fn copyThreadLastErrorMessage(
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    raw_status: i32,
) ?std.mem.Allocator.Error {
    const store = diagnostic_store orelse return null;
    const message = std.mem.span(c.mln_thread_last_error_message());
    store.set(raw_status, message) catch |err| return err;
    return null;
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

test "native status values map to stable Zig errors" {
    try std.testing.expectError(error.InvalidArgument, checkStatus(c.MLN_STATUS_INVALID_ARGUMENT, null));
    try std.testing.expectError(error.InvalidState, checkStatus(c.MLN_STATUS_INVALID_STATE, null));
    try std.testing.expectError(error.WrongThread, checkStatus(c.MLN_STATUS_WRONG_THREAD, null));
    try std.testing.expectError(error.Unsupported, checkStatus(c.MLN_STATUS_UNSUPPORTED, null));
    try std.testing.expectError(error.Cancelled, checkStatus(c.MLN_STATUS_CANCELLED, null));
    try std.testing.expectError(error.Busy, checkStatus(c.MLN_STATUS_BUSY, null));
    try std.testing.expectError(error.TargetLost, checkStatus(c.MLN_STATUS_TARGET_LOST, null));
    try std.testing.expectError(error.NotReady, checkStatus(c.MLN_STATUS_NOT_READY, null));
    try std.testing.expectError(error.NotFound, checkStatus(c.MLN_STATUS_NOT_FOUND, null));
    try std.testing.expectError(error.NativeError, checkStatus(c.MLN_STATUS_NATIVE_ERROR, null));
    try checkStatus(c.MLN_STATUS_OK, null);
}

test "diagnostic store copies thread-local native message" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    var invalid_operation: c.mln_operation = 0;
    try std.testing.expectError(error.InvalidArgument, checkStatus(c.mln_runtime_close_start(0, &invalid_operation), &store));
    const first = store.get().?;
    try std.testing.expectEqual(@as(?i32, c.MLN_STATUS_INVALID_ARGUMENT), first.raw_status);
    try std.testing.expect(first.message.len > 0);
    const copied = try std.testing.allocator.dupe(u8, first.message);
    defer std.testing.allocator.free(copied);

    var source: c.mln_notification_source = 0;
    try checkStatus(c.mln_notification_source_create(&source), null);
    defer checkStatus(c.mln_notification_source_close(source), null) catch
        @panic("notification source close failed");
    var options = c.mln_runtime_options_default();
    options.notification_source = source;
    var create_operation: c.mln_operation = 0;
    try checkStatus(c.mln_runtime_create_start(&options, &create_operation), null);
    defer c.mln_operation_release(create_operation);
    var completed = false;
    try checkStatus(c.mln_operation_wait(create_operation, -1, &completed), null);
    var runtime: c.mln_runtime = 0;
    try checkStatus(c.mln_runtime_create_take_result(create_operation, &runtime), null);
    var close_operation: c.mln_operation = 0;
    try checkStatus(c.mln_runtime_close_start(runtime, &close_operation), null);
    defer c.mln_operation_release(close_operation);
    try checkStatus(c.mln_operation_wait(close_operation, -1, &completed), null);

    try std.testing.expectEqualStrings(copied, store.get().?.message);
}

test "unknown status preserves raw status" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    const unknown_raw_status: i32 = -9999;
    const unknown_status: c.mln_status = unknown_raw_status;
    try std.testing.expectError(error.UnknownStatus, checkStatus(unknown_status, &store));
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
