const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

const HttpServerState = struct {
    server: *std.Io.net.Server,
    served: bool = false,
    err: ?anyerror = null,
};

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (std.meta.eql(owned_event.event_type, event_type)) return true;
        }
        try sleepOneMillisecond();
    }
    return false;
}

fn waitForOwnedEvent(
    runtime: *maplibre.RuntimeHandle,
    event_type: maplibre.RuntimeEventType,
) !maplibre.OwnedRuntimeEvent {
    for (0..5000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            if (std.meta.eql(owned_event.event_type, event_type)) return owned_event;
            owned_event.deinit();
        }
        try sleepOneMillisecond();
    }
    return error.EventNotObserved;
}

fn waitForStyleLoaded(runtime: *maplibre.RuntimeHandle) !void {
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
}

const TempStyle = struct {
    tmp: std.testing.TmpDir,
    dir_path: []const u8,
    style_url: []const u8,

    fn deinit(self: *TempStyle) void {
        testing.allocator.free(self.style_url);
        testing.allocator.free(self.dir_path);
        self.tmp.cleanup();
    }
};

fn tempDirPath(allocator: std.mem.Allocator, sub_path: []const u8) ![]u8 {
    const cwd = try std.process.currentPathAlloc(testing.io, allocator);
    defer allocator.free(cwd);
    return std.fmt.allocPrint(allocator, "{s}/.zig-cache/tmp/{s}", .{ cwd, sub_path });
}

fn tempPath(allocator: std.mem.Allocator, sub_path: []const u8, relative_path: []const u8) ![]u8 {
    const dir_path = try tempDirPath(allocator, sub_path);
    defer allocator.free(dir_path);
    return std.fs.path.join(allocator, &.{ dir_path, relative_path });
}

fn writeTempStyle() !TempStyle {
    var tmp = testing.tmpDir(.{});
    errdefer tmp.cleanup();
    try tmp.dir.writeFile(testing.io, .{ .sub_path = "style.json", .data = support.style_json });

    const dir_path = try tempDirPath(testing.allocator, tmp.sub_path[0..]);
    errdefer testing.allocator.free(dir_path);
    const style_path = try tempPath(testing.allocator, tmp.sub_path[0..], "style.json");
    defer testing.allocator.free(style_path);
    const style_url = try std.fmt.allocPrint(testing.allocator, "file://{s}", .{style_path});
    errdefer testing.allocator.free(style_url);

    return .{ .tmp = tmp, .dir_path = dir_path, .style_url = style_url };
}

test "network status APIs wrap process-global MapLibre status" {
    const original_status = try maplibre.getNetworkStatus(null);
    defer maplibre.setNetworkStatus(original_status, null) catch @panic("network status restore failed");

    try maplibre.setNetworkStatus(.offline, null);
    try testing.expect(std.meta.eql(try maplibre.getNetworkStatus(null), maplibre.NetworkStatus.offline));

    try maplibre.setNetworkStatus(.online, null);
    try testing.expect(std.meta.eql(try maplibre.getNetworkStatus(null), maplibre.NetworkStatus.online));

    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();
    try testing.expectError(error.InvalidArgument, maplibre.setNetworkStatus(.{ .unknown = 999 }, &diagnostics));
    try testing.expect(diagnostics.get().?.message.len > 0);
}

test "ambient cache operations validate cache configuration" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    try runtime.runAmbientCacheOperation(.pack_database);
    try runtime.close();

    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "ambient-cache.db");
    defer testing.allocator.free(cache_path);

    var cached_runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
    defer cached_runtime.close() catch @panic("cached runtime close failed");
    try cached_runtime.runAmbientCacheOperation(.reset_database);
    try cached_runtime.runAmbientCacheOperation(.pack_database);
    try cached_runtime.runAmbientCacheOperation(.invalidate);
    try cached_runtime.runAmbientCacheOperation(.clear);
}

test "file URL style loads through public binding" {
    var fixture = try writeTempStyle();
    defer fixture.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, fixture.style_url);
    try waitForStyleLoaded(&runtime);
}

test "asset URL style loads through public binding runtime asset path" {
    var fixture = try writeTempStyle();
    defer fixture.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .asset_path = fixture.dir_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "asset://style.json");
    try waitForStyleLoaded(&runtime);
}

test "missing file URL reports map loading failure through public events" {
    var fixture = try writeTempStyle();
    defer fixture.deinit();

    const missing_path = try std.fs.path.join(testing.allocator, &.{ fixture.dir_path, "missing-style.json" });
    defer testing.allocator.free(missing_path);
    const missing_url = try std.fmt.allocPrint(testing.allocator, "file://{s}", .{missing_path});
    defer testing.allocator.free(missing_url);

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, missing_url);
    try testing.expect(try waitForEvent(&runtime, .map_loading_failed));
}

