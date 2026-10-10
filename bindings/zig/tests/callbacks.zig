//! Callback registrations, host errors inside callbacks, resource-request
//! decisions, and scoped callback arguments.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

fn fetchNothing(_: ?*anyopaque, _: maplibre.CanonicalTileId) maplibre.Error!void {}

fn countRelease(context: ?*anyopaque) void {
    const counter: *support.Counter = @ptrCast(@alignCast(context.?));
    counter.add();
}

fn customSource(counter: *support.Counter, max_zoom: ?f64) maplibre.CustomGeometrySourceOptions {
    return .{ .fetch_tile = fetchNothing, .context = counter, .release_context = countRelease, .max_zoom = max_zoom };
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
    try support.expectCommitted(try maplibre.mapAddCustomGeometrySource(testing.allocator, fixture.map, "removed", customSource(&removed, null), null));
    try support.expectCommitted(try maplibre.mapAddCustomGeometrySource(testing.allocator, fixture.map, "retained", customSource(&retained, null), null));
    try testing.expectError(error.InvalidArgument, maplibre.mapAddCustomGeometrySource(testing.allocator, fixture.map, "refused", customSource(&refused, 33.0), null));

    // A registration that native accepted and then failed to commit is native's
    // to release.
    const failed = try support.resolve(try maplibre.mapAddCustomGeometrySource(testing.allocator, fixture.map, "removed", customSource(&duplicate, null), null));
    try testing.expectEqual(maplibre.CommandDisposition.failed, failed.disposition);
    try duplicate.waitFor(1);

    try support.expectCommitted(try maplibre.mapRemoveStyleSource(fixture.map, "removed", null));
    try removed.waitFor(1);
    try fixture.barrier();
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

    fn record(context: ?*anyopaque, severity: maplibre.LogSeverity, event: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!u32 {
        const self: *LogProbe = @ptrCast(@alignCast(context.?));
        if (severity == .@"error" and event == .parse_style and message.len != 0) self.saw_parse_error.set();
        return 1;
    }

    fn released(context: ?*anyopaque) void {
        const self: *LogProbe = @ptrCast(@alignCast(context.?));
        self.releases.add();
    }

    fn handler(self: *LogProbe) maplibre.LogHandler {
        return .{ .callback = record, .context = self, .release_context = released };
    }
};

// The log callback is process-global. Installing a replacement releases the
// registration it replaces, and clearing releases the replacement.
test "replacing the process-global log callback releases the previous one" {
    var first = LogProbe{};
    var replacement = LogProbe{};
    try maplibre.logSetCallback(testing.allocator, first.handler(), null);
    defer maplibre.logClearCallback(null) catch |err| std.log.err("log callback clear failed: {s}", .{@errorName(err)});
    try maplibre.logSetCallback(testing.allocator, replacement.handler(), null);
    try first.releases.waitFor(1);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    const unparseable = try support.resolve(try maplibre.mapSetStyleJson(fixture.map, "{", null));
    try testing.expectEqual(maplibre.CommandDisposition.failed, unparseable.disposition);
    try replacement.saw_parse_error.wait();

    try maplibre.logClearCallback(null);
    try replacement.releases.waitFor(1);
    try testing.expectEqual(@as(usize, 1), first.releases.get());
}

const failing_style_url = "custom://failing-provider.json";

var provider_failures = support.Counter{};

fn completeThenFail(_: ?*anyopaque, request: maplibre.ResourceRequest, handle: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
    if (std.mem.eql(u8, request.requested_url orelse "", failing_style_url)) {
        try maplibre.resourceRequestComplete(testing.allocator, handle, .{ .bytes = support.style_json }, null);
        try maplibre.resourceRequestRelease(handle);
    }
    provider_failures.add();
    return error.NativeError;
}

/// The callback errors that the binding reported, and whether each one named
/// the provider's callback type and error.
const ProviderReports = struct {
    var matching = support.Counter{};
    var other = std.atomic.Value(usize).init(0);

    fn report(callback: []const u8, err: anyerror) void {
        if (std.mem.eql(u8, callback, "mln_resource_provider_callback") and err == error.NativeError) {
            matching.add();
        } else {
            _ = other.fetchAdd(1, .acq_rel);
        }
    }
};

// A provider's error stays inside the binding, which reports it. A request the
// provider already answered keeps that answer, and one it did not answer
// passes through to the native loader, which has no loader for the custom
// scheme.
test "an error returned by a resource provider is contained and reported" {
    const previous = maplibre.setCallbackErrorReporter(ProviderReports.report);
    defer _ = maplibre.setCallbackErrorReporter(previous);
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = completeThenFail });

    try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, fixture.map, failing_style_url, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();

    try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, fixture.map, "custom://unanswered.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    defer failed.deinit();
    try testing.expect(std.mem.indexOf(u8, failed.value.message, "custom://unanswered.json") != null);

    // Each failed call reports once, before native receives its fallback.
    try testing.expectEqual(provider_failures.get(), ProviderReports.matching.get());
    try testing.expect(ProviderReports.matching.get() >= 2);
    try testing.expectEqual(@as(usize, 0), ProviderReports.other.load(.acquire));
}

