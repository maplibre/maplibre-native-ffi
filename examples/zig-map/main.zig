const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const objc = if (build_options.supports_metal) @import("objc") else struct {};

const c = @import("c.zig").c;
const diagnostics = @import("diagnostics.zig");
const events = @import("events.zig");
const maplibre = @import("maplibre_native_ffi");
const input = @import("input.zig");
const map_state = @import("map_state.zig");
const render = @import("render/mod.zig");
const render_target = @import("render_target.zig");
const types = @import("types.zig");
const viewport = @import("viewport.zig");

const RenderTarget = render.RenderTarget;
const uses_egl = build_options.supports_opengl and
    (builtin.os.tag == .linux or builtin.os.tag == .macos);

/// Whether this run is a smoke test, which `MLN_EXAMPLE_SMOKE=1` selects: the
/// example opens a hidden window, loads an inline style instead of fetching
/// one, and exits after its first rendered frame.
fn isSmokeRun(init_args: std.process.Init) bool {
    const value = init_args.environ_map.get("MLN_EXAMPLE_SMOKE") orelse return false;
    return std.mem.eql(u8, value, "1");
}

/// How long a smoke run waits for its first rendered frame.
const smoke_timeout_ms = 60_000;

/// How long the loop waits before it retries a frame that did not reach the
/// window, about one display refresh. No map-update event prompts that retry.
const frame_retry_ms = 16;

/// How long the render loop waits before SDL checks for a quit signal. SDL
/// turns SIGINT and SIGTERM into a quit event only when it next pumps events.
const signal_check_ms = 250;

pub fn main(init_args: std.process.Init) !void {
    const target_mode = (try parseRenderTargetMode(init_args)) orelse return;
    const smoke = isSmokeRun(init_args);
    try validateNativeRenderBackend();

    try maplibre.logSetCallback(std.heap.smp_allocator, .{ .callback = diagnostics.logRecord }, null);
    defer maplibre.logClearCallback(null) catch {};

    if (uses_egl) {
        _ = c.SDL_SetHint(c.SDL_HINT_VIDEO_FORCE_EGL, "1");
    }

    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        std.debug.print("SDL_Init failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
        return types.AppError.SdlInitFailed;
    }
    defer c.SDL_Quit();
    try events.init();

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
        c.SDL_WINDOW_HIGH_PIXEL_DENSITY |
        (if (smoke) c.SDL_WINDOW_HIDDEN else 0);
    const window = c.SDL_CreateWindow(
        "MapLibre SDL3 Map",
        viewport.window_width,
        viewport.window_height,
        window_flags,
    ) orelse {
        std.debug.print("SDL_CreateWindow failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
        return types.AppError.WindowCreateFailed;
    };
    defer c.SDL_DestroyWindow(window);
    if (!smoke) _ = c.SDL_RaiseWindow(window);

    var app = App{ .window = window, .viewport = viewport.get(window), .smoke = smoke };
    viewport.log("initial viewport", app.viewport);

    var gpa = std.heap.DebugAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // The graphics context, render session, and presentation resources remain
    // on the window-owning thread.
    app.target = try RenderTarget.init(allocator, window, app.viewport, target_mode);
    defer app.target.deinit();
    app.map = try map_state.MapState.init(allocator, app.viewport, smoke);
    defer app.map.deinit();
    try app.target.attach(&app.map.map, app.viewport);
    // The session detaches before its map and runtime close, and the graphics
    // resources go after them.
    defer app.target.detach();

    printStartupStatus(target_mode, app.target.session().driver);
    input.logControls();
    try app.run();
}

/// The render loop only reacts to SDL events. Input submits camera commands,
/// and native wakes post app events: a runtime event drain demands a frame for
/// each map update, and a frame-result drain shows what the session rendered.
/// A caller-driver session also gets driver wakes, which service it.
const App = struct {
    window: *c.SDL_Window,
    viewport: types.Viewport,
    smoke: bool,
    target: RenderTarget = undefined,
    map: map_state.MapState = undefined,
    input: input.Controller = .{},
    running: bool = true,

    fn run(self: *App) !void {
        if (self.smoke) events.pushAfter(.smoke_timeout, smoke_timeout_ms);
        // Updates the map published before attachment have no event left to
        // demand their frame.
        _ = try self.target.session().requestFrame(false);
        while (self.running) {
            var event: c.SDL_Event = undefined;
            if (!c.SDL_WaitEventTimeout(&event, signal_check_ms)) continue;
            const pool = if (build_options.supports_metal) objc.AutoreleasePool.init() else {};
            defer if (build_options.supports_metal) pool.deinit();
            try self.handleEvent(&event);
        }
    }

    fn handleEvent(self: *App, event: *const c.SDL_Event) !void {
        if (events.codeOf(event)) |code| return self.handleAppEvent(code);
        switch (event.type) {
            c.SDL_EVENT_QUIT, c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => self.running = false,
            c.SDL_EVENT_WINDOW_RESIZED,
            c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
            c.SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
            => {
                // One window change raises several of these events.
                const resized = viewport.get(self.window);
                if (resized.eql(self.viewport)) return;
                self.viewport = resized;
                viewport.log("resized viewport", resized);
                // The session resize carries the new logical extent to the
                // map, and a later resize supersedes an earlier one that has
                // not applied yet.
                try self.target.resize(resized);
            },
            else => try self.input.handleEvent(event, &self.map, self.viewport),
        }
    }

    fn handleAppEvent(self: *App, code: events.Code) !void {
        const session = self.target.session();
        switch (code) {
            .runtime_events => if (try self.map.drainEvents()) {
                _ = try session.requestFrame(false);
            },
            .driver_work => try session.service(),
            .target_replaced => try self.target.showReplacements(),
            .frame_results => try self.showFrameResults(),
            .retry_frame => _ = try session.requestFrame(true),
            .smoke_timeout => {
                std.debug.print("smoke: no frame rendered within 60 s\n", .{});
                return types.AppError.SmokeFrameTimedOut;
            },
        }
    }

    fn showFrameResults(self: *App) !void {
        const session = self.target.session();
        const results = try session.drainResults();
        const presented = results.rendered and try self.target.present(self.viewport);
        if (presented and self.smoke) {
            std.debug.print("smoke: rendered a frame\n", .{});
            self.running = false;
            return;
        }
        if (results.target_not_ready or (results.rendered and !presented)) {
            // Neither a target that was not ready nor a frame that missed the
            // window causes a map-update event, so the retry waits about one
            // display refresh. It forces the frame, because a frame that
            // missed the window consumed its update.
            events.pushAfter(.retry_frame, frame_retry_ms);
        } else if (results.needs_repaint) {
            _ = try session.requestFrame(false);
        }
        if (results.any) try session.compositorDone();
    }
};

fn validateNativeRenderBackend() !void {
    const support = try maplibre.supportedRenderBackendMask();
    var support_label_buffer: [32]u8 = undefined;
    std.debug.print("native render backends: {s}\n", .{
        renderBackendSupportLabel(&support_label_buffer, support),
    });
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_opengl and !support.opengl) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn printStartupStatus(target_mode: types.RenderTargetMode, driver: maplibre.RenderDriverKind) void {
    std.debug.print("render target: {s}\n", .{target_mode.label()});
    std.debug.print("render target status: {s}\n", .{target_mode.statusLine()});
    std.debug.print("render driver: {s}\n", .{render_target.driverLabel(driver)});
}

fn renderBackendSupportLabel(buffer: []u8, support: maplibre.RenderBackendFlag) []const u8 {
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