const pmtiles_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-pmtiles-range-test",
    \\  "sources": {
    \\    "archive": {
    \\      "type": "vector",
    \\      "url": "pmtiles://http://example.invalid/test.pmtiles"
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"archive-fill","type":"fill","source":"archive","source-layer":"land","paint":{"fill-color":"#8dd3c7"}}
    \\  ]
    \\}
;

const pmtiles_style_url = "custom://pmtiles-range-style.json";
const pmtiles_archive_url = "http://example.invalid/test.pmtiles";

const TransformState = struct {
    replacement_url: [:0]const u8,
    calls: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
};

fn rewriteStyleUrl(context: ?*anyopaque, request: maplibre.ResourceTransformRequest) maplibre.ResourceTransformResponse {
    const state: *TransformState = @ptrCast(@alignCast(context.?));
    _ = state.calls.fetchAdd(1, .seq_cst);
    _ = request.kind;
    _ = request.url;
    return .{ .replacement_url = state.replacement_url };
}

fn serveOneHttpStyleInner(state: *HttpServerState) !void {
    var stream = try state.server.accept(testing.io);
    defer stream.close(testing.io);

    var request_buffer: [1024]u8 = undefined;
    var reader = stream.reader(testing.io, &request_buffer);
    _ = try reader.interface.discardDelimiterInclusive('\n');

    var header_buffer: [256]u8 = undefined;
    const header = try std.fmt.bufPrint(
        &header_buffer,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nCache-Control: public, max-age=3600\r\nETag: \"zig-style\"\r\nContent-Length: {d}\r\nConnection: close\r\n\r\n",
        .{support.style_json.len},
    );
    var response_buffer: [1024]u8 = undefined;
    var writer = stream.writer(testing.io, &response_buffer);
    try writer.interface.writeAll(header);
    try writer.interface.writeAll(support.style_json);
    try writer.interface.flush();
    state.served = true;
}

fn serveOneHttpStyle(state: *HttpServerState) void {
    serveOneHttpStyleInner(state) catch |err| {
        state.err = err;
    };
}

test "resource transform can be cleared after map creation" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = TransformState{ .replacement_url = "unsupported://rewritten-style.json" };
    try runtime.setResourceTransform(.{ .handler = rewriteStyleUrl, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");
    try runtime.setResourceTransform(null);

    var address = try std.Io.net.IpAddress.parse("127.0.0.1", 0);
    var server = try address.listen(testing.io, .{ .reuse_address = true });
    var server_state = HttpServerState{ .server = &server };
    const server_thread = try std.Thread.spawn(.{}, serveOneHttpStyle, .{&server_state});
    var server_thread_joined = false;
    defer {
        server.deinit(testing.io);
        if (!server_thread_joined) server_thread.join();
    }
    const style_url = try std.fmt.allocPrint(testing.allocator, "http://127.0.0.1:{d}/style.json", .{server.socket.address.getPort()});
    defer testing.allocator.free(style_url);

    try map.setStyleUrl(testing.allocator, style_url);
    try waitForStyleLoaded(&runtime);
    server_thread.join();
    server_thread_joined = true;
    try testing.expect(server_state.served);
    try testing.expectEqual(@as(?anyerror, null), server_state.err);
    try testing.expectEqual(@as(usize, 0), state.calls.load(.seq_cst));
}

test "http URL style loads through native network provider" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    var address = try std.Io.net.IpAddress.parse("127.0.0.1", 0);
    var server = try address.listen(testing.io, .{ .reuse_address = true });
    var server_state = HttpServerState{ .server = &server };
    const server_thread = try std.Thread.spawn(.{}, serveOneHttpStyle, .{&server_state});
    var server_thread_joined = false;
    defer {
        server.deinit(testing.io);
        if (!server_thread_joined) server_thread.join();
    }
    const style_url = try std.fmt.allocPrint(testing.allocator, "http://127.0.0.1:{d}/style.json", .{server.socket.address.getPort()});
    defer testing.allocator.free(style_url);

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, style_url);
    try waitForStyleLoaded(&runtime);
    server_thread.join();
    server_thread_joined = true;
    try testing.expect(server_state.served);
    try testing.expectEqual(@as(?anyerror, null), server_state.err);
}

const PassThroughProviderState = struct {
    calls: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    saw_style: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
};

fn passThroughStyleProvider(
    context: ?*anyopaque,
    request: maplibre.ResourceRequest,
    maybe_handle: ?maplibre.ResourceRequestHandle,
) maplibre.ResourceProviderDecision {
    const state: *PassThroughProviderState = @ptrCast(@alignCast(context.?));
    _ = state.calls.fetchAdd(1, .seq_cst);
    if (std.meta.eql(request.kind, maplibre.ResourceKind.style)) state.saw_style.store(true, .seq_cst);
    _ = maybe_handle;
    return .pass_through;
}

