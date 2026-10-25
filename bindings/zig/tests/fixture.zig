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
        return .init(self, onWake, null);
    }

    fn onWake(self: *Signal) maplibre.Error!void {
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

/// Records the calls of a camera end handler and its release.
pub const EndProbe = struct {
    ends: std.atomic.Value(u32) = .init(0),
    outcome: maplibre.CameraTransitionOutcome = .completed,
    releases: Counter = .{},

    fn record(self: *EndProbe, end: maplibre.CameraTransitionEnd) maplibre.Error!void {
        self.outcome = end.outcome;
        _ = self.ends.fetchAdd(1, .release);
    }

    fn released(self: *EndProbe) void {
        self.releases.add();
    }

    pub fn handler(self: *EndProbe) maplibre.CameraTransitionHandler {
        return .init(self, record, released);
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
    _ = try resolve(future_value);
}

/// Answers every request with a not-found error, so no test reaches the
/// network by accident.
fn denyResource(_: ?*anyopaque, _: maplibre.ResourceRequestInfo, request: maplibre.ResourceRequest) maplibre.Error!void {
    var owned = request;
    try owned.complete(.{
        .status = .@"error",
        .error_reason = .not_found,
        .error_message = "the Zig test fixture serves no resources",
    }, null);
}

pub const denying_provider = maplibre.ResourceProvider{ .callback = denyResource };

/// Map options with every required member set: a 256 by 256 initial extent
/// at scale factor 1.
pub fn mapOptions() maplibre.MapOptions {
    var options = maplibre.mapOptionsDefault();
    options.initial_extent = .{ .width = 256, .height = 256, .scale_factor = 1.0 };
    return options;
}

pub const FixtureOptions = struct {
    asset_path: []const u8 = &.{},
    extent: maplibre.LogicalExtent = .{ .width = 32, .height = 32, .scale_factor = 1.0 },
    map_mode: maplibre.MapMode = .continuous,
};

/// One runtime and one map. The runtime's event handler signals `waitForEvent`.
/// `destroy` closes whatever the test left open, so every path tears down.
pub const Fixture = struct {
    events: Signal = .{},
    event_mutex: std.Io.Mutex = .init,
    pending_events: std.ArrayList(maplibre.OwnedValue(maplibre.RuntimeEvent)) = .empty,
    runtime: maplibre.Runtime,
    map: maplibre.Map,

    pub fn create(options: FixtureOptions) !*Fixture {
        const self = try testing.allocator.create(Fixture);
        errdefer testing.allocator.destroy(self);
        self.* = .{ .runtime = undefined, .map = undefined };
        var runtime_options = try maplibre.runtimeOptionsDefault(testing.allocator);
        defer runtime_options.deinit();
        runtime_options.value.asset_path = options.asset_path;
        self.runtime = try maplibre.runtimeCreate(runtime_options.value, null);
        errdefer closeRuntimeHandle(&self.runtime) catch self.runtime.deinit();
        // A handler accepted before createMap receives the map's first events.
        try self.runtime.discarding().setEventHandler(.init(self, receiveEvents, null), null);
        try self.setProvider(denying_provider);
        var map_options = mapOptions();
        map_options.initial_extent = options.extent;
        map_options.map_mode = options.map_mode;
        const created = try self.runtime.createMap(map_options, null);
        self.map = created.map;
        errdefer self.map.deinit();
        try resolve(created.ready);
        return self;
    }

    fn receiveEvents(self: *Fixture, events: []const maplibre.RuntimeEvent) maplibre.Error!void {
        std.Io.Threaded.mutexLockUncancelable(&self.event_mutex);
        defer self.event_mutex.unlock(testing.io);
        for (events) |event| {
            var arena = std.heap.ArenaAllocator.init(testing.allocator);
            errdefer arena.deinit();
            var copy = event;
            copy.message = try arena.allocator().dupe(u8, event.message);
            try self.pending_events.append(testing.allocator, .{ .arena = arena, .value = copy });
        }
        self.events.notify();
    }

    /// Closes the map and runtime if the test left them open, and frees the
    /// fixture. A close that fails logs an error, which fails the test, and
    /// falls back to disposal so the next test starts clean.
    pub fn destroy(self: *Fixture) void {
        if (!self.map.isClosed()) self.closeMap() catch |err| {
            std.log.err("fixture map close failed: {s}", .{@errorName(err)});
            self.map.deinit();
        };
        if (!self.runtime.isClosed()) self.closeRuntime() catch |err| {
            std.log.err("fixture runtime close failed: {s}", .{@errorName(err)});
            self.runtime.deinit();
        };
        for (self.pending_events.items) |*event| event.deinit();
        self.pending_events.deinit(testing.allocator);
        testing.allocator.destroy(self);
    }

    pub fn closeMap(self: *Fixture) !void {
        try closeMapHandle(&self.map);
    }

    fn closeRuntime(self: *Fixture) !void {
        try closeRuntimeHandle(&self.runtime);
    }

    /// Releases the runtime once native has retired every child that the test
    /// disposed. A release is refused while any child is live or pending, so
    /// the wait retries the release until one is accepted.
    pub fn releaseRuntimeWhenChildless(self: *Fixture) !void {
        try self.events.waitUntil(self, struct {
            fn ready(fixture: *Fixture) anyerror!bool {
                var teardown = fixture.runtime.release(null) catch |err| switch (err) {
                    error.InvalidState => return false,
                    else => return err,
                };
                defer teardown.deinit();
                try teardown.wait(null);
                return true;
            }
        }.ready);
    }

    /// Waits for a runtime command, which follows every submission to the
    /// runtime's worker accepted so far.
    pub fn fence(self: *Fixture) !void {
        try expectCommitted(try self.runtime.clearResourceTransform(null));
    }

    pub fn setProvider(self: *Fixture, provider: maplibre.ResourceProvider) !void {
        try expectCommitted(try self.runtime.setResourceProvider(testing.allocator, provider, null));
    }

    pub fn loadStyle(self: *Fixture) !void {
        try expectCommitted(try self.map.setStyleJson(style_json, null));
        var loaded = try self.waitForEvent(.map_style_loaded);
        loaded.deinit();
    }

    /// Drains until an event of `event_type` arrives and returns a copy of it,
    /// blocking until the event handler receives more events.
    pub fn waitForEvent(self: *Fixture, event_type: maplibre.RuntimeEventType) !maplibre.OwnedValue(maplibre.RuntimeEvent) {
        const deadline = Deadline.start();
        while (true) {
            const seen = self.events.count.load(.acquire);
            std.Io.Threaded.mutexLockUncancelable(&self.event_mutex);
            while (self.pending_events.items.len > 0) {
                var event = self.pending_events.orderedRemove(0);
                if (event.value.type == event_type) {
                    self.event_mutex.unlock(testing.io);
                    return event;
                }
                event.deinit();
            }
            self.event_mutex.unlock(testing.io);
            if (deadline.expired()) return error.EventNotObserved;
            try testing.io.futexWaitTimeout(u32, &self.events.count.raw, seen, .{ .deadline = deadline.at });
        }
    }
};

pub fn closeRuntimeHandle(runtime: *maplibre.Runtime) !void {
    try resolve(try runtime.release(null));
}

pub fn closeMapHandle(map: *maplibre.Map) !void {
    try resolve(try map.release(null));
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
    maplibre.MetalTextureFrame
else if (build_options.supports_vulkan)
    maplibre.VulkanTextureFrame
else
    maplibre.OpenglTextureFrame;

/// Reads a scoped view of an acquired frame's texture on this build's backend.
pub fn getOwnedTexture(frame: maplibre.AcquiredFrame, context: anytype, comptime use: anytype) @TypeOf(getBackendTexture(frame, context, use, null)) {
    return getBackendTexture(frame, context, use, null);
}

const getBackendTexture = if (build_options.supports_metal)
    maplibre.AcquiredFrame.getMetalTexture
else if (build_options.supports_vulkan)
    maplibre.AcquiredFrame.getVulkanTexture
else
    maplibre.AcquiredFrame.getOpenglTexture;

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

    pub fn attach(map: maplibre.Map, extent: maplibre.LogicalExtent, driver: maplibre.RenderDriverKind) !*OwnedTexture {
        const self = try testing.allocator.create(OwnedTexture);
        errdefer testing.allocator.destroy(self);
        self.* = .{ .graphics = try Graphics.create(), .session = undefined, .caller_driven = driver == .caller_graphics_thread };
        errdefer self.graphics.destroy();
        if (build_options.supports_opengl and self.caller_driven) try self.graphics.makeCurrent();
        var defaults = try maplibre.renderSessionAttachOptionsDefault(testing.allocator);
        defer defaults.deinit();
        var options = defaults.value;
        options.driver = driver;
        options.requested_texture_ring_depth = 2;
        options.frame_wake = self.wakes.wake();
        options.driver_work_wake = self.wakes.wake();
        const attachment = if (build_options.supports_metal)
            try map.attachMetalOwnedTexture(.{
                .extent = extent,
                .context = .{ .device = self.graphics.context.metal_device },
                // MTLPixelFormatBGRA8Unorm.
                .pixel_format = 80,
            }, options, null)
        else if (build_options.supports_vulkan)
            try map.attachVulkanOwnedTexture(.{ .extent = extent, .context = self.graphics.vulkanContext() }, options, null)
        else attachment: {
            var descriptor = try maplibre.openglOwnedTextureDescriptorDefault(testing.allocator);
            defer descriptor.deinit();
            descriptor.value.extent = extent;
            descriptor.value.context = self.graphics.openglContext();
            break :attachment try map.attachOpenglOwnedTexture(descriptor.value, options, null);
        };
        self.session = attachment.session;
        errdefer self.release();
        try self.resolve(attachment.ready);
        return self;
    }

    /// Detaches and releases the session unless the test already did, then
    /// destroys the graphics context. A failed detach logs an error, which
    /// fails the test, and the release abandons the session instead.
    pub fn close(self: *OwnedTexture) void {
        if (!self.session.isClosed()) {
            if (self.attached) self.detach() catch |err| {
                std.log.err("render session detach failed: {s}", .{@errorName(err)});
            };
            self.release();
        }
        self.graphics.destroy();
        testing.allocator.destroy(self);
    }

    /// Releases the session and waits until it retires, after which the host
    /// may destroy its graphics objects. A failure logs an error, which fails
    /// the test.
    fn release(self: *OwnedTexture) void {
        var retired = self.session.release(null) catch |err| {
            std.log.err("render session release failed: {s}", .{@errorName(err)});
            return;
        };
        defer retired.deinit();
        retired.wait(null) catch |err| {
            std.log.err("render session retirement failed: {s}", .{@errorName(err)});
        };
    }

    pub fn detach(self: *OwnedTexture) !void {
        try self.resolve(try self.session.detach(null));
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

    fn service(self: *OwnedTexture) !void {
        if (self.caller_driven) _ = try self.session.serviceDriverWork(null);
    }

    /// Requests one forced frame, which renders whether or not the map
    /// changed, and returns its result, blocking on the session's frame wake.
    pub fn renderFrame(self: *OwnedTexture) !maplibre.RenderFrameResult {
        const token = self.next_token;
        self.next_token += 1;
        try self.session.requestFrame(.{ .flags = .{ .force = true }, .token = token }, null);
        const Context = struct { owner: *OwnedTexture, token: u64, result: ?maplibre.RenderFrameResult = null };
        var context = Context{ .owner = self, .token = token };
        try self.wakes.waitUntil(&context, struct {
            fn ready(ctx: *Context) anyerror!bool {
                try ctx.owner.service();
                var results = try ctx.owner.session.drainFrameResults(testing.allocator, null);
                defer results.deinit();
                for (results.value) |result| {
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
                .no_update, .target_not_ready => {},
                else => return error.UnexpectedFrameDisposition,
            }
        }
        return error.FrameDidNotRender;
    }
};
