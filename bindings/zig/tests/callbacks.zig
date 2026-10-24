//! Callback registrations, host errors inside callbacks, resource-request
//! handles, provider routes, and scoped callback arguments.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

fn fetchNothing(_: *support.Counter, _: maplibre.CanonicalTileId) void {}

fn countRelease(counter: *support.Counter) void {
    counter.add();
}

fn customSource(counter: *support.Counter, max_zoom: ?f64) maplibre.CustomGeometrySourceOptions {
    var options = maplibre.CustomGeometrySourceOptions.init(counter, fetchNothing, null, countRelease);
    options.max_zoom = max_zoom;
    return options;
}

// The binding roots a registration's context from submission until native
// releases it, then runs release_context once. A submission native refuses
// never hands the context over, so the caller keeps it and nothing runs.
test "a callback registration is rooted until native releases it" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();

    var removed = support.Counter{};
    var retained = support.Counter{};
    var refused = support.Counter{};
    var duplicate = support.Counter{};
    try support.expectCommitted(try fixture.map.addCustomGeometrySource("removed", customSource(&removed, null), null));
    try support.expectCommitted(try fixture.map.addCustomGeometrySource("retained", customSource(&retained, null), null));
    try testing.expectError(error.InvalidArgument, fixture.map.addCustomGeometrySource("refused", customSource(&refused, 33.0), null));

    // A registration that native accepted and then failed to commit is native's
    // to release.
    try testing.expectError(error.InvalidArgument, support.resolve(try fixture.map.addCustomGeometrySource("removed", customSource(&duplicate, null), null)));
    try duplicate.waitFor(1);

    try support.expectCommitted(try fixture.map.removeStyleSource("removed", null));
    try removed.waitFor(1);
    try fixture.fence();
    try testing.expectEqual(@as(usize, 0), retained.get());
    try fixture.closeMap();
    try retained.waitFor(1);

    try testing.expectEqual(@as(usize, 1), removed.get());
    try testing.expectEqual(@as(usize, 1), duplicate.get());
    try testing.expectEqual(@as(usize, 0), refused.get());
}

