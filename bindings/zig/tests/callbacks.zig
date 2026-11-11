//! Callback registrations, host errors inside callbacks, resource-request
//! handles, provider routes, and scoped callback arguments.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

const LogProbe = struct {
    saw_parse_error: support.Flag = .{},

    fn record(self: *LogProbe, severity: maplibre.LogSeverity, event: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!void {
        if (severity == .@"error" and event == .parse_style and message.len != 0) self.saw_parse_error.set();
    }

    fn handler(self: *LogProbe) maplibre.LogHandler {
        return .init(self, record, null);
    }
};

// The log callback is process-global, and a replacement receives the records
// native logs after it is installed.
test "a replacement process-global log callback receives native log records" {
    var first = LogProbe{};
    var replacement = LogProbe{};
    try maplibre.setLogCallback(first.handler(), null);
    defer maplibre.clearLogCallback(null) catch |err| std.log.err("log callback clear failed: {s}", .{@errorName(err)});
    try maplibre.setLogCallback(replacement.handler(), null);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try testing.expectError(error.InvalidArgument, (try fixture.map.setStyleJson("{", null)).await(null));
    try replacement.saw_parse_error.wait();
    try testing.expect(!first.saw_parse_error.isSet());
}

const failing_style_url = "custom://failing-provider.json";

fn completeThenFail(_: ?*anyopaque, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
    if (std.mem.eql(u8, request.requested_url, failing_style_url)) {
        var owned = handle;
        try owned.complete(.{ .bytes = support.style_json }, null);
    }
    return error.NativeError;
}

// A provider's error stays inside the binding, which logs it and releases the
// request it adopted. A request the provider already answered keeps that
// answer, and one it did not answer fails.
test "an error returned by a resource provider is contained" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = completeThenFail });

    try support.expectCommitted(try fixture.map.setStyleUrl(failing_style_url, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();

    try fixture.map.discarding().setStyleUrl("custom://unanswered.json", null);
    var failed = try fixture.waitForEvent(.map_loading_failed);
    defer failed.deinit();
}

const DeferringProvider = struct {
    url: []const u8,
    taken: support.Counter = .{},
    handles: [4]maplibre.ResourceRequest = undefined,

    fn provide(self: *DeferringProvider, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
        if (!std.mem.eql(u8, request.requested_url, self.url)) {
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
        // The load waits for the kept request, so its result is discarded.
        try fixture.map.discarding().setStyleUrl(self.url, null);
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

// A refused answer still consumes the handle, and its request fails as a
// release fails it.
test "a refused answer consumes its request handle and fails the request" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://refused.json" };
    try provider.install(fixture);
    var handle = try provider.take(fixture);
    defer handle.release();

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, handle.complete(.{ .etag = "bad\x00tag" }, &diagnostic));
    try testing.expect(diagnostic.status != null);
    try testing.expect(handle.isClosed());
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
}

// A provider can keep its request and answer it later from any thread. An
// accepted answer consumes the handle. Native refuses a second completion,
// through the consumed handle or a copy made before the answer.
test "a kept request handle answers its request later from another thread" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://later.json" };
    try provider.install(fixture);
    var handle = try provider.take(fixture);
    defer handle.release();
    try testing.expect(!try handle.isCancelled(null));

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
    try support.expectCommitted(try fixture.runtime.setHttpHeaderTransform(testing.allocator, .init(&probe, HeaderProbe.transform, null), null));

    try fixture.map.discarding().setStyleUrl("http://127.0.0.1:1/style.json", null);
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.acquire));

    try testing.expectEqual(@as(?anyerror, error.InvalidState), probe.other_thread);
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, (probe.saved.?).setHeader("X-Test", "late", &diagnostic));
    try testing.expectEqual(null, diagnostic.status);
    try support.expectCommitted(try fixture.runtime.clearHttpHeaderTransform(null));
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

    fn answer(request: *maplibre.ResourceTransformRequest, result: *?anyerror) void {
        request.complete("unsupported://rewritten.json", null) catch |err| {
            result.* = err;
        };
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
    try support.expectCommitted(try fixture.runtime.setResourceTransform(testing.allocator, .init(&probe, TransformProbe.transform, null), null));

    try fixture.map.discarding().setStyleUrl("unsupported://original.json", null);
    try probe.taken.waitFor(1);
    var request = probe.saved.?;
    var answered: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, TransformProbe.answer, .{ &request, &answered });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), answered);
    // The accepted answer consumed the request, so native refuses a second one.
    try testing.expect(request.isClosed());
    try testing.expectError(error.InvalidState, request.complete("unsupported://rewritten.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try support.expectCommitted(try fixture.runtime.clearResourceTransform(null));
}

const routed_style_url = "custom://routed/style.json";
const routed_source_url = "custom://routed/source.json";

/// A style whose one source is fetched under the routed prefix as a source
/// request, not a style request.
const routed_style_json =
    \\{"version":8,"sources":{"routed":{"type":"vector","url":"custom://routed/source.json"}},"layers":[]}
;

const RoutedProvider = struct {
    styles: support.Counter = .{},
    sources: support.Counter = .{},
    others: support.Counter = .{},

    fn provide(self: *RoutedProvider, request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
        const url = request.requested_url;
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
    const routes = try scratch.allocator().dupe(maplibre.ResourceRoute, &.{
        .{ .kinds = .{ .tile = true }, .flags = .{ .glob = true }, .url = try scratch.allocator().dupe(u8, "**") },
        .{ .flags = .{ .glob = true }, .url = try scratch.allocator().dupe(u8, "custom://routed/**") },
    });
    var routed = maplibre.ResourceProvider.init(&provider, RoutedProvider.provide, null);
    routed.routes = routes;
    const registered = try fixture.runtime.setResourceProvider(testing.allocator, routed, null);
    _ = scratch.reset(.free_all);
    try support.expectCommitted(registered);

    try fixture.map.discarding().setStyleUrl("custom://outside.json", null);
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
// Native holds the handler while the ease runs, and runs it once, before the
// cancellation completes, when the command's identity is cancelled.
test "a camera end handler inside a command runs once when its transition ends" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var ended = support.EndProbe{};
    const update: maplibre.CameraUpdate = .{
        .mode = .ease,
        .camera = .{ .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000, .transition_id = 3, .end_handler = ended.handler() },
    };
    try support.expectCommitted(try fixture.map.updateCamera(update, null));
    try testing.expectEqual(@as(u32, 0), ended.ends.load(.acquire));
    try support.expectCommitted(try fixture.map.cancelCameraTransitionsWithId(3, null));
    try testing.expectEqual(@as(u32, 1), ended.ends.load(.acquire));
    try testing.expectEqual(maplibre.CameraTransitionOutcome.cancelled, ended.outcome);
}