test "http style can load from ambient cache after online load" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "http-cache.db");
    defer testing.allocator.free(cache_path);

    var address = try std.Io.net.IpAddress.parse("127.0.0.1", 0);
    var server = try address.listen(testing.io, .{ .reuse_address = true });
    var server_state = HttpServerState{ .server = &server };
    const server_thread = try std.Thread.spawn(.{}, serveOneHttpStyle, .{&server_state});
    var server_thread_joined = false;
    defer {
        server.deinit(testing.io);
        if (!server_thread_joined) server_thread.join();
    }
    const style_url = try std.fmt.allocPrint(testing.allocator, "http://127.0.0.1:{d}/style.json", .{server.socket.address.getPort()});
    defer testing.allocator.free(style_url);

    {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path, .maximum_cache_size = 1024 * 1024 }, null);
        defer runtime.close() catch @panic("runtime close failed");
        var map = try maplibre.MapHandle.create(&runtime, .{});
        defer map.close() catch @panic("map close failed");
        try map.setStyleUrl(testing.allocator, style_url);
        try waitForStyleLoaded(&runtime);
        try runtime.runAmbientCacheOperation(.pack_database);
    }

    server_thread.join();
    server_thread_joined = true;
    try testing.expect(server_state.served);
    try testing.expectEqual(@as(?anyerror, null), server_state.err);

    try maplibre.setNetworkStatus(.offline, null);
    var cached_runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
    defer cached_runtime.close() catch @panic("cached runtime close failed");
    var cached_map = try maplibre.MapHandle.create(&cached_runtime, .{});
    defer cached_map.close() catch @panic("cached map close failed");
    try cached_map.setStyleUrl(testing.allocator, style_url);
    try waitForStyleLoaded(&cached_runtime);
}

test "resource provider pass-through delegates to native HTTP" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    var address = try std.Io.net.IpAddress.parse("127.0.0.1", 0);
    var server = try address.listen(testing.io, .{ .reuse_address = true });
    var server_state = HttpServerState{ .server = &server };
    const server_thread = try std.Thread.spawn(.{}, serveOneHttpStyle, .{&server_state});
    var server_thread_joined = false;
    defer {
        server.deinit(testing.io);
        if (!server_thread_joined) server_thread.join();
    }
    const style_url = try std.fmt.allocPrint(testing.allocator, "http://127.0.0.1:{d}/style.json", .{server.socket.address.getPort()});
    defer testing.allocator.free(style_url);

    var state = PassThroughProviderState{};
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    try runtime.setResourceProvider(.{ .handler = passThroughStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, style_url);
    try waitForStyleLoaded(&runtime);
    server_thread.join();
    server_thread_joined = true;
    try testing.expect(server_state.served);
    try testing.expectEqual(@as(?anyerror, null), server_state.err);
    try testing.expect(state.calls.load(.seq_cst) > 0);
    try testing.expect(state.saw_style.load(.seq_cst));
}

test "resource transform rewrites network style URL" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const original_url = "http://example.invalid/original-style.json";
    var state = TransformState{
        .replacement_url = "unsupported://rewritten-style.json",
    };
    var replacement_state = TransformState{
        .replacement_url = "unsupported://unexpected-replacement.json",
    };
    try runtime.setResourceTransform(.{ .handler = rewriteStyleUrl, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try runtime.setResourceTransform(.{ .handler = rewriteStyleUrl, .context = &replacement_state });

    try map.setStyleUrl(testing.allocator, original_url);
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            owned_event.deinit();
        }
        if (replacement_state.calls.load(.seq_cst) > 0) break;
        try sleepOneMillisecond();
    }

    try testing.expectEqual(@as(usize, 0), state.calls.load(.seq_cst));
    try testing.expect(replacement_state.calls.load(.seq_cst) > 0);
}

const ProviderState = struct {
    calls: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    completions: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    saw_cancelled_query: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_second_complete_error: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_after_release_error: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_style: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_all_loading: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_regular_priority: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_online_usage: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_permanent_storage: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_no_range: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
};

fn customStyleProvider(
    context: ?*anyopaque,
    request: maplibre.ResourceRequest,
    maybe_handle: ?maplibre.ResourceRequestHandle,
) maplibre.ResourceProviderDecision {
    const state: *ProviderState = @ptrCast(@alignCast(context.?));
    _ = state.calls.fetchAdd(1, .seq_cst);
    if (!std.mem.eql(u8, request.url, "custom://style.json")) return .pass_through;

    state.saw_style.store(std.meta.eql(request.kind, maplibre.ResourceKind.style), .seq_cst);
    state.saw_all_loading.store(std.meta.eql(request.loading_method, maplibre.ResourceLoadingMethod.all), .seq_cst);
    state.saw_regular_priority.store(std.meta.eql(request.priority, maplibre.ResourcePriority.regular), .seq_cst);
    state.saw_online_usage.store(std.meta.eql(request.usage, maplibre.ResourceUsage.online), .seq_cst);
    state.saw_permanent_storage.store(std.meta.eql(request.storage_policy, maplibre.ResourceStoragePolicy.permanent), .seq_cst);
    state.saw_no_range.store(request.range == null, .seq_cst);

    const handle = maybe_handle orelse return .pass_through;
    const is_cancelled = handle.cancelled() catch true;
    state.saw_cancelled_query.store(!is_cancelled, .seq_cst);
    handle.complete(.{ .bytes = support.style_json }) catch {
        handle.release();
        return .pass_through;
    };
    handle.complete(.{ .bytes = support.style_json }) catch |err| {
        if (err == error.AlreadyCompleted) state.saw_second_complete_error.store(true, .seq_cst);
    };
    handle.release();
    handle.release();
    _ = handle.cancelled() catch |err| {
        if (err == error.ClosedHandle) state.saw_after_release_error.store(true, .seq_cst);
    };
    _ = state.completions.fetchAdd(1, .seq_cst);
    return .handle;
}