const LogProbe = struct {
    saw_parse_error: support.Flag = .{},
    releases: support.Counter = .{},

    fn record(self: *LogProbe, severity: maplibre.LogSeverity, event: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!u32 {
        if (severity == .@"error" and event == .parse_style and message.len != 0) self.saw_parse_error.set();
        return 1;
    }

    fn released(self: *LogProbe) void {
        self.releases.add();
    }

    fn handler(self: *LogProbe) maplibre.LogHandler {
        return .init(self, record, released);
    }
};

// The log callback is process-global. Installing a replacement releases the
// registration it replaces, and clearing releases the replacement.
test "replacing the process-global log callback releases the previous one" {
    var first = LogProbe{};
    var replacement = LogProbe{};
    try maplibre.logSetCallback(first.handler(), null);
    defer maplibre.logClearCallback(null) catch |err| std.log.err("log callback clear failed: {s}", .{@errorName(err)});
    try maplibre.logSetCallback(replacement.handler(), null);
    try first.releases.waitFor(1);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try testing.expectError(error.NativeError, support.resolve(try fixture.map.setStyleJson("{", null)));
    try replacement.saw_parse_error.wait();

    try maplibre.logClearCallback(null);
    try replacement.releases.waitFor(1);
    try testing.expectEqual(@as(usize, 1), first.releases.get());
}

const failing_style_url = "custom://failing-provider.json";

fn completeThenFail(_: ?*anyopaque, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
    if (std.mem.eql(u8, request.requested_url orelse "", failing_style_url)) {
        var owned = handle;
        try owned.complete(.{ .bytes = support.style_json }, null);
    }
    return error.NativeError;
}

// A provider's error stays inside the binding, which reports it and releases
// the request it adopted. A request the provider already answered keeps that
// answer, and one it did not answer fails.
test "an error returned by a resource provider is contained and reported" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = completeThenFail });

    try support.expectCommitted(try fixture.map.setStyleUrl(failing_style_url, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();

    try support.expectCommitted(try fixture.map.setStyleUrl("custom://unanswered.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    defer failed.deinit();
}

/// A provider that keeps every request matching `url`, hands it to the test,
/// and denies the rest.
const DeferringProvider = struct {
    url: []const u8,
    taken: support.Counter = .{},
    handles: [4]maplibre.ResourceRequest = undefined,

    fn provide(self: *DeferringProvider, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
        if (!std.mem.eql(u8, request.requested_url orelse "", self.url)) {
            var owned = handle;
            return owned.complete(.{ .status = .@"error", .error_reason = .not_found }, null);
        }
        const index = self.taken.get();
        self.handles[index] = handle;
        self.taken.add();
    }

    fn install(self: *DeferringProvider, fixture: *support.Fixture) !void {
        try fixture.setProvider(.init(self, provide, null));
    }

    fn take(self: *DeferringProvider, fixture: *support.Fixture) !maplibre.ResourceRequest {
        const index = self.taken.get();
        try support.expectCommitted(try fixture.map.setStyleUrl(self.url, null));
        try self.taken.waitFor(index + 1);
        return self.handles[index];
    }
};

fn completeOnThread(handle: *maplibre.ResourceRequest, result: *?anyerror) void {
    handle.complete(.{ .bytes = support.style_json }, null) catch |err| {
        result.* = err;
        return;
    };
    result.* = null;
}

// A provider can keep its request and answer it later from any thread. The
// binding validates the response's strings first, a refused completion leaves
// the handle open, and an accepted one closes it. Native refuses a second
// completion, through the closed handle or a copy made before it closed.
test "a kept request handle answers its request later from another thread" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://later.json" };
    try provider.install(fixture);
    var handle = try provider.take(fixture);
    defer handle.release();
    try testing.expect(!try handle.isCancelled(null));

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, handle.complete(.{ .etag = "bad\x00tag" }, &diagnostic));
    try testing.expect(diagnostic.status != null);
    try testing.expect(!handle.isClosed());

    var copy = handle;
    var thread_result: ?anyerror = error.NotRun;
    const thread = try std.Thread.spawn(.{}, completeOnThread, .{ &handle, &thread_result });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), thread_result);
    try testing.expect(handle.isClosed());
    try testing.expectError(error.InvalidState, handle.complete(.{ .bytes = support.style_json }, null));
    try testing.expectError(error.InvalidState, copy.complete(.{ .bytes = support.style_json }, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
}

// A released handle's copies stay closed, including after the provider takes
// further requests.
test "released request handle copies stay closed after later requests" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://stale.json" };
    try provider.install(fixture);

    var stale = try provider.take(fixture);
    var stale_copy = stale;
    stale.release();
    try testing.expectError(error.InvalidState, stale_copy.isCancelled(null));

    var live = try provider.take(fixture);
    defer live.release();
    try testing.expectError(error.InvalidState, stale_copy.complete(.{ .bytes = support.style_json }, null));
    try live.complete(.{ .bytes = support.style_json }, null);
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
}

const CancelProbe = struct {
    cancels: support.Counter = .{},
    releases: support.Counter = .{},

    fn cancelled(self: *CancelProbe) maplibre.Error!void {
        self.cancels.add();
    }

    fn released(self: *CancelProbe) void {
        self.releases.add();
    }
};

// Registering a cancel callback after cancellation returns true, and native
// takes no ownership of the registration. The binding then frees the
// registration before returning and leaves the context unrooted, so neither
// the callback nor release_context ever runs.
test "a registration that reports cancellation is never rooted" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://cancelled.json" };
    try provider.install(fixture);
    var handle = try provider.take(fixture);
    try fixture.closeMap();

    // Cancellation lands on a MapLibre thread with nothing registered to
    // report it, so the wait checks the request again after each wake.
    try fixture.events.waitUntil(handle, struct {
        fn ready(request: maplibre.ResourceRequest) anyerror!bool {
            return request.isCancelled(null);
        }
    }.ready);

    var probe = CancelProbe{};
    try testing.expect(try handle.setCancelCallback(.init(&probe, CancelProbe.cancelled, CancelProbe.released), null));
    handle.release();
    try fixture.releaseRuntimeWhenChildless();
    try testing.expectEqual(@as(usize, 0), probe.cancels.get());
    try testing.expectEqual(@as(usize, 0), probe.releases.get());
}

