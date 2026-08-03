const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const objc = if (build_options.supports_metal) @import("objc") else struct {};

const c = @import("c.zig").c;
const channel = @import("channel.zig");
const diagnostics = @import("diagnostics.zig");
const maplibre = @import("maplibre_native_ffi");
const input = @import("input.zig");
const map_state = @import("map_state.zig");
const render = @import("render/mod.zig");
const types = @import("types.zig");
const viewport = @import("viewport.zig");

const RenderTarget = render.RenderTarget;

/// Backstop for the runtime loop's park. The render loop's wake source is what
/// normally releases it, so this only bounds a pump that nothing signals.
const park_timeout_milliseconds = 100;
const uses_egl = build_options.supports_opengl and (builtin.os.tag == .linux or builtin.os.tag == .macos);

const RuntimeLoopArgs = struct {
    allocator: std.mem.Allocator,
    io: std.Io,
    initial_viewport: types.Viewport,
    commands: *channel.CommandQueue,
    render_request: *channel.RenderRequest,
    map_channel: *channel.MapChannel,
};

/// Owns the runtime and the map for their whole lifetime, on a thread that is
/// not the one presenting. It never touches the render session: the render loop
/// attaches its own against the map published here.
fn runtimeLoop(args: RuntimeLoopArgs) void {
    var state = map_state.MapState.init(args.allocator, args.initial_viewport) catch |err| {
        args.map_channel.fail(err);
        return;
    };
    // However this loop exits, the render loop still owns the session, and a map
    // with an attached session cannot be destroyed. The body publishes any
    // failure before these run, so the render loop stops, closes its session,
    // and requests shutdown; only then is the map destroyed. Defers run in
    // reverse, so the wait happens first.
    defer state.deinit();
    defer args.map_channel.awaitShutdown(args.io);

    runtimeLoopBody(args, &state) catch |err| args.map_channel.fail(err);
}

fn runtimeLoopBody(args: RuntimeLoopArgs, state: *map_state.MapState) !void {
    // The render loop signals this to release the parked pump, so a camera
    // command or a shutdown request lands without waiting out the bound below.
    const wake = try state.runtime.wakeSource();
    defer wake.release();

    // Reused across drains, so applying a batch allocates nothing.
    var batch: std.ArrayList(channel.CameraCommand) = .empty;
    defer batch.deinit(args.allocator);

    args.map_channel.publish(state.map, wake);

    while (!args.map_channel.shutdownRequested() and args.map_channel.failureValue() == null) {
        try state.applyCommands(args.commands, &batch);
        // This thread has no display to pace it, so it takes its cadence from
        // the runtime's own work and parks in between. The bound is a backstop
        // for work that queues nothing on the owner thread, not the cadence.
        try state.runtime.pump(park_timeout_milliseconds);
        if (try map_state.drainEvents(args.allocator, &state.runtime, &state.map)) {
            args.render_request.set();
        }
    }
}

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

    var gpa = std.heap.DebugAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // The graphics context, the render session, and every presentation resource
    // belong to this thread, which owns the window and its display callbacks.
    var target = try RenderTarget.init(allocator, window_handle, current_viewport, target_mode);

    var commands = channel.CommandQueue.init(allocator);
    defer commands.deinit();
    var render_request = channel.RenderRequest{};
    var map_channel = channel.MapChannel{};

    const runtime_thread = try std.Thread.spawn(.{}, runtimeLoop, .{RuntimeLoopArgs{
        .allocator = allocator,
        .io = init_args.io,
        .initial_viewport = current_viewport,
        .commands = &commands,
        .render_request = &render_request,
        .map_channel = &map_channel,
    }});

    const result = renderLoop(
        init_args.io,
        window_handle,
        target_mode,
        &target,
        &current_viewport,
        &commands,
        &render_request,
        &map_channel,
    );

    // Destroy the session before the runtime loop destroys the map: a map with
    // an attached session cannot be destroyed.
    target.deinit();
    map_channel.requestShutdown();
    runtime_thread.join();

    try result;
    if (map_channel.failureValue()) |err| return err;
}

/// The display-paced render loop. Owns the window, input, and the render
/// session once it adopts it.
fn renderLoop(
    io: std.Io,
    window_handle: *c.SDL_Window,
    target_mode: types.RenderTargetMode,
    target: *RenderTarget,
    current_viewport: *types.Viewport,
    commands: *channel.CommandQueue,
    render_request: *channel.RenderRequest,
    map_channel: *channel.MapChannel,
) !void {
    // The runtime loop creates the map; this loop attaches its own session
    // against it and owns that session for the rest of the run.
    var map = while (true) {
        if (map_channel.failureValue()) |err| return err;
        if (map_channel.mapHandle()) |handle| break handle;
        try io.sleep(.fromMilliseconds(1), .awake);
    };
    try target.attach(&map, current_viewport.*);

    printStartupStatus(target_mode);
    input.logControls();

    var running = true;
    var input_controller = input.Controller{};
    while (running) {
        const pool = if (build_options.supports_metal) objc.AutoreleasePool.init() else {};
        defer if (build_options.supports_metal) pool.deinit();

        if (map_channel.failureValue()) |err| return err;

        var event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&event)) {
            switch (event.type) {
                c.SDL_EVENT_QUIT => running = false,
                c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => running = false,
                c.SDL_EVENT_WINDOW_RESIZED,
                c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
                c.SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
                => {
                    current_viewport.* = viewport.get(window_handle);
                    viewport.log("resized viewport", current_viewport.*);
                    // Every mode follows a resize without losing its session:
                    // the ones the session sizes resize in place, and a
                    // caller-owned target allocates a replacement and hands it
                    // over.
                    try target.resize(current_viewport.*);
                    // The session resize enqueued the new extent to the map's
                    // owner thread, so release its parked pump the way an
                    // input command does.
                    map_channel.wakeRuntimeLoop();
                    render_request.set();
                },
                else => {
                    const input_result = input_controller.handleEvent(
                        &event,
                        commands,
                        current_viewport.*,
                    );
                    if (input_result.handled) {
                        // Release the runtime loop's parked pump so the command
                        // just queued is applied on this frame rather than after
                        // the parking bound.
                        map_channel.wakeRuntimeLoop();
                    }
                    if (input_result.camera_changed) render_request.set();
                },
            }
        }

        try target.finishFrame();

        // Consume before rendering, so a request the runtime loop publishes
        // during the render call is not discarded.
        if (render_request.consume()) {
            if (!try target.renderUpdate(null, current_viewport.*)) {
                render_request.set();
            }
        }

        // Stand-in for a display-refresh subscription until the host loop is
        // display-paced; see the frame loop section of the example spec.
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