const PmtilesRangeProviderState = struct {
    saw_style_absent_range: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    recorded_pmtiles_request: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_source_kind: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_network_only_loading: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    range_start: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
    range_end: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),

    fn markStyle(self: *PmtilesRangeProviderState, request: maplibre.ResourceRequest) void {
        self.saw_style_absent_range.store(request.range == null, .seq_cst);
    }

    fn markPmtilesRequest(self: *PmtilesRangeProviderState, request: maplibre.ResourceRequest) void {
        self.saw_source_kind.store(std.meta.eql(request.kind, maplibre.ResourceKind.source), .seq_cst);
        self.saw_network_only_loading.store(std.meta.eql(request.loading_method, maplibre.ResourceLoadingMethod.network_only), .seq_cst);
        if (request.range) |range| {
            self.range_start.store(range.start, .seq_cst);
            self.range_end.store(range.end, .seq_cst);
        }
        self.recorded_pmtiles_request.store(true, .seq_cst);
    }

    fn expectObservedRequest(self: *PmtilesRangeProviderState) !void {
        const start = self.range_start.load(.seq_cst);
        const end = self.range_end.load(.seq_cst);
        try testing.expect(self.saw_style_absent_range.load(.seq_cst));
        try testing.expect(self.recorded_pmtiles_request.load(.seq_cst));
        try testing.expect(self.saw_source_kind.load(.seq_cst));
        try testing.expect(self.saw_network_only_loading.load(.seq_cst));
        try testing.expectEqual(@as(u64, 0), start);
        try testing.expect(end >= start);
        try testing.expect(end - start + 1 > 0);
    }
};

fn pmtilesRangeProvider(
    context: ?*anyopaque,
    request: maplibre.ResourceRequest,
    maybe_handle: ?maplibre.ResourceRequestHandle,
) maplibre.ResourceProviderDecision {
    const state: *PmtilesRangeProviderState = @ptrCast(@alignCast(context.?));
    const handle = maybe_handle orelse return .pass_through;

    if (std.mem.eql(u8, request.url, pmtiles_style_url)) {
        state.markStyle(request);
        handle.complete(.{ .bytes = pmtiles_style_json }) catch {
            handle.release();
            return .pass_through;
        };
        handle.release();
        return .handle;
    }

    if (std.mem.eql(u8, request.url, pmtiles_archive_url)) {
        state.markPmtilesRequest(request);
        handle.complete(.{
            .status = .@"error",
            .error_reason = .not_found,
            .error_message = "pmtiles archive intentionally unavailable",
        }) catch {
            handle.release();
            return .pass_through;
        };
        handle.release();
        return .handle;
    }

    return .pass_through;
}

fn waitForPmtilesRangeRequest(runtime: *maplibre.RuntimeHandle, state: *PmtilesRangeProviderState) !void {
    for (0..1000) |_| {
        try runtime.runOnce();
        if (state.recorded_pmtiles_request.load(.seq_cst)) return;
        try sleepOneMillisecond();
    }
    return error.ProviderNotCalled;
}

test "resource provider observes PMTiles range metadata" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = PmtilesRangeProviderState{};
    try runtime.setResourceProvider(.{ .handler = pmtilesRangeProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, pmtiles_style_url);
    try waitForPmtilesRangeRequest(&runtime, &state);
    try state.expectObservedRequest();
}