fn passThrough(_: ?*anyopaque, _: maplibre.ResourceRequestInfo, request: maplibre.ResourceRequest) maplibre.Error!void {
    var owned = request;
    try owned.passThrough(null);
}

const HeaderProbe = struct {
    calls: std.atomic.Value(usize) = .init(0),
    saved: ?maplibre.HttpHeaderTransformResponse = null,
    other_thread: ?anyerror = null,

    fn setHeaderOnThread(response: maplibre.HttpHeaderTransformResponse, result: *?anyerror) void {
        result.* = if (response.setHeader("X-Test", "other", null)) null else |err| err;
    }

    fn transform(self: *HeaderProbe, kind: maplibre.ResourceKind, _: []const u8, response: maplibre.HttpHeaderTransformResponse) maplibre.Error!void {
        if (kind != .style or self.calls.load(.acquire) != 0) return;
        const thread = std.Thread.spawn(.{}, setHeaderOnThread, .{ response, &self.other_thread }) catch return error.OutOfMemory;
        thread.join();
        try response.setHeader("X-Test", "inside", null);
        self.saved = response;
        _ = self.calls.fetchAdd(1, .release);
    }
};

// A header transform's response object is valid only on the callback's thread
// while the callback runs. The provider passes the request through to the HTTP
// client, which runs the header transform before it tries a loopback port
// that nothing serves, so the style fails to load without leaving the machine.
test "a scoped response rejects use after its callback and from another thread" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = passThrough });
    var probe = HeaderProbe{};
    try support.resolve(try fixture.runtime.setHttpHeaderTransform(testing.allocator, .init(&probe, HeaderProbe.transform, null), null));

    try support.expectCommitted(try fixture.map.setStyleUrl("http://127.0.0.1:1/style.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.acquire));

    try testing.expectEqual(@as(?anyerror, error.InvalidState), probe.other_thread);
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, (probe.saved.?).setHeader("X-Test", "late", &diagnostic));
    try testing.expectEqual(null, diagnostic.status);
    try support.resolve(try fixture.runtime.clearHttpHeaderTransform(null));
}

const TransformProbe = struct {
    saved: ?maplibre.ResourceTransformRequest = null,
    taken: support.Counter = .{},

    fn transform(self: *TransformProbe, kind: maplibre.ResourceKind, _: []const u8, request: maplibre.ResourceTransformRequest) maplibre.Error!void {
        if (kind != .style or self.saved != null) {
            var owned = request;
            return owned.release();
        }
        self.saved = request;
        self.taken.add();
    }

    fn answer(request: maplibre.ResourceTransformRequest, result: *?anyerror) void {
        request.complete("unsupported://rewritten.json", null) catch |err| {
            result.* = err;
            return;
        };
        var owned = request;
        owned.release();
    }
};

// A transform owns the request its callback receives, and another thread
// answers it after the callback returns. The transform runs in the native
// network loader, after the provider passes a request through, and no loader
// serves the rewritten scheme.
test "a transform request is answered from another thread after its callback" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = passThrough });
    var probe = TransformProbe{};
    try support.resolve(try fixture.runtime.setResourceTransform(.init(&probe, TransformProbe.transform, null), null));

    try support.expectCommitted(try fixture.map.setStyleUrl("unsupported://original.json", null));
    try probe.taken.waitFor(1);
    var answered: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, TransformProbe.answer, .{ probe.saved.?, &answered });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), answered);
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try support.resolve(try fixture.runtime.clearResourceTransform(null));
}

const routed_style_url = "custom://routed/style.json";
const routed_source_url = "custom://routed/source.json";

/// A style whose one source is fetched under the routed prefix as a source
/// request, not a style request.
const routed_style_json =
    \\{"version":8,"sources":{"routed":{"type":"vector","url":"custom://routed/source.json"}},"layers":[]}
;

