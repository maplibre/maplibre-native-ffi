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

pub const background_rgba = [4]u8{ 0xd8, 0xf1, 0xff, 0xff };

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

pub const EndProbe = struct {
    ends: std.atomic.Value(u32) = .init(0),
    outcome: maplibre.CameraTransitionOutcome = .completed,

    fn record(self: *EndProbe, end: maplibre.CameraTransitionEnd) maplibre.Error!void {
        self.outcome = end.outcome;
        _ = self.ends.fetchAdd(1, .release);
    }

    pub fn handler(self: *EndProbe) maplibre.CameraTransitionHandler {
        return .init(self, record, null);
    }
};

pub fn resolve(future_value: anytype) !@TypeOf(future_value).Value {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

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
    return .{ .initial_extent = .{ .width = 256, .height = 256, .scale_factor = 1.0 } };
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
        self.runtime = try maplibre.Runtime.create(.{ .asset_path = options.asset_path }, null);
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

    fn openGlContext(self: *const Graphics) maplibre.OpenGlContextDescriptor {
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

/// The texture of an acquired frame on the build's backend.
pub const FrameTexture = if (build_options.supports_metal)
    maplibre.MetalTextureFrame
else if (build_options.supports_vulkan)
    maplibre.VulkanTextureFrame
else
    maplibre.OpenGlTextureFrame;

pub fn getFrameTexture(frame: maplibre.AcquiredFrame, context: anytype, comptime use: anytype) @TypeOf(getBackendTexture(frame, context, use, null)) {
    return getBackendTexture(frame, context, use, null);
}

const getBackendTexture = if (build_options.supports_metal)
    maplibre.AcquiredFrame.getMetalTexture
else if (build_options.supports_vulkan)
    maplibre.AcquiredFrame.getVulkanTexture
else
    maplibre.AcquiredFrame.getOpenGlTexture;

const BorrowedTexture = if (build_options.supports_metal)
    maplibre.MetalBorrowedTexture
else if (build_options.supports_vulkan)
    maplibre.VulkanBorrowedTexture
else
    maplibre.OpenGlBorrowedTexture;

/// A render session on a fixture's map, with the wakes that its waits block
/// on. An offscreen session renders on a device of its own. A borrowed
/// session renders into a ring of textures from tests/graphics.
pub const TextureSession = struct {
    wakes: Signal = .{},
    graphics: ?Graphics = null,
    textures: [ring_depth]?*test_graphics.mln_test_graphics_texture = @splat(null),
    session: maplibre.RenderSession = undefined,
    caller_driven: bool = false,
    attached: bool = true,
    next_token: u64 = 1,

    /// Two textures, so a test can hold two acquired frames at once.
    const ring_depth = 2;

    /// The driver of a borrowed session. OpenGL sessions share the fixture
    /// context, which is current on the thread that attaches, so they take
    /// the caller driver.
    pub const borrowed_driver: maplibre.RenderDriverKind = if (build_options.supports_opengl) .caller_graphics_thread else .core_worker;

    pub fn attachOffscreen(map: maplibre.Map, extent: maplibre.LogicalExtent) !*TextureSession {
        const self = try testing.allocator.create(TextureSession);
        errdefer testing.allocator.destroy(self);
        self.* = .{};
        const attachment = try map.attachOffscreen(.{ .extent = extent }, self.attachOptions(.core_worker), null, null);
        self.session = attachment.session;
        errdefer self.release();
        try self.resolve(attachment.ready);
        return self;
    }

    pub fn attachBorrowed(map: maplibre.Map, extent: maplibre.LogicalExtent, driver: maplibre.RenderDriverKind) !*TextureSession {
        const self = try testing.allocator.create(TextureSession);
        errdefer testing.allocator.destroy(self);
        self.* = .{ .graphics = try Graphics.create(), .caller_driven = driver == .caller_graphics_thread };
        errdefer self.destroyGraphics();
        const graphics = &self.graphics.?;
        if (build_options.supports_opengl) try graphics.makeCurrent();
        const width: u32 = @intFromFloat(@round(@as(f64, @floatFromInt(extent.width)) * extent.scale_factor));
        const height: u32 = @intFromFloat(@round(@as(f64, @floatFromInt(extent.height)) * extent.scale_factor));
        var entries: [ring_depth]BorrowedTexture = undefined;
        var info: test_graphics.mln_test_graphics_texture_info = undefined;
        for (&self.textures, &entries) |*texture, *entry| {
            texture.* = test_graphics.mln_test_graphics_texture_create(graphics.handle, width, height) orelse return Graphics.graphicsError();
            if (!test_graphics.mln_test_graphics_texture_get_info(texture.*.?, &info)) return Graphics.graphicsError();
            entry.* = if (build_options.supports_metal)
                .{ .texture = info.metal_texture }
            else if (build_options.supports_vulkan)
                .{ .image = info.vulkan_image, .image_view = info.vulkan_image_view }
            else
                .{ .texture = info.opengl_texture };
        }
        const options = self.attachOptions(driver);
        const attachment = if (build_options.supports_metal)
            try map.attachMetalBorrowedTexture(testing.allocator, .{
                .extent = extent,
                .physical_width = info.width,
                .physical_height = info.height,
                .textures = &entries,
            }, options, null)
        else if (build_options.supports_vulkan)
            try map.attachVulkanBorrowedTexture(testing.allocator, .{
                .extent = extent,
                .physical_width = info.width,
                .physical_height = info.height,
                .context = graphics.vulkanContext(),
                .textures = &entries,
                .format = info.format,
                .final_layout = info.vulkan_final_layout,
            }, options, null)
        else
            try map.attachOpenGlBorrowedTexture(testing.allocator, .{
                .extent = extent,
                .physical_width = info.width,
                .physical_height = info.height,
                .context = graphics.openGlContext(),
                .textures = &entries,
                .target = info.opengl_target,
            }, options, null);
        self.session = attachment.session;
        errdefer self.release();
        try self.resolve(attachment.ready);
        return self;
    }

    fn attachOptions(self: *TextureSession, driver: maplibre.RenderDriverKind) maplibre.RenderSessionAttachOptions {
        return .{
            .driver = driver,
            .frame_wake = self.wakes.wake(),
            .driver_work_wake = self.wakes.wake(),
        };
    }

    /// Detaches and releases the session unless the test already did, then
    /// destroys the textures and graphics context. A failed detach logs an
    /// error, which fails the test, and the release abandons the session
    /// instead.
    pub fn close(self: *TextureSession) void {
        if (!self.session.isClosed()) {
            if (self.attached) self.detach() catch |err| {
                std.log.err("render session detach failed: {s}", .{@errorName(err)});
            };
            self.release();
        }
        self.destroyGraphics();
        testing.allocator.destroy(self);
    }

    fn destroyGraphics(self: *TextureSession) void {
        for (&self.textures) |*texture| {
            test_graphics.mln_test_graphics_texture_destroy(texture.*);
            texture.* = null;
        }
        if (self.graphics) |*graphics| graphics.destroy();
        self.graphics = null;
    }

    /// Releases the session and waits until it retires, after which the host
    /// may destroy its graphics objects. A failure logs an error, which fails
    /// the test.
    fn release(self: *TextureSession) void {
        var retired = self.session.release(null) catch |err| {
            std.log.err("render session release failed: {s}", .{@errorName(err)});
            return;
        };
        defer retired.deinit();
        retired.wait(null) catch |err| {
            std.log.err("render session retirement failed: {s}", .{@errorName(err)});
        };
    }

    pub fn detach(self: *TextureSession) !void {
        try self.resolve(try self.session.detach(null));
        self.attached = false;
    }

    /// Waits for a session future, servicing a caller driver's work between
    /// checks.
    pub fn resolve(self: *TextureSession, future_value: anytype) !@TypeOf(future_value).Value {
        var future = future_value;
        defer future.deinit();
        const Context = struct { owner: *TextureSession, future: *@TypeOf(future) };
        try self.wakes.waitUntil(Context{ .owner = self, .future = &future }, struct {
            fn ready(context: Context) anyerror!bool {
                try context.owner.service();
                return context.future.poll();
            }
        }.ready);
        return future.wait(null);
    }

    fn service(self: *TextureSession) !void {
        if (self.caller_driven) _ = try self.session.serviceDriverWork(null);
    }

    /// Requests one forced frame, which renders whether or not the map
    /// changed, and returns its result, blocking on the session's frame wake.
    pub fn renderFrame(self: *TextureSession) !maplibre.RenderFrameResult {
        const token = self.next_token;
        self.next_token += 1;
        try self.session.demandFrame(.{ .flags = .{ .force = true }, .token = token }, null);
        const Context = struct { owner: *TextureSession, token: u64, result: ?maplibre.RenderFrameResult = null };
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
    pub fn renderUntilRendered(self: *TextureSession) !maplibre.RenderFrameResult {
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