test "custom URL style loads through resource provider" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = ProviderState{};
    var replacement_state = ProviderState{};
    try runtime.setResourceProvider(.{ .handler = customStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");
    try testing.expectError(error.InvalidState, runtime.setResourceProvider(.{ .handler = customStyleProvider, .context = &replacement_state }));

    try map.setStyleUrl(testing.allocator, "custom://style.json");
    try waitForStyleLoaded(&runtime);
    try testing.expect(state.calls.load(.seq_cst) > 0);
    try testing.expectEqual(@as(usize, 1), state.completions.load(.seq_cst));
    try testing.expectEqual(@as(usize, 0), replacement_state.calls.load(.seq_cst));
    try testing.expect(state.saw_cancelled_query.load(.seq_cst));
    try testing.expect(state.saw_second_complete_error.load(.seq_cst));
    try testing.expect(state.saw_after_release_error.load(.seq_cst));
    try testing.expect(state.saw_style.load(.seq_cst));
    try testing.expect(state.saw_all_loading.load(.seq_cst));
    try testing.expect(state.saw_regular_priority.load(.seq_cst));
    try testing.expect(state.saw_online_usage.load(.seq_cst));
    try testing.expect(state.saw_permanent_storage.load(.seq_cst));
    try testing.expect(state.saw_no_range.load(.seq_cst));
}

const offline_style_url = "http://example.com/offline-style.json";

fn offlineTileDefinition() maplibre.OfflineRegionDefinition {
    return .{ .tile_pyramid = .{
        .style_url = offline_style_url,
        .bounds = .{
            .southwest = .{ .latitude = 1.0, .longitude = 2.0 },
            .northeast = .{ .latitude = 3.0, .longitude = 4.0 },
        },
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
    } };
}

fn expectOfflineTileRegion(region: *const maplibre.OwnedOfflineRegion, expected_metadata: []const u8) !void {
    try testing.expect(region.id > 0);
    const definition = region.definition.tile_pyramid;
    try testing.expectEqualStrings(offline_style_url, definition.style_url);
    try testing.expectEqual(@as(f64, 1.0), definition.bounds.southwest.latitude);
    try testing.expectEqual(@as(f64, 2.0), definition.bounds.southwest.longitude);
    try testing.expectEqual(@as(f64, 3.0), definition.bounds.northeast.latitude);
    try testing.expectEqual(@as(f64, 4.0), definition.bounds.northeast.longitude);
    try testing.expectEqual(@as(f64, 5.0), definition.min_zoom);
    try testing.expectEqual(@as(f64, 6.0), definition.max_zoom);
    try testing.expectEqual(@as(f32, 2.0), definition.pixel_ratio);
    try testing.expect(definition.include_ideographs);
    try testing.expectEqualSlices(u8, expected_metadata, region.metadata);
}

test "offline tile-pyramid regions copy definitions and metadata" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "cache.db");
    defer testing.allocator.free(cache_path);

    const metadata = [_]u8{ 1, 2, 3 };
    const updated_metadata = [_]u8{ 4, 5, 6, 7 };
    var region_id: maplibre.OfflineRegionId = 0;

    {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
        defer runtime.close() catch @panic("runtime close failed");

        var created = try runtime.createOfflineRegion(testing.allocator, offlineTileDefinition(), metadata[0..]);
        defer created.deinit();
        region_id = created.id;
        try expectOfflineTileRegion(&created, metadata[0..]);

        const status = try runtime.getOfflineRegionStatus(region_id);
        try testing.expect(std.meta.eql(status.download_state, maplibre.OfflineRegionDownloadState.inactive));

        var list = try runtime.listOfflineRegions(testing.allocator);
        defer list.deinit();
        try testing.expectEqual(@as(usize, 1), list.items.len);
        try expectOfflineTileRegion(&list.items[0], metadata[0..]);

        var updated = try runtime.updateOfflineRegionMetadata(testing.allocator, region_id, updated_metadata[0..]);
        defer updated.deinit();
        try expectOfflineTileRegion(&updated, updated_metadata[0..]);
    }

    {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
        defer runtime.close() catch @panic("runtime close failed");

        var reloaded = (try runtime.getOfflineRegion(testing.allocator, region_id)) orelse return error.RegionReloadFailed;
        defer reloaded.deinit();
        try expectOfflineTileRegion(&reloaded, updated_metadata[0..]);

        try runtime.invalidateOfflineRegion(region_id);
        try runtime.deleteOfflineRegion(region_id);

        const missing = try runtime.getOfflineRegion(testing.allocator, region_id);
        try testing.expect(missing == null);

        var list = try runtime.listOfflineRegions(testing.allocator);
        defer list.deinit();
        try testing.expectEqual(@as(usize, 0), list.items.len);
    }
}

test "offline region definitions reject invalid public values" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "invalid-offline-cache.db");
    defer testing.allocator.free(cache_path);

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    var invalid_zoom = offlineTileDefinition();
    invalid_zoom.tile_pyramid.min_zoom = 8.0;
    invalid_zoom.tile_pyramid.max_zoom = 2.0;
    try testing.expectError(error.InvalidArgument, runtime.createOfflineRegion(testing.allocator, invalid_zoom, &.{}));

    var invalid_bounds = offlineTileDefinition();
    invalid_bounds.tile_pyramid.bounds.southwest.latitude = std.math.inf(f64);
    try testing.expectError(error.InvalidArgument, runtime.createOfflineRegion(testing.allocator, invalid_bounds, &.{}));

    try testing.expectError(error.InvalidArgument, runtime.createOfflineRegion(testing.allocator, .{ .geometry = .{
        .style_url = offline_style_url,
        .geometry = .empty,
        .min_zoom = 5.0,
        .max_zoom = 6.0,
    } }, &.{}));

    var nested_geometries: [66]maplibre.Geometry = undefined;
    nested_geometries[nested_geometries.len - 1] = .{ .point = .{ .latitude = 1.0, .longitude = 2.0 } };
    var nested_index = nested_geometries.len - 1;
    while (nested_index > 0) {
        nested_index -= 1;
        nested_geometries[nested_index] = .{ .collection = nested_geometries[nested_index + 1 .. nested_index + 2] };
    }
    try testing.expectError(error.InvalidArgument, runtime.createOfflineRegion(testing.allocator, .{ .geometry = .{
        .style_url = offline_style_url,
        .geometry = nested_geometries[0],
        .min_zoom = 5.0,
        .max_zoom = 6.0,
    } }, &.{}));
}