/// A provider that takes every request matching `url` as a decision, hands
/// it to the test, and denies the rest.
const DeferringProvider = struct {
    url: []const u8,
    taken: support.Counter = .{},
    handles: [4]maplibre.ResourceRequestHandle = undefined,

    fn provide(context: ?*anyopaque, request: maplibre.ResourceRequest, handle: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
        const self: *DeferringProvider = @ptrCast(@alignCast(context.?));
        if (!std.mem.eql(u8, request.requested_url orelse "", self.url)) {
            try maplibre.resourceRequestComplete(std.heap.smp_allocator, handle, .{ .status = .@"error", .error_reason = .not_found }, null);
            try maplibre.resourceRequestRelease(handle);
            return .handle;
        }
        const index = self.taken.get();
        self.handles[index] = handle;
        self.taken.add();
        return .handle;
    }

    fn install(self: *DeferringProvider, fixture: *support.Fixture) !void {
        try fixture.setProvider(.{ .callback = provide, .context = self });
    }

    fn take(self: *DeferringProvider, fixture: *support.Fixture) !maplibre.ResourceRequestHandle {
        const index = self.taken.get();
        try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, fixture.map, self.url, null));
        try self.taken.waitFor(index + 1);
        return self.handles[index];
    }
};

fn completeOnThread(handle: maplibre.ResourceRequestHandle, result: *?anyerror) void {
    maplibre.resourceRequestComplete(std.heap.smp_allocator, handle, .{ .bytes = support.style_json }, null) catch |err| {
        result.* = err;
        return;
    };
    result.* = null;
}

// A provider can keep a request as a decision handle and answer it later from
// any thread. The binding validates the response's strings first, and a
// handle accepts exactly one completion.
test "a decision handle answers its request later from another thread" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var provider = DeferringProvider{ .url = "custom://later.json" };
    try provider.install(fixture);
    const handle = try provider.take(fixture);
    defer maplibre.resourceRequestRelease(handle) catch {};
    try testing.expect(!try maplibre.resourceRequestIsCancelled(handle, null));

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidString, maplibre.resourceRequestComplete(testing.allocator, handle, .{ .etag = "bad\x00tag" }, &diagnostic));
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);

    var thread_result: ?anyerror = error.NotRun;
    const thread = try std.Thread.spawn(.{}, completeOnThread, .{ handle, &thread_result });
    thread.join();
    try testing.expectEqual(@as(?anyerror, null), thread_result);
    try testing.expectError(error.AlreadyCompleted, maplibre.resourceRequestComplete(testing.allocator, handle, .{ .bytes = support.style_json }, null));
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

    const stale = try provider.take(fixture);
    const stale_copy = stale;
    try maplibre.resourceRequestRelease(stale);
    try testing.expectError(error.InvalidState, maplibre.resourceRequestIsCancelled(stale_copy, null));

    const live = try provider.take(fixture);
    defer maplibre.resourceRequestRelease(live) catch {};
    try testing.expectError(error.InvalidState, maplibre.resourceRequestComplete(testing.allocator, stale_copy, .{ .bytes = support.style_json }, null));
    try maplibre.resourceRequestComplete(testing.allocator, live, .{ .bytes = support.style_json }, null);
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
}

