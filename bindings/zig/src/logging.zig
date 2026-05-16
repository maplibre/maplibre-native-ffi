const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");

pub const LogSeverity = union(enum) {
    info,
    warning,
    @"error",
    unknown: u32,

    fn fromRaw(raw: u32) LogSeverity {
        return switch (raw) {
            c.MLN_LOG_SEVERITY_INFO => .info,
            c.MLN_LOG_SEVERITY_WARNING => .warning,
            c.MLN_LOG_SEVERITY_ERROR => .@"error",
            else => .{ .unknown = raw },
        };
    }
};

pub const LogEvent = union(enum) {
    general,
    setup,
    shader,
    parse_style,
    parse_tile,
    render,
    style,
    database,
    http_request,
    sprite,
    image,
    opengl,
    jni,
    android,
    crash,
    glyph,
    timing,
    unknown: u32,

    fn fromRaw(raw: u32) LogEvent {
        return switch (raw) {
            c.MLN_LOG_EVENT_GENERAL => .general,
            c.MLN_LOG_EVENT_SETUP => .setup,
            c.MLN_LOG_EVENT_SHADER => .shader,
            c.MLN_LOG_EVENT_PARSE_STYLE => .parse_style,
            c.MLN_LOG_EVENT_PARSE_TILE => .parse_tile,
            c.MLN_LOG_EVENT_RENDER => .render,
            c.MLN_LOG_EVENT_STYLE => .style,
            c.MLN_LOG_EVENT_DATABASE => .database,
            c.MLN_LOG_EVENT_HTTP_REQUEST => .http_request,
            c.MLN_LOG_EVENT_SPRITE => .sprite,
            c.MLN_LOG_EVENT_IMAGE => .image,
            c.MLN_LOG_EVENT_OPENGL => .opengl,
            c.MLN_LOG_EVENT_JNI => .jni,
            c.MLN_LOG_EVENT_ANDROID => .android,
            c.MLN_LOG_EVENT_CRASH => .crash,
            c.MLN_LOG_EVENT_GLYPH => .glyph,
            c.MLN_LOG_EVENT_TIMING => .timing,
            else => .{ .unknown = raw },
        };
    }
};

pub const LogSeverityMask = struct {
    info: bool = false,
    warning: bool = false,
    @"error": bool = false,

    pub const default = LogSeverityMask{ .info = true, .warning = true };
    pub const all = LogSeverityMask{ .info = true, .warning = true, .@"error" = true };
    pub const none = LogSeverityMask{};

    fn toRaw(self: LogSeverityMask) u32 {
        var raw: u32 = 0;
        if (self.info) raw |= c.MLN_LOG_SEVERITY_MASK_INFO;
        if (self.warning) raw |= c.MLN_LOG_SEVERITY_MASK_WARNING;
        if (self.@"error") raw |= c.MLN_LOG_SEVERITY_MASK_ERROR;
        return raw;
    }
};

pub const LogRecord = struct {
    severity: LogSeverity,
    event: LogEvent,
    code: i64,
    message: []const u8,
};

pub const LogHandler = *const fn (context: ?*anyopaque, record: LogRecord) bool;

pub const LogCallback = struct {
    handler: LogHandler,
    context: ?*anyopaque = null,
};

const LogCallbackState = struct {
    handler: LogHandler,
    context: ?*anyopaque,
};

var log_callback_state: std.atomic.Value(usize) = std.atomic.Value(usize).init(0);

pub fn setLogCallback(callback: LogCallback, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!void {
    const replacement = try std.heap.smp_allocator.create(LogCallbackState);
    replacement.* = .{ .handler = callback.handler, .context = callback.context };
    errdefer std.heap.smp_allocator.destroy(replacement);

    try status.checkStatus(c.mln_log_set_callback(logTrampoline, replacement), diagnostic_store);
    const previous = log_callback_state.swap(@intFromPtr(replacement), .seq_cst);
    if (previous != 0) {
        const previous_state: *LogCallbackState = @ptrFromInt(previous);
        std.heap.smp_allocator.destroy(previous_state);
    }
}

pub fn clearLogCallback(diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!void {
    try status.checkStatus(c.mln_log_clear_callback(), diagnostic_store);
    const previous = log_callback_state.swap(0, .seq_cst);
    if (previous != 0) {
        const previous_state: *LogCallbackState = @ptrFromInt(previous);
        std.heap.smp_allocator.destroy(previous_state);
    }
}

pub fn setAsyncLogSeverityMask(mask: LogSeverityMask, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!void {
    try status.checkStatus(c.mln_log_set_async_severity_mask(mask.toRaw()), diagnostic_store);
}

fn logTrampoline(user_data: ?*anyopaque, severity: u32, event: u32, code: i64, message: [*c]const u8) callconv(.c) u32 {
    const callback_state: *LogCallbackState = @ptrCast(@alignCast(user_data orelse return 0));
    const copied_message = std.heap.smp_allocator.dupe(u8, if (message == null) "" else std.mem.span(message)) catch return 0;
    defer std.heap.smp_allocator.free(copied_message);

    return if (callback_state.handler(callback_state.context, .{
        .severity = LogSeverity.fromRaw(severity),
        .event = LogEvent.fromRaw(event),
        .code = code,
        .message = copied_message,
    })) 1 else 0;
}

test "log raw domains preserve unknown values" {
    try std.testing.expect(std.meta.eql(LogSeverity.fromRaw(0xbeef), LogSeverity{ .unknown = 0xbeef }));
    try std.testing.expect(std.meta.eql(LogEvent.fromRaw(0xfeed), LogEvent{ .unknown = 0xfeed }));
}

test "raw log severity masks reject unknown bits" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    try std.testing.expectError(
        error.InvalidArgument,
        status.checkStatus(c.mln_log_set_async_severity_mask(c.MLN_LOG_SEVERITY_MASK_ALL << 1), &store),
    );
    const diagnostic = store.get().?;
    try std.testing.expectEqual(@as(?i32, c.MLN_STATUS_INVALID_ARGUMENT), diagnostic.raw_status);
    try std.testing.expect(diagnostic.message.len > 0);
}