fn expectOfflineGeometryRegion(region: *const maplibre.OwnedOfflineRegion, expected_metadata: []const u8) !void {
    try testing.expect(region.id > 0);
    const definition = region.definition.geometry;
    try testing.expectEqualStrings(offline_style_url, definition.style_url);
    try testing.expectEqual(@as(f64, 5.0), definition.min_zoom);
    try testing.expectEqual(@as(f64, 6.0), definition.max_zoom);
    try testing.expectEqual(@as(f32, 2.0), definition.pixel_ratio);
    try testing.expect(definition.include_ideographs);
    const copied_line = definition.geometry.line_string;
    try testing.expectEqual(@as(usize, 2), copied_line.len);
    try testing.expectEqual(@as(f64, 1.0), copied_line[0].latitude);
    try testing.expectEqual(@as(f64, 2.0), copied_line[0].longitude);
    try testing.expectEqual(@as(f64, 3.0), copied_line[1].latitude);
    try testing.expectEqual(@as(f64, 4.0), copied_line[1].longitude);
    try testing.expectEqualSlices(u8, expected_metadata, region.metadata);
}

test "offline database merge returns copied region list" {
    var main_tmp = testing.tmpDir(.{});
    defer main_tmp.cleanup();
    var side_tmp = testing.tmpDir(.{});
    defer side_tmp.cleanup();
    const main_cache_path = try tempPath(testing.allocator, main_tmp.sub_path[0..], "cache.db");
    defer testing.allocator.free(main_cache_path);
    const side_cache_path = try tempPath(testing.allocator, side_tmp.sub_path[0..], "cache.db");
    defer testing.allocator.free(side_cache_path);

    const metadata = [_]u8{ 5, 4, 3 };
    {
        var side_runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = side_cache_path }, null);
        defer side_runtime.close() catch @panic("side runtime close failed");
        var created = try side_runtime.createOfflineRegion(testing.allocator, offlineTileDefinition(), metadata[0..]);
        defer created.deinit();
    }

    var main_runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = main_cache_path }, null);
    defer main_runtime.close() catch @panic("main runtime close failed");
    var merged = try main_runtime.mergeOfflineRegionsDatabase(testing.allocator, side_cache_path);
    defer merged.deinit();
    try testing.expectEqual(@as(usize, 1), merged.items.len);
    try expectOfflineTileRegion(&merged.items[0], metadata[0..]);
}

test "offline geometry regions expose copied geometry values" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "geometry-cache.db");
    defer testing.allocator.free(cache_path);

    const coordinates = [_]maplibre.LatLng{
        .{ .latitude = 1.0, .longitude = 2.0 },
        .{ .latitude = 3.0, .longitude = 4.0 },
    };
    const metadata = [_]u8{ 7, 8, 9 };
    var region_id: maplibre.OfflineRegionId = 0;

    {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
        defer runtime.close() catch @panic("runtime close failed");

        var created = try runtime.createOfflineRegion(testing.allocator, .{ .geometry = .{
            .style_url = offline_style_url,
            .geometry = .{ .line_string = coordinates[0..] },
            .min_zoom = 5.0,
            .max_zoom = 6.0,
            .pixel_ratio = 2.0,
            .include_ideographs = true,
        } }, metadata[0..]);
        defer created.deinit();
        region_id = created.id;
        try expectOfflineGeometryRegion(&created, metadata[0..]);

        var list = try runtime.listOfflineRegions(testing.allocator);
        defer list.deinit();
        try testing.expectEqual(@as(usize, 1), list.items.len);
        try expectOfflineGeometryRegion(&list.items[0], metadata[0..]);
    }

    {
        var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
        defer runtime.close() catch @panic("runtime close failed");

        var reloaded = (try runtime.getOfflineRegion(testing.allocator, region_id)) orelse return error.RegionReloadFailed;
        defer reloaded.deinit();
        try expectOfflineGeometryRegion(&reloaded, metadata[0..]);

        try runtime.deleteOfflineRegion(region_id);
        const missing = try runtime.getOfflineRegion(testing.allocator, region_id);
        try testing.expect(missing == null);
    }
}

