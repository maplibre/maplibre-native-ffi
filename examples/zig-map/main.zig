const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const objc = if (build_options.supports_metal) @import("objc") else struct {};

const c = @import("c.zig").c;
const diagnostics = @import("diagnostics.zig");
const maplibre = @import("maplibre_native_ffi");
const input = @import("input.zig");
const map_state = @import("map_state.zig");
const render = @import("render/mod.zig");
const types = @import("types.zig");
const viewport = @import("viewport.zig");

const RenderTarget = render.RenderTarget;
const uses_egl = build_options.supports_opengl and
    (builtin.os.tag == .linux or builtin.os.tag == .macos);

const NotificationReceiver = struct {
    scheduled: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    wake_failed: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    event_type: u32,

    fn schedule(user_data: ?*anyopaque) callconv(.c) void {
        const self: *NotificationReceiver = @ptrCast(@alignCast(user_data.?));
        if (self.scheduled.swap(true, .acq_rel)) return;

        var event = std.mem.zeroes(c.SDL_Event);
        event.type = self.event_type;
        if (!c.SDL_PushEvent(&event)) self.wake_failed.store(true, .release);
    }
};

pub fn main(init_args: std.process.Init) !void {
    const target_mode = (try parseRenderTargetMode(init_args)) orelse return;
    try validateNativeRenderBackend();

    try maplibre.setLogCallback(.{ .handler = diagnostics.logRecord }, null);
    defer maplibre.clearLogCallback(null) catch {};

    if (uses_egl) {
        _ = c.SDL_SetHint(c.SDL_HINT_VIDEO_FORCE_EGL, "1");
    }

    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        std.debug.print("SDL_Init failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
        return types.AppError.SdlInitFailed;
    }
    defer c.SDL_Quit();

    if (uses_egl) {
        if (!c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES) or
            !c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3) or
            !c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 0))
        {
            std.debug.print("SDL_GL_SetAttribute failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
            return types.AppError.BackendSetupFailed;
        }
    }

    const window_flags = RenderTarget.window_flags |
        c.SDL_WINDOW_RESIZABLE |
        c.SDL_WINDOW_HIGH_PIXEL_DENSITY;
    const window = c.SDL_CreateWindow(
        "MapLibre SDL3 Map",
        viewport.window_width,
        viewport.window_height,
        window_flags,
    );
    if (window == null) {
        std.debug.print("SDL_CreateWindow failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
        return types.AppError.WindowCreateFailed;
    }
    defer c.SDL_DestroyWindow(window);

    const window_handle = window.?;
    _ = c.SDL_RaiseWindow(window_handle);
    var current_viewport = viewport.get(window_handle);
    viewport.log("initial viewport", current_viewport);

    const notification_event_type = c.SDL_RegisterEvents(1);
    if (notification_event_type == 0) return types.AppError.EventDrainFailed;
    var notification_receiver = NotificationReceiver{
        .event_type = notification_event_type,
    };

    var gpa = std.heap.DebugAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var state = try map_state.MapState.init(allocator, current_viewport);
    defer state.deinit();
    try state.runtime.setNotificationCallback(
        NotificationReceiver.schedule,
        &notification_receiver,
    );
    defer state.runtime.clearNotificationCallback() catch {};

    // The graphics context, render session, and presentation resources remain
    // on the window-owning thread.
    var target = try RenderTarget.init(allocator, window_handle, current_viewport, target_mode);
    defer target.deinit();
    try target.attach(&state.map, current_viewport);

    try renderLoop(
        init_args.io,
        window_handle,
        target_mode,
        &target,
        &current_viewport,
        &state,
        &notification_receiver,
    );
}

