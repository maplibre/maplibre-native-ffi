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
    InvalidString,
    AbiVersionMismatch,
    AlreadyCompleted,
};

pub const Error = BindingError || std.mem.Allocator.Error;

/// Starts a call that reports into `diagnostic`, clearing the details of an
/// earlier call.
pub fn begin(diagnostic: ?*diagnostics.Diagnostic) void {
    const target = diagnostic orelse return;
    target.raw_status = null;
    target.native.message[0] = 0;
    target.recorded = false;
}

/// Records a binding error into `diagnostic` unless the failing step already
/// recorded details. Generated functions run it from `errdefer`, so an error
/// from a host callback passes through without a message.
pub fn fail(diagnostic: ?*diagnostics.Diagnostic, err: anyerror) void {
    const target = diagnostic orelse return;
    if (target.recorded) return;
    inline for (@typeInfo(Error).error_set.?) |binding_error| {
        if (err == @field(anyerror, binding_error.name)) return record(target, null, bindingMessage(err));
    }
}

/// Copies `message`, truncated to the buffer, and `raw_status` into
/// `diagnostic`.
pub fn record(diagnostic: ?*diagnostics.Diagnostic, raw_status: ?i32, message: []const u8) void {
    const target = diagnostic orelse return;
    const length = @min(message.len, target.native.message.len - 1);
    @memcpy(target.native.message[0..length], message[0..length]);
    target.native.message[length] = 0;
    target.raw_status = raw_status;
    target.recorded = true;
}

/// Calls a status-returning native function with `arguments` and a trailing
/// diagnostic, and converts its status. Native writes the message of a failure
/// straight into `diagnostic`.
pub fn call(
    comptime function: anytype,
    arguments: anytype,
    diagnostic: ?*diagnostics.Diagnostic,
) Error!void {
    var out_diagnostic: [*c]c.mln_diagnostic = null;
    if (diagnostic) |target| {
        target.native.size = @sizeOf(c.mln_diagnostic);
        out_diagnostic = &target.native;
    }
    const raw_status: i32 = @call(.auto, function, arguments ++ .{out_diagnostic});
    if (raw_status == c.MLN_STATUS_OK) return;
    if (diagnostic) |target| {
        target.raw_status = raw_status;
        target.recorded = true;
    }
    return nativeStatusError(raw_status);
}

pub fn validateAbiVersion(diagnostic: ?*diagnostics.Diagnostic) Error!void {
    return validateAbiVersionValue(c.mln_c_version(), expected_c_abi_version, diagnostic);
}

pub fn validateAbiVersionValue(
    actual: u32,
    expected: u32,
    diagnostic: ?*diagnostics.Diagnostic,
) Error!void {
    begin(diagnostic);
    if (actual == expected) return;
    var buffer: [96]u8 = undefined;
    const message = std.fmt.bufPrint(
        &buffer,
        "unsupported MapLibre Native C ABI version: expected {d}, got {d}",
        .{ expected, actual },
    ) catch "unsupported MapLibre Native C ABI version";
    record(diagnostic, null, message);
    return error.AbiVersionMismatch;
}

fn bindingMessage(err: anyerror) []const u8 {
    return switch (err) {
        error.InvalidString => "the string contains a NUL byte",
        error.AlreadyCompleted => "the operation already completed",
        error.OutOfMemory => "out of memory",
        error.InvalidState => "the handle is not in a state that permits the call",
        error.WrongThread => "the call is not permitted on this thread",
        error.NativeError => "native returned a value that the binding cannot read",
        else => @errorName(err),
    };
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

/// The status that reports `err` to native code.
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

test "statuses map both ways, keep an unknown code, and carry the diagnostic" {
    const table = .{
        .{ c.MLN_STATUS_INVALID_ARGUMENT, error.InvalidArgument },
        .{ c.MLN_STATUS_INVALID_STATE, error.InvalidState },
        .{ c.MLN_STATUS_WRONG_THREAD, error.WrongThread },
        .{ c.MLN_STATUS_UNSUPPORTED, error.Unsupported },
        .{ c.MLN_STATUS_CANCELLED, error.Cancelled },
        .{ c.MLN_STATUS_BUSY, error.Busy },
        .{ c.MLN_STATUS_TARGET_LOST, error.TargetLost },
        .{ c.MLN_STATUS_NOT_READY, error.NotReady },
        .{ c.MLN_STATUS_NOT_FOUND, error.NotFound },
        .{ c.MLN_STATUS_NATIVE_ERROR, error.NativeError },
    };
    inline for (table) |row| {
        try std.testing.expectError(row[1], errorFromRawStatus(row[0]));
        try std.testing.expectEqual(@as(c.mln_status, row[0]), rawStatus(row[1]));
    }
    // A host callback's error that has no native status reports a native error.
    try std.testing.expectEqual(@as(c.mln_status, c.MLN_STATUS_NATIVE_ERROR), rawStatus(error.OutOfMemory));

    // A status the binding does not name keeps its code and the call's message.
    var diagnostic: diagnostics.Diagnostic = .{};
    const unknown_raw_status: i32 = -9999;
    const unknown = struct {
        fn function(out_diagnostic: [*c]c.mln_diagnostic) callconv(.c) c.mln_status {
            @memcpy(out_diagnostic[0].message[0..8], "unknown\x00");
            return unknown_raw_status;
        }
    }.function;
    try std.testing.expectError(error.UnknownStatus, call(unknown, .{}, &diagnostic));
    try std.testing.expectEqual(@as(?i32, unknown_raw_status), diagnostic.raw_status);
    try std.testing.expectEqualStrings("unknown", diagnostic.message());

    record(&diagnostic, -1, "x" ** 8192);
    try std.testing.expectEqual(diagnostic.native.message.len - 1, diagnostic.message().len);
    begin(&diagnostic);
    try std.testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try std.testing.expectEqualStrings("", diagnostic.message());
    // A failure the binding detects records its own message only once, so the
    // first recorded cause wins.
    fail(&diagnostic, error.InvalidString);
    fail(&diagnostic, error.AlreadyCompleted);
    try std.testing.expectEqualStrings("the string contains a NUL byte", diagnostic.message());
}

test "ABI version validation reports mismatch diagnostics" {
    var diagnostic: diagnostics.Diagnostic = .{};
    try std.testing.expectError(error.AbiVersionMismatch, validateAbiVersionValue(1, 0, &diagnostic));
    try std.testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try std.testing.expect(std.mem.indexOf(u8, diagnostic.message(), "expected 0, got 1") != null);
}