const AsyncProviderState = struct {
    handle_lock: std.atomic.Mutex = .unlocked,
    handle: ?maplibre.ResourceRequestHandle = null,
    saw_style: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_all_loading: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_regular_priority: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_online_usage: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_permanent_storage: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_no_range: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    saw_no_prior: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),

    fn store(self: *AsyncProviderState, request: maplibre.ResourceRequest, handle: maplibre.ResourceRequestHandle) void {
        self.saw_style.store(std.meta.eql(request.kind, maplibre.ResourceKind.style), .seq_cst);
        self.saw_all_loading.store(std.meta.eql(request.loading_method, maplibre.ResourceLoadingMethod.all), .seq_cst);
        self.saw_regular_priority.store(std.meta.eql(request.priority, maplibre.ResourcePriority.regular), .seq_cst);
        self.saw_online_usage.store(std.meta.eql(request.usage, maplibre.ResourceUsage.online), .seq_cst);
        self.saw_permanent_storage.store(std.meta.eql(request.storage_policy, maplibre.ResourceStoragePolicy.permanent), .seq_cst);
        self.saw_no_range.store(request.range == null, .seq_cst);
        self.saw_no_prior.store(request.prior_modified_unix_ms == null and request.prior_expires_unix_ms == null and
            request.prior_etag == null and request.prior_data.len == 0, .seq_cst);
        self.lockHandle();
        defer self.unlockHandle();
        self.handle = handle;
    }

    fn takeHandle(self: *AsyncProviderState) ?maplibre.ResourceRequestHandle {
        self.lockHandle();
        defer self.unlockHandle();
        const handle = self.handle;
        self.handle = null;
        return handle;
    }

    fn lockHandle(self: *AsyncProviderState) void {
        while (!self.handle_lock.tryLock()) {
            std.Thread.yield() catch {};
        }
    }

    fn unlockHandle(self: *AsyncProviderState) void {
        self.handle_lock.unlock();
    }

    fn expectObservedRequest(self: *AsyncProviderState) !void {
        try testing.expect(self.saw_style.load(.seq_cst));
        try testing.expect(self.saw_all_loading.load(.seq_cst));
        try testing.expect(self.saw_regular_priority.load(.seq_cst));
        try testing.expect(self.saw_online_usage.load(.seq_cst));
        try testing.expect(self.saw_permanent_storage.load(.seq_cst));
        try testing.expect(self.saw_no_range.load(.seq_cst));
        try testing.expect(self.saw_no_prior.load(.seq_cst));
    }
};

fn delayedStyleProvider(
    context: ?*anyopaque,
    request: maplibre.ResourceRequest,
    maybe_handle: ?maplibre.ResourceRequestHandle,
) maplibre.ResourceProviderDecision {
    if (!std.mem.startsWith(u8, request.url, "custom://delayed-style")) return .pass_through;
    const handle = maybe_handle orelse return .pass_through;
    const state: *AsyncProviderState = @ptrCast(@alignCast(context.?));
    state.store(request, handle);
    return .handle;
}

fn waitForProviderHandle(runtime: *maplibre.RuntimeHandle, state: *AsyncProviderState) !maplibre.ResourceRequestHandle {
    for (0..1000) |_| {
        try runtime.runOnce();
        if (state.takeHandle()) |handle| return handle;
        try sleepOneMillisecond();
    }
    return error.ProviderNotCalled;
}

fn completeStyleOnThread(handle: maplibre.ResourceRequestHandle, out_error: *?anyerror) void {
    handle.complete(.{ .bytes = support.style_json }) catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "resource provider can complete style request later" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(&runtime, &state);
    defer handle.release();

    try state.expectObservedRequest();
    try testing.expect(!try handle.cancelled());

    try handle.complete(.{ .bytes = support.style_json });
    try testing.expectError(error.AlreadyCompleted, handle.complete(.{ .bytes = support.style_json }));
    try waitForStyleLoaded(&runtime);
}

test "released resource request handle copies stay closed after later requests" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const stale_handle = try waitForProviderHandle(&runtime, &state);
    stale_handle.release();
    try testing.expectError(error.ClosedHandle, stale_handle.cancelled());

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const live_handle = try waitForProviderHandle(&runtime, &state);
    defer live_handle.release();
    try testing.expectError(error.ClosedHandle, stale_handle.complete(.{ .bytes = support.style_json }));
    try testing.expect(!try live_handle.cancelled());
    try live_handle.complete(.{ .bytes = support.style_json });
    try waitForStyleLoaded(&runtime);
}

test "resource request handles stay usable across many handled requests" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    for (0..16) |request_index| {
        const style_url = try std.fmt.allocPrint(testing.allocator, "custom://delayed-style-{d}.json", .{request_index});
        defer testing.allocator.free(style_url);
        try map.setStyleUrl(testing.allocator, style_url);

        const handle = try waitForProviderHandle(&runtime, &state);
        try state.expectObservedRequest();
        try testing.expect(!try handle.cancelled());
        try handle.complete(.{ .bytes = support.style_json });
        handle.release();
        try waitForStyleLoaded(&runtime);
    }
}