/// The display-paced loop. Runtime/map calls are any-thread; the render session
/// remains attached to this graphics thread.
fn renderLoop(
    io: std.Io,
    window_handle: *c.SDL_Window,
    target_mode: types.RenderTargetMode,
    target: *RenderTarget,
    current_viewport: *types.Viewport,
    state: *map_state.MapState,
    notification_receiver: *NotificationReceiver,
) !void {
    printStartupStatus(target_mode);
    input.logControls();

    var running = true;
    var render_requested = true;
    var input_controller = input.Controller{};
    while (running) {
        const pool = if (build_options.supports_metal) objc.AutoreleasePool.init() else {};
        defer if (build_options.supports_metal) pool.deinit();

        if (notification_receiver.wake_failed.swap(false, .acq_rel)) {
            notification_receiver.scheduled.store(false, .release);
            if (try map_state.drainNotifications(state)) render_requested = true;
        }

        var event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&event)) {
            if (event.type == notification_receiver.event_type) {
                notification_receiver.scheduled.store(false, .release);
                if (try map_state.drainNotifications(state)) render_requested = true;
                continue;
            }
            switch (event.type) {
                c.SDL_EVENT_QUIT => running = false,
                c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => running = false,
                c.SDL_EVENT_WINDOW_RESIZED,
                c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
                c.SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
                => {
                    current_viewport.* = viewport.get(window_handle);
                    viewport.log("resized viewport", current_viewport.*);
                    try target.resize(current_viewport.*);
                    _ = try state.map.resize(
                        current_viewport.logical_width,
                        current_viewport.logical_height,
                        current_viewport.scale_factor,
                    );
                    render_requested = true;
                },
                else => {
                    const input_result = try input_controller.handleEvent(
                        &event,
                        state,
                        current_viewport.*,
                    );
                    if (input_result.camera_changed) render_requested = true;
                },
            }
        }

        try target.finishFrame();

        if (render_requested) {
            render_requested = false;
            if (!try target.renderUpdate(null, current_viewport.*)) {
                render_requested = true;
            }
        }

        // Stand-in for a display-refresh subscription.
        try io.sleep(.fromMilliseconds(8), .awake);
    }
}

fn validateNativeRenderBackend() !void {
    const support = maplibre.supportedRenderBackends();
    var support_label_buffer: [32]u8 = undefined;
    std.debug.print("native render backends: {s}\n", .{
        renderBackendSupportLabel(&support_label_buffer, support),
    });
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_opengl and !support.opengl) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn printStartupStatus(target_mode: types.RenderTargetMode) void {
    std.debug.print("render target: {s}\n", .{target_mode.label()});
    std.debug.print("render target status: {s}\n", .{target_mode.statusLine()});
}

fn renderBackendSupportLabel(buffer: []u8, support: maplibre.RenderBackendSupport) []const u8 {
    var len: usize = 0;
    var has_backend = false;
    if (support.metal) appendBackendLabel(buffer, &len, &has_backend, "metal");
    if (support.opengl) appendBackendLabel(buffer, &len, &has_backend, "opengl");
    if (support.vulkan) appendBackendLabel(buffer, &len, &has_backend, "vulkan");
    if (!has_backend) return "none";
    return buffer[0..len];
}

fn appendBackendLabel(buffer: []u8, len: *usize, has_backend: *bool, label: []const u8) void {
    if (has_backend.*) {
        buffer[len.*] = ',';
        len.* += 1;
    }
    @memcpy(buffer[len.*..][0..label.len], label);
    len.* += label.len;
    has_backend.* = true;
}

fn parseRenderTargetMode(init_args: std.process.Init) !?types.RenderTargetMode {
    var args = try std.process.Args.Iterator.initAllocator(init_args.minimal.args, init_args.gpa);
    defer args.deinit();
    _ = args.skip();

    const mode_arg = args.next() orelse {
        printUsage();
        std.process.exit(1);
    };
    if (std.mem.eql(u8, mode_arg, "--help")) {
        printUsage();
        return null;
    }
    if (std.mem.startsWith(u8, mode_arg, "-")) {
        printUsage();
        std.process.exit(1);
    }
    const mode = types.RenderTargetMode.parse(mode_arg) orelse {
        printUsage();
        std.process.exit(1);
    };
    while (args.next()) |_| {
        printUsage();
        std.process.exit(1);
    }
    return mode;
}

fn printUsage() void {
    std.debug.print(
        \\Usage: zig-map <mode>
        \\
        \\Modes:
        \\  owned-texture     session-owned texture render target
        \\  borrowed-texture  caller-owned texture render target
        \\  native-surface    native surface render target
        \\
    , .{});
}