const CancelProbe = struct {
    cancels: support.Counter = .{},
    releases: support.Counter = .{},

    fn cancelled(context: ?*anyopaque) maplibre.Error!void {
        const self: *CancelProbe = @ptrCast(@alignCast(context.?));
        self.cancels.add();
    }

    fn released(context: ?*anyopaque) void {
        const self: *CancelProbe = @ptrCast(@alignCast(context.?));
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
    const handle = try provider.take(fixture);
    try fixture.closeMap();

    // Cancellation lands on a MapLibre thread with nothing registered to
    // report it, so the wait checks the request again after each wake.
    try fixture.events.waitUntil(handle, struct {
        fn ready(request: maplibre.ResourceRequestHandle) anyerror!bool {
            return maplibre.resourceRequestIsCancelled(request, null);
        }
    }.ready);

    var probe = CancelProbe{};
    const live = maplibre.testing.liveCallbackRegistrations();
    try testing.expect(try maplibre.resourceRequestSetCancelCallback(testing.allocator, handle, .{
        .callback = CancelProbe.cancelled,
        .context = &probe,
        .release_context = CancelProbe.released,
    }, null));
    try testing.expectEqual(live, maplibre.testing.liveCallbackRegistrations());
    try maplibre.resourceRequestRelease(handle);
    try fixture.releaseRuntimeWhenChildless();
    try testing.expectEqual(@as(usize, 0), probe.cancels.get());
    try testing.expectEqual(@as(usize, 0), probe.releases.get());
}

fn passThrough(_: ?*anyopaque, _: maplibre.ResourceRequest, _: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
    return .pass_through;
}

const TransformProbe = struct {
    calls: std.atomic.Value(usize) = .init(0),
    saved: ?maplibre.ResourceTransformResponse = null,
    other_thread: ?anyerror = null,

    fn setUrlOnThread(response: maplibre.ResourceTransformResponse, result: *?anyerror) void {
        result.* = if (maplibre.resourceTransformResponseSetUrl(response, "unsupported://elsewhere.json", null)) null else |err| err;
    }

    fn transform(context: ?*anyopaque, kind: maplibre.ResourceKind, _: []const u8, response: maplibre.ResourceTransformResponse) maplibre.Error!void {
        const self: *TransformProbe = @ptrCast(@alignCast(context.?));
        if (kind != .style or self.calls.load(.acquire) != 0) return;
        const thread = std.Thread.spawn(.{}, setUrlOnThread, .{ response, &self.other_thread }) catch return error.OutOfMemory;
        thread.join();
        try maplibre.resourceTransformResponseSetUrl(response, "unsupported://rewritten.json", null);
        self.saved = response;
        _ = self.calls.fetchAdd(1, .release);
    }
};

// A transform's response object is valid only on the callback's thread while
// the callback runs. The transform runs in the native network loader, after
// the provider passes a request through. No loader serves either the original
// or the rewritten scheme, so the style fails to load without a fetch.
test "a scoped response rejects use after its callback and from another thread" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.setProvider(.{ .callback = passThrough });
    var probe = TransformProbe{};
    try support.resolve(try maplibre.runtimeSetResourceTransform(testing.allocator, fixture.runtime, .{ .callback = TransformProbe.transform, .context = &probe }, null));

    try support.expectCommitted(try maplibre.mapSetStyleUrl(testing.allocator, fixture.map, "unsupported://original.json", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.acquire));

    try testing.expectEqual(@as(?anyerror, error.InvalidState), probe.other_thread);
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.resourceTransformResponseSetUrl(probe.saved.?, "unsupported://late.json", &diagnostic));
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
}

// A camera command carries its end handler two records deep in its input.
// Native runs the handler once and releases it when the command's identity is
// cancelled. A command that native rejects leaves the context with the
// caller, so neither callback runs.
test "a camera end handler inside a command is released after it runs" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var ended = support.EndProbe{};
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, .{
        .mode = .ease,
        .camera = .{ .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000, .transition_id = 3, .end_handler = ended.handler() },
    }, null));
    try testing.expectEqual(@as(usize, 0), ended.releases.get());
    try support.expectCommitted(try maplibre.mapCancelCameraTransition(fixture.map, 3, null));
    try testing.expectEqual(@as(u32, 1), ended.ends.load(.acquire));
    try testing.expectEqual(maplibre.CameraTransitionOutcome.cancelled, ended.outcome);
    try ended.releases.waitFor(1);

    var rejected = support.EndProbe{};
    try testing.expectError(error.InvalidArgument, maplibre.mapApplyCameraDelta(testing.allocator, fixture.map, .{
        .scale = -1,
        .animation = .{ .end_handler = rejected.handler() },
    }, null));
    try fixture.barrier();
    try testing.expectEqual(@as(usize, 0), rejected.releases.get());
    try testing.expectEqual(@as(u32, 0), rejected.ends.load(.acquire));
}