test "resource provider can complete request from another thread" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(&runtime, &state);
    defer handle.release();

    var completion_error: ?anyerror = error.NativeError;
    const thread = try std.Thread.spawn(.{}, completeStyleOnThread, .{ handle, &completion_error });
    thread.join();
    try testing.expect(completion_error == null);
    try waitForStyleLoaded(&runtime);
}

fn errorStyleProvider(
    _: ?*anyopaque,
    request: maplibre.ResourceRequest,
    maybe_handle: ?maplibre.ResourceRequestHandle,
) maplibre.ResourceProviderDecision {
    if (!std.mem.eql(u8, request.url, "custom://error-style.json")) return .pass_through;
    const handle = maybe_handle orelse return .pass_through;
    handle.complete(.{
        .status = .@"error",
        .error_reason = .not_found,
        .error_message = "custom style failed",
    }) catch {
        handle.release();
        return .pass_through;
    };
    handle.release();
    return .handle;
}

test "resource provider error response fails style load" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.setResourceProvider(.{ .handler = errorStyleProvider });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://error-style.json");
    try testing.expect(try waitForEvent(&runtime, .map_loading_failed));
}

test "offline region download errors are runtime events" {
    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.setResourceProvider(.{ .handler = errorStyleProvider });

    var definition = offlineTileDefinition();
    definition.tile_pyramid.style_url = "custom://error-style.json";
    const metadata = [_]u8{8};
    var created = try runtime.createOfflineRegion(testing.allocator, definition, metadata[0..]);
    defer created.deinit();
    const region_id = created.id;

    try runtime.setOfflineRegionObserved(region_id, true);
    defer runtime.setOfflineRegionObserved(region_id, false) catch {};
    try runtime.setOfflineRegionDownloadState(region_id, .active);
    defer runtime.setOfflineRegionDownloadState(region_id, .inactive) catch {};

    var event = try waitForOwnedEvent(&runtime, .offline_region_response_error);
    defer event.deinit();
    try testing.expect(std.meta.eql(event.payload_type, maplibre.RuntimeEventPayloadType.offline_region_response_error));
    const payload = switch (event.payload) {
        .offline_region_response_error => |payload| payload,
        else => return error.UnexpectedPayload,
    };
    try testing.expectEqual(region_id, payload.region_id);
    try testing.expect(std.meta.eql(payload.reason, maplibre.ResourceErrorReason.not_found));
    try testing.expect(event.message.len > 0);
}

fn waitForRequestCancellation(runtime: *maplibre.RuntimeHandle, handle: maplibre.ResourceRequestHandle) !void {
    for (0..5000) |_| {
        if (try handle.cancelled()) return;
        try runtime.runOnce();
        try sleepOneMillisecond();
    }
    return error.RequestNotCancelled;
}

test "resource provider observes cancellation before late completion" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    var map = try maplibre.MapHandle.create(&runtime, .{});
    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(&runtime, &state);
    defer handle.release();

    try map.close();
    try waitForRequestCancellation(&runtime, handle);
    try testing.expectError(error.InvalidState, handle.complete(.{ .bytes = support.style_json }));
    const diagnostic = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, -2), diagnostic.raw_status);
    try testing.expect(diagnostic.message.len > 0);
}

test "offline region download control emits copied status events" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPath(testing.allocator, tmp.sub_path[0..], "events-cache.db");
    defer testing.allocator.free(cache_path);

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    const metadata = [_]u8{9};
    var created = try runtime.createOfflineRegion(testing.allocator, offlineTileDefinition(), metadata[0..]);
    defer created.deinit();
    const region_id = created.id;

    try testing.expectError(error.InvalidArgument, runtime.setOfflineRegionObserved(region_id + 1000, true));
    try testing.expectError(error.InvalidArgument, runtime.setOfflineRegionDownloadState(region_id, .{ .unknown = 999 }));

    try runtime.setOfflineRegionObserved(region_id, true);
    defer runtime.setOfflineRegionObserved(region_id, false) catch {};
    try runtime.setOfflineRegionDownloadState(region_id, .active);
    defer runtime.setOfflineRegionDownloadState(region_id, .inactive) catch {};

    var observed = false;
    for (0..5000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (!std.meta.eql(owned_event.event_type, maplibre.RuntimeEventType.offline_region_status_changed)) continue;
            const payload = owned_event.payload.offline_region_status;
            try testing.expectEqual(region_id, payload.region_id);
            try testing.expect(
                std.meta.eql(payload.status.download_state, maplibre.OfflineRegionDownloadState.active) or
                    std.meta.eql(payload.status.download_state, maplibre.OfflineRegionDownloadState.inactive),
            );
            observed = true;
            break;
        }
        if (observed) break;
        try sleepOneMillisecond();
    }
    try testing.expect(observed);
}
