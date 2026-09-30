//! The shared fixture of the Zig binding suite: one runtime and map per test
//! with guaranteed teardown, a resource provider that denies every request, and
//! waits that block on the runtime's and render session's wakes. GPU contexts
//! come from tests/graphics.

const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const test_graphics = @import("mln_test_graphics");

pub const style_json =
    \\{"version":8,"name":"zig-binding-test","sources":{},
    \\"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}
;

/// The RGBA8 value of the style's background color.
pub const background_rgba = [4]u8{ 0xd8, 0xf1, 0xff, 0xff };

/// A deadline for one wait: ten seconds, scaled by MLN_TEST_TIMEOUT_SCALE on
/// slow runners.
pub const Deadline = struct {
    at: std.Io.Clock.Timestamp,

    pub fn start() Deadline {
        const scale: i64 = if (std.c.getenv("MLN_TEST_TIMEOUT_SCALE")) |value|
            std.fmt.parseInt(i64, std.mem.span(value), 10) catch 1
        else
            1;
        return .{ .at = std.Io.Clock.Timestamp.now(testing.io, .awake).addDuration(.{
            .raw = .fromSeconds(10 * scale),
            .clock = .awake,
        }) };
    }

    pub fn expired(self: Deadline) bool {
        return std.Io.Clock.Timestamp.now(testing.io, .awake).raw.nanoseconds >= self.at.raw.nanoseconds;
    }
};

/// A count that native callbacks raise from any thread and a test thread
/// blocks on. It serves as the `Wake` of a runtime or render session.
pub const Signal = struct {
    count: std.atomic.Value(u32) = .init(0),

    pub fn notify(self: *Signal) void {
        _ = self.count.fetchAdd(1, .release);
        testing.io.futexWake(u32, &self.count.raw, std.math.maxInt(u32));
    }

    pub fn wake(self: *Signal) maplibre.Wake {
        return .{ .context = self, .callback = onWake };
    }

    fn onWake(context: ?*anyopaque) maplibre.Error!void {
        const self: *Signal = @ptrCast(@alignCast(context.?));
        self.notify();
    }

    /// Returns once `ready(context)` holds, checking it again after each
    /// notification. A condition that native code settles without notifying
    /// is checked again at least every 20 milliseconds, like the C harness.
    pub fn waitUntil(self: *Signal, context: anytype, comptime ready: fn (@TypeOf(context)) anyerror!bool) !void {
        const deadline = Deadline.start();
        while (true) {
            const seen = self.count.load(.acquire);
            if (try ready(context)) return;
            if (deadline.expired()) return error.WaitTimedOut;
            const recheck = std.Io.Clock.Timestamp.now(testing.io, .awake).addDuration(.{
                .raw = .fromMilliseconds(20),
                .clock = .awake,
            });
            const until = if (recheck.raw.nanoseconds < deadline.at.raw.nanoseconds) recheck else deadline.at;
            try testing.io.futexWaitTimeout(u32, &self.count.raw, seen, .{ .deadline = until });
        }
    }
};

/// Waits for a flag that another thread sets through `set`.
pub const Flag = struct {
    signal: Signal = .{},
    value: std.atomic.Value(bool) = .init(false),

    pub fn set(self: *Flag) void {
        self.value.store(true, .release);
        self.signal.notify();
    }

    pub fn isSet(self: *Flag) bool {
        return self.value.load(.acquire);
    }

    pub fn wait(self: *Flag) !void {
        try self.signal.waitUntil(self, struct {
            fn ready(flag: *Flag) anyerror!bool {
                return flag.isSet();
            }
        }.ready);
    }
};

/// A count that another thread raises through `add`, with a wait for a target.
pub const Counter = struct {
    signal: Signal = .{},
    value: std.atomic.Value(usize) = .init(0),

    pub fn add(self: *Counter) void {
        _ = self.value.fetchAdd(1, .acq_rel);
        self.signal.notify();
    }

    pub fn get(self: *Counter) usize {
        return self.value.load(.acquire);
    }

    pub fn waitFor(self: *Counter, target: usize) !void {
        const Target = struct { counter: *Counter, target: usize };
        try self.signal.waitUntil(Target{ .counter = self, .target = target }, struct {
            fn ready(context: Target) anyerror!bool {
                return context.counter.get() >= context.target;
            }
        }.ready);
    }
};

/// Waits for a future's terminal value and releases the future.
pub fn resolve(future_value: anytype) !@TypeOf(future_value).Value {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

/// Waits for an accepted command and asserts that it committed.
pub fn expectCommitted(future_value: maplibre.Future(maplibre.CommandCompletion)) !void {
    const completion = try resolve(future_value);
    try testing.expectEqual(maplibre.CommandDisposition.committed, completion.disposition);
}

/// Answers every request with a not-found error, so no test reaches the
/// network by accident.
fn denyResource(_: ?*anyopaque, _: maplibre.ResourceRequest, request: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
    try maplibre.resourceRequestComplete(std.heap.smp_allocator, request, .{
        .status = .@"error",
        .error_reason = .not_found,
        .error_message = "the Zig test fixture serves no resources",
    }, null);
    try maplibre.resourceRequestRelease(request);
    return .handle;
}

pub const denying_provider = maplibre.ResourceProvider{ .callback = denyResource };

pub const FixtureOptions = struct {
    asset_path: ?[]const u8 = null,
    extent: maplibre.LogicalExtent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
    map_mode: maplibre.MapMode = .continuous,
};

/// One runtime and one map. The runtime's event wake drives `waitForEvent`.
/// `destroy` closes whatever the test left open, so every path tears down.
pub const Fixture = struct {
    events: Signal = .{},
    runtime: maplibre.Runtime,
    map: maplibre.Map,
    map_open: bool = true,
    runtime_open: bool = true,

    pub fn create(options: FixtureOptions) !*Fixture {
        const self = try testing.allocator.create(Fixture);
        errdefer testing.allocator.destroy(self);
        self.* = .{ .runtime = undefined, .map = undefined };
        var runtime_options = try maplibre.runtimeOptionsDefault(testing.allocator);
        defer runtime_options.deinit();
        runtime_options.value.asset_path = options.asset_path;
        runtime_options.value.event_wake = self.events.wake();
        self.runtime = try maplibre.runtimeCreate(testing.allocator, runtime_options.value, null);
        errdefer closeRuntime(self.runtime) catch self.runtime.deinit();
        try self.setProvider(denying_provider);
        var map_options = try maplibre.mapOptionsDefault();
        map_options.initial_extent = options.extent;
        map_options.map_mode = options.map_mode;
        self.map = try resolve(try maplibre.mapCreate(testing.allocator, self.runtime, map_options, null));
        return self;
    }

    /// Closes the map and runtime if the test left them open, and frees the
    /// fixture. A close that fails logs an error, which fails the test, and
    /// falls back to disposal so the next test starts clean.
    pub fn destroy(self: *Fixture) void {
        if (self.map_open) self.closeMap() catch |err| {
            std.log.err("fixture map close failed: {s}", .{@errorName(err)});
            self.map.deinit();
        };
        if (self.runtime_open) self.closeRuntime_() catch |err| {
            std.log.err("fixture runtime close failed: {s}", .{@errorName(err)});
            self.runtime.deinit();
        };
        testing.allocator.destroy(self);
    }

    pub fn closeMap(self: *Fixture) !void {
        try closeMapHandle(self.map);
        self.map_open = false;
    }

    fn closeRuntime_(self: *Fixture) !void {
        try closeRuntime(self.runtime);
        self.runtime_open = false;
    }

    /// Releases the runtime once native has retired every child that the test
    /// disposed. A release is refused while any child is live or pending, so
    /// the wait retries the release until one is accepted.
    pub fn releaseRuntimeWhenChildless(self: *Fixture) !void {
        try self.events.waitUntil(self, struct {
            fn ready(fixture: *Fixture) anyerror!bool {
                var teardown = maplibre.runtimeRelease(fixture.runtime, null) catch |err| switch (err) {
                    error.InvalidState => return false,
                    else => return err,
                };
                defer teardown.deinit();
                try teardown.wait(null);
                return true;
            }
        }.ready);
        self.runtime_open = false;
    }

    pub fn barrier(self: *Fixture) !void {
        try resolve(try maplibre.runtimeBarrier(self.runtime, null));
    }

    pub fn setProvider(self: *Fixture, provider: maplibre.ResourceProvider) !void {
        try resolve(try maplibre.runtimeSetResourceProvider(testing.allocator, self.runtime, provider, null));
    }

    pub fn loadStyle(self: *Fixture) !void {
        try expectCommitted(try maplibre.mapSetStyleJson(self.map, style_json, null));
        var loaded = try self.waitForEvent(.map_style_loaded);
        loaded.deinit();
    }

    /// Drains until an event of `event_type` arrives and returns a copy of it,
    /// blocking on the runtime's event wake between drains.
    pub fn waitForEvent(self: *Fixture, event_type: maplibre.RuntimeEventType) !maplibre.OwnedValue(maplibre.RuntimeEvent) {
        const deadline = Deadline.start();
        while (true) {
            const seen = self.events.count.load(.acquire);
            var batch = try maplibre.runtimeDrainEvents(self.runtime, null);
            defer batch.deinit();
            var copy = try maplibre.eventBatchGet(testing.allocator, batch, null);
            for (copy.value.events) |event| {
                if (event.type == event_type) return .{ .arena = copy.arena, .value = event };
            }
            copy.deinit();
            if (deadline.expired()) return error.EventNotObserved;
            try testing.io.futexWaitTimeout(u32, &self.events.count.raw, seen, .{ .deadline = deadline.at });
        }
    }
};

pub fn closeRuntime(runtime: maplibre.Runtime) !void {
    try resolve(try maplibre.runtimeRelease(runtime, null));
}

pub fn closeMapHandle(map: maplibre.Map) !void {
    try resolve(try maplibre.mapRelease(map, null));
}

// Rendering. Every native backend but WebGL and WebGPU takes its context from
// tests/graphics, loaded as the mln_test_graphics shared library.

pub const supports_egl = build_options.supports_opengl and builtin.os.tag != .windows;

const graphics_backend: u32 = if (build_options.supports_metal)
    test_graphics.MLN_TEST_GRAPHICS_BACKEND_METAL
else if (build_options.supports_vulkan)
    test_graphics.MLN_TEST_GRAPHICS_BACKEND_VULKAN
else if (supports_egl)
    test_graphics.MLN_TEST_GRAPHICS_BACKEND_EGL
else
    test_graphics.MLN_TEST_GRAPHICS_BACKEND_WGL;

/// A device or context from tests/graphics for this build's backend.
pub const Graphics = struct {
    handle: *test_graphics.mln_test_graphics,
    context: test_graphics.mln_test_graphics_context,

    pub fn create() !Graphics {
        const handle = test_graphics.mln_test_graphics_create(graphics_backend) orelse return graphicsError();
        errdefer test_graphics.mln_test_graphics_destroy(handle);
        var context: test_graphics.mln_test_graphics_context = undefined;
        if (!test_graphics.mln_test_graphics_get_context(handle, &context)) return graphicsError();
        return .{ .handle = handle, .context = context };
    }

    pub fn destroy(self: *Graphics) void {
        test_graphics.mln_test_graphics_destroy(self.handle);
    }

    pub fn makeCurrent(self: *Graphics) !void {
        if (!test_graphics.mln_test_graphics_make_current(self.handle)) return graphicsError();
    }

    fn graphicsError() error{GraphicsUnavailable} {
        std.log.err("tests/graphics: {s}", .{std.mem.span(test_graphics.mln_test_graphics_last_error())});
        return error.GraphicsUnavailable;
    }

    fn openglContext(self: *const Graphics) maplibre.OpenglContextDescriptor {
        if (supports_egl) return .{ .data = .{ .egl = .{
            .display = self.context.egl_display,
            .config = self.context.egl_config,
            .share_context = self.context.egl_context,
        } } };
        return .{ .data = .{ .wgl = .{
            .device_context = self.context.wgl_device_context,
            .share_context = self.context.wgl_context,
            .get_proc_address = self.context.get_proc_address,
        } } };
    }

    fn vulkanContext(self: *const Graphics) maplibre.VulkanContextDescriptor {
        return .{
            .instance = self.context.vulkan_instance,
            .physical_device = self.context.vulkan_physical_device,
            .device = self.context.vulkan_device,
            .graphics_queue = self.context.vulkan_queue,
            .graphics_queue_family_index = self.context.vulkan_queue_family_index,
            .get_instance_proc_addr = self.context.vulkan_get_instance_proc_addr,
            .get_device_proc_addr = self.context.vulkan_get_device_proc_addr,
        };
    }
};

/// The frame view type that this build's backend hands to a scoped texture
/// getter.
pub const OwnedTextureFrame = if (build_options.supports_metal)
    maplibre.MetalOwnedTextureFrame
else if (build_options.supports_vulkan)
    maplibre.VulkanOwnedTextureFrame
else
    maplibre.OpenglOwnedTextureFrame;

/// Reads a scoped view of an acquired frame's texture on this build's backend.
pub fn getOwnedTexture(comptime Result: type, frame: maplibre.AcquiredFrame, context: anytype, comptime use: *const fn (@TypeOf(context), OwnedTextureFrame) anyerror!Result) anyerror!Result {
    if (build_options.supports_metal) return maplibre.acquiredFrameGetMetalTexture(Result, frame, context, use, null);
    if (build_options.supports_vulkan) return maplibre.acquiredFrameGetVulkanTexture(Result, frame, context, use, null);
    return maplibre.acquiredFrameGetOpenglTexture(Result, frame, context, use, null);
}

/// An owned-texture render session on a fixture's map, with the wakes that
/// its waits block on. OpenGL sessions share the fixture context, which is
/// current on the thread that attaches, so they take the caller driver.
pub const OwnedTexture = struct {
    wakes: Signal = .{},
    graphics: Graphics,
    session: maplibre.RenderSession,
    caller_driven: bool,
    attached: bool = true,
    next_token: u64 = 1,

    pub const default_driver: maplibre.RenderDriverKind = if (build_options.supports_opengl) .caller_graphics_thread else .core_worker;

    pub fn attach(map: maplibre.Map, extent: maplibre.RenderTargetExtent, driver: maplibre.RenderDriverKind) !*OwnedTexture {
        const self = try testing.allocator.create(OwnedTexture);
        errdefer testing.allocator.destroy(self);
        self.* = .{ .graphics = try Graphics.create(), .session = undefined, .caller_driven = driver == .caller_graphics_thread };
        errdefer self.graphics.destroy();
        if (build_options.supports_opengl and self.caller_driven) try self.graphics.makeCurrent();
        const options = maplibre.RenderSessionAttachOptions{
            .driver = driver,
            .requested_texture_ring_depth = 2,
            .frame_wake = self.wakes.wake(),
            .driver_work_wake = self.wakes.wake(),
        };
        const attachment = if (build_options.supports_metal)
            try maplibre.metalOwnedTextureAttach(testing.allocator, map, .{ .extent = extent, .context = .{ .device = self.graphics.context.metal_device } }, options, null)
        else if (build_options.supports_vulkan)
            try maplibre.vulkanOwnedTextureAttach(testing.allocator, map, .{ .extent = extent, .context = self.graphics.vulkanContext() }, options, null)
        else
            try maplibre.openglOwnedTextureAttach(testing.allocator, map, .{ .extent = extent, .context = self.graphics.openglContext() }, options, null);
        self.session = attachment.session;
        errdefer {
            _ = maplibre.renderSessionAbandon(self.session, null) catch {};
            maplibre.renderSessionDestroy(self.session, null) catch {};
        }
        try self.finish(attachment.ready);
        return self;
    }

    /// Detaches and destroys the session unless the test already did, then
    /// releases the graphics context. A failed detach logs an error, which
    /// fails the test, and abandons the session instead.
    pub fn close(self: *OwnedTexture) void {
        if (self.attached) {
            self.detach() catch |err| {
                std.log.err("render session detach failed: {s}", .{@errorName(err)});
                _ = maplibre.renderSessionAbandon(self.session, null) catch {};
            };
            maplibre.renderSessionDestroy(self.session, null) catch |err| {
                std.log.err("render session destroy failed: {s}", .{@errorName(err)});
                self.session.deinit();
            };
        }
        self.graphics.destroy();
        testing.allocator.destroy(self);
    }

    pub fn detach(self: *OwnedTexture) !void {
        try self.finish(try maplibre.renderSessionDetach(self.session, null));
        self.attached = false;
    }

    /// Waits for a session future, servicing a caller driver's work between
    /// checks.
    pub fn resolve(self: *OwnedTexture, future_value: anytype) !@TypeOf(future_value).Value {
        var future = future_value;
        defer future.deinit();
        const Context = struct { owner: *OwnedTexture, future: *@TypeOf(future) };
        try self.wakes.waitUntil(Context{ .owner = self, .future = &future }, struct {
            fn ready(context: Context) anyerror!bool {
                try context.owner.service();
                return context.future.poll();
            }
        }.ready);
        return future.wait(null);
    }

    fn finish(self: *OwnedTexture, future: maplibre.Future(void)) !void {
        try self.resolve(future);
    }

    fn service(self: *OwnedTexture) !void {
        if (self.caller_driven) _ = try maplibre.renderSessionServiceDriverWork(self.session, 0, null);
    }

    /// Requests one frame and returns its result, blocking on the session's
    /// frame wake.
    pub fn renderFrame(self: *OwnedTexture) !maplibre.RenderFrameResult {
        const token = self.next_token;
        self.next_token += 1;
        try maplibre.renderSessionRequestFrame(testing.allocator, self.session, .{ .token = token }, null);
        const Context = struct { owner: *OwnedTexture, token: u64, result: ?maplibre.RenderFrameResult = null };
        var context = Context{ .owner = self, .token = token };
        try self.wakes.waitUntil(&context, struct {
            fn ready(ctx: *Context) anyerror!bool {
                try ctx.owner.service();
                var batch = maplibre.renderSessionDrainFrameResults(ctx.owner.session, null) catch |err| switch (err) {
                    error.NotReady => return false,
                    else => return err,
                };
                defer batch.deinit();
                for (0..try maplibre.renderFrameBatchCount(batch, null)) |index| {
                    const result = try maplibre.renderFrameBatchGet(batch, index, null);
                    if (result.token == ctx.token) ctx.result = result;
                }
                return ctx.result != null;
            }
        }.ready);
        return context.result.?;
    }

    /// Renders until a frame reports `.rendered`, passing over the pending
    /// dispositions that precede a map's first update.
    pub fn renderUntilRendered(self: *OwnedTexture) !maplibre.RenderFrameResult {
        const deadline = Deadline.start();
        while (!deadline.expired()) {
            const result = try self.renderFrame();
            switch (result.disposition) {
                .rendered => return result,
                .no_update, .size_pending, .target_not_ready => {},
                else => return error.UnexpectedFrameDisposition,
            }
        }
        return error.FrameDidNotRender;
    }
};
