const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const objc = if (build_options.supports_metal) @import("objc") else struct {};

const c = @import("c.zig").c;
const diagnostics = @import("diagnostics.zig");
const maplibre = @import("maplibre_native");
const input = @import("input.zig");
const map_state = @import("map_state.zig");
const render = @import("render/mod.zig");
const types = @import("types.zig");
const viewport = @import("viewport.zig");

const RenderTarget = render.RenderTarget;
const uses_egl = build_options.supports_opengl and (builtin.os.tag == .linux or builtin.os.tag == .macos);

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

    var map = try map_state.MapState.init(allocator, window_handle, current_viewport, target_mode);
    defer map.deinit();
    defer map.finishFrame() catch |err| {
        std.debug.print("failed to finish final frame: {s}\n", .{@errorName(err)});
    };

    printStartupStatus(target_mode);
    input.logControls();

    var running = true;
    var has_presented_frame = false;
    var render_pending = true;
    var input_controller = input.Controller{};
    while (running) {
        const pool = if (build_options.supports_metal) objc.AutoreleasePool.init() else {};
        defer if (build_options.supports_metal) pool.deinit();

        var did_work = false;
        var event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&event)) {
            did_work = true;
            switch (event.type) {
                c.SDL_EVENT_QUIT => running = false,
                c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => running = false,
                c.SDL_EVENT_WINDOW_RESIZED,
                c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
                c.SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
                => {
                    current_viewport = viewport.get(window_handle);
                    viewport.log("resized viewport", current_viewport);
                    if (map.needsReattachOnResize()) {
                        try map.resizeWithReattachedTarget(window_handle, current_viewport, target_mode);
                    } else {
                        try map.resize(current_viewport);
                    }
                    render_pending = true;
                },
                else => {
                    const input_result = try input_controller.handleEvent(
                        &event,
                        &map.map,
                        map.diagnostic_store,
                        current_viewport,
                    );
                    render_pending = render_pending or input_result.camera_changed;
                },
            }
        }

        try map.runtime.runOnce();
        const render_update_available = try map_state.drainEvents(allocator, &map.runtime, &map.map);
        render_pending = render_pending or render_update_available;
        did_work = did_work or render_update_available;

        try map.finishFrame();

        if (render_pending) {
            if (try map.renderUpdate(current_viewport)) {
                render_pending = false;
                did_work = true;
                has_presented_frame = true;
            }
        }

        if (!did_work) c.SDL_Delay(if (has_presented_frame) 8 else 1);
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