/// A provider that serves the routed style, denies the routed source, and
/// counts every request it receives by what it is.
const RoutedProvider = struct {
    styles: support.Counter = .{},
    sources: support.Counter = .{},
    others: support.Counter = .{},

    fn provide(self: *RoutedProvider, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
        const url = request.requested_url orelse "";
        const is_style = request.kind == .style and std.mem.eql(u8, url, routed_style_url);
        const is_source = request.kind == .source and std.mem.eql(u8, url, routed_source_url);
        var owned = handle;
        if (is_style) {
            try owned.complete(.{ .bytes = routed_style_json }, null);
        } else {
            try owned.complete(.{ .status = .@"error", .error_reason = .not_found }, null);
        }
        if (is_style) {
            self.styles.add();
        } else if (is_source) {
            self.sources.add();
        } else {
            self.others.add();
        }
    }

    fn calls(self: *RoutedProvider) usize {
        return self.styles.get() + self.sources.get() + self.others.get();
    }
};

// Registration copies the routes, so the caller frees them before the
// registration settles. A style request outside every route passes through to
// the native loader, even though a tile route's pattern matches its URL. The
// route that names only a URL pattern claims both the style and the source
// that the style references.
test "a provider receives only the requests its routes match" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = RoutedProvider{};

    var scratch = std.heap.ArenaAllocator.init(testing.allocator);
    defer scratch.deinit();
    var fallback = try maplibre.resourceRouteDefault(scratch.allocator());
    fallback.value.flags = .{ .glob = true };
    fallback.value.url = try scratch.allocator().dupe(u8, "custom://routed/**");
    const routes = try scratch.allocator().dupe(maplibre.ResourceRoute, &.{
        .{ .kinds = .{ .tile = true }, .flags = .{ .glob = true }, .url = try scratch.allocator().dupe(u8, "**") },
        fallback.value,
    });
    var routed = maplibre.ResourceProvider.init(&provider, RoutedProvider.provide, null);
    routed.routes = routes;
    const registered = try fixture.runtime.setResourceProvider(testing.allocator, routed, null);
    _ = scratch.reset(.free_all);
    try support.resolve(registered);

    try support.expectCommitted(try fixture.map.setStyleUrl("custom://outside.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    defer failed.deinit();
    try testing.expect(std.mem.indexOf(u8, failed.value.message, "custom://outside.json") != null);
    try testing.expectEqual(@as(usize, 0), provider.calls());

    try support.expectCommitted(try fixture.map.setStyleUrl(routed_style_url, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
    try provider.sources.waitFor(1);
    try testing.expectEqual(@as(usize, 1), provider.styles.get());
    try testing.expectEqual(@as(usize, 0), provider.others.get());
}

// A camera command carries its end handler two records deep in its input.
// Native runs the handler once and releases it when the command's identity is
// cancelled. A command that native rejects leaves the context with the
// caller, so neither callback runs.
test "a camera end handler inside a command is released after it runs" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var ended = support.EndProbe{};
    var update = try maplibre.cameraUpdateDefault(testing.allocator);
    defer update.deinit();
    update.value.mode = .ease;
    update.value.camera = .{ .zoom = 4.0 };
    update.value.animation = .{ .duration_ms = 60_000, .transition_id = 3, .end_handler = ended.handler() };
    try support.expectCommitted(try fixture.map.updateCamera(update.value, null));
    try testing.expectEqual(@as(usize, 0), ended.releases.get());
    try support.expectCommitted(try fixture.map.cancelCameraTransition(3, null));
    try testing.expectEqual(@as(u32, 1), ended.ends.load(.acquire));
    try testing.expectEqual(maplibre.CameraTransitionOutcome.cancelled, ended.outcome);
    try ended.releases.waitFor(1);

    var rejected = support.EndProbe{};
    var delta = try maplibre.cameraDeltaDefault(testing.allocator);
    defer delta.deinit();
    delta.value.scale = -1;
    delta.value.animation.end_handler = rejected.handler();
    try testing.expectError(error.InvalidArgument, fixture.map.applyCameraDelta(delta.value, null));
    try fixture.fence();
    try testing.expectEqual(@as(usize, 0), rejected.releases.get());
    try testing.expectEqual(@as(u32, 0), rejected.ends.load(.acquire));
}
