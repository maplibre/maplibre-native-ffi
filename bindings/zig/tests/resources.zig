const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

extern fn usleep(useconds: c_uint) c_int;

fn waitForEvent(runtime: maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            if (std.meta.eql(owned_event.event_type, event_type)) return true;
        }
        _ = usleep(1000);
    }
    return false;
}

fn waitForStyleLoaded(runtime: maplibre.RuntimeHandle) !void {
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
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
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.runAmbientCacheOperation(.pack_database);
}

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

test "resource transform rewrites network style URL" {
    try maplibre.setNetworkStatus(.online, null);
    defer maplibre.setNetworkStatus(.online, null) catch @panic("network status restore failed");

    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    const original_url = "http://example.invalid/original-style.json";
    var state = TransformState{
        .replacement_url = "unsupported://rewritten-style.json",
    };
    var replacement_state = TransformState{
        .replacement_url = "unsupported://unexpected-replacement.json",
    };
    try runtime.setResourceTransform(.{ .handler = rewriteStyleUrl, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try testing.expectError(error.InvalidState, runtime.setResourceTransform(.{ .handler = rewriteStyleUrl, .context = &replacement_state }));

    try map.setStyleUrl(testing.allocator, original_url);
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEventOwned(testing.allocator)) |event| {
            var owned_event = event;
            owned_event.deinit();
        }
        if (state.calls.load(.seq_cst) > 0) break;
    }

    try testing.expect(state.calls.load(.seq_cst) > 0);
    try testing.expectEqual(@as(usize, 0), replacement_state.calls.load(.seq_cst));
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

test "custom URL style loads through resource provider" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = ProviderState{};
    var replacement_state = ProviderState{};
    try runtime.setResourceProvider(.{ .handler = customStyleProvider, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");
    try testing.expectError(error.InvalidState, runtime.setResourceProvider(.{ .handler = customStyleProvider, .context = &replacement_state }));

    try map.setStyleUrl(testing.allocator, "custom://style.json");
    try waitForStyleLoaded(runtime);
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
    const cwd = try std.process.currentPathAlloc(testing.io, testing.allocator);
    defer testing.allocator.free(cwd);
    const cache_path = try std.fmt.allocPrint(testing.allocator, "{s}/.zig-cache/tmp/{s}/cache.db", .{ cwd, tmp.sub_path[0..] });
    defer testing.allocator.free(cache_path);

    const metadata = [_]u8{ 1, 2, 3 };
    const updated_metadata = [_]u8{ 4, 5, 6, 7 };
    var region_id: maplibre.OfflineRegionId = 0;

    {
        const runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
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
        const runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
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

test "offline geometry regions expose copied geometry values" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cwd = try std.process.currentPathAlloc(testing.io, testing.allocator);
    defer testing.allocator.free(cwd);
    const cache_path = try std.fmt.allocPrint(testing.allocator, "{s}/.zig-cache/tmp/{s}/geometry-cache.db", .{ cwd, tmp.sub_path[0..] });
    defer testing.allocator.free(cache_path);

    const coordinates = [_]maplibre.LatLng{
        .{ .latitude = 1.0, .longitude = 2.0 },
        .{ .latitude = 3.0, .longitude = 4.0 },
    };
    const metadata = [_]u8{ 7, 8, 9 };

    const runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
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

    const definition = created.definition.geometry;
    try testing.expectEqualStrings(offline_style_url, definition.style_url);
    try testing.expectEqual(@as(f64, 5.0), definition.min_zoom);
    try testing.expectEqual(@as(f64, 6.0), definition.max_zoom);
    try testing.expectEqual(@as(f32, 2.0), definition.pixel_ratio);
    try testing.expect(definition.include_ideographs);
    const copied_line = definition.geometry.line_string;
    try testing.expectEqual(@as(usize, 2), copied_line.len);
    try testing.expectEqual(@as(f64, 1.0), copied_line[0].latitude);
    try testing.expectEqual(@as(f64, 4.0), copied_line[1].longitude);
    try testing.expectEqualSlices(u8, metadata[0..], created.metadata);
}

const AsyncProviderState = struct {
    handle: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
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
        self.handle.store(handle.state, .seq_cst);
    }

    fn takeHandle(self: *AsyncProviderState) ?maplibre.ResourceRequestHandle {
        const raw = self.handle.swap(0, .seq_cst);
        if (raw == 0) return null;
        return .{ .state = raw };
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
    if (!std.mem.eql(u8, request.url, "custom://delayed-style.json")) return .pass_through;
    const handle = maybe_handle orelse return .pass_through;
    const state: *AsyncProviderState = @ptrCast(@alignCast(context.?));
    state.store(request, handle);
    return .handle;
}

fn waitForProviderHandle(runtime: maplibre.RuntimeHandle, state: *AsyncProviderState) !maplibre.ResourceRequestHandle {
    for (0..1000) |_| {
        try runtime.runOnce();
        if (state.takeHandle()) |handle| return handle;
        _ = usleep(1000);
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
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(runtime, &state);
    defer handle.release();

    try state.expectObservedRequest();
    try testing.expect(!try handle.cancelled());

    try handle.complete(.{ .bytes = support.style_json });
    try testing.expectError(error.AlreadyCompleted, handle.complete(.{ .bytes = support.style_json }));
    try waitForStyleLoaded(runtime);
}

test "released resource request handle copies stay closed after later requests" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const stale_handle = try waitForProviderHandle(runtime, &state);
    stale_handle.release();
    try testing.expectError(error.ClosedHandle, stale_handle.cancelled());

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const live_handle = try waitForProviderHandle(runtime, &state);
    defer live_handle.release();

    try testing.expectError(error.ClosedHandle, stale_handle.complete(.{ .bytes = support.style_json }));
    try testing.expect(!try live_handle.cancelled());
    try live_handle.complete(.{ .bytes = support.style_json });
    try waitForStyleLoaded(runtime);
}

test "resource provider can complete request from another thread" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(runtime, &state);
    defer handle.release();

    var completion_error: ?anyerror = error.NativeError;
    const thread = try std.Thread.spawn(.{}, completeStyleOnThread, .{ handle, &completion_error });
    thread.join();
    try testing.expect(completion_error == null);
    try waitForStyleLoaded(runtime);
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
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.setResourceProvider(.{ .handler = errorStyleProvider });

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleUrl(testing.allocator, "custom://error-style.json");
    try testing.expect(try waitForEvent(runtime, .map_loading_failed));
}

fn waitForRequestCancellation(runtime: maplibre.RuntimeHandle, handle: maplibre.ResourceRequestHandle) !void {
    for (0..5000) |_| {
        if (try handle.cancelled()) return;
        try runtime.runOnce();
        _ = usleep(1000);
    }
    return error.RequestNotCancelled;
}

test "resource provider observes cancellation before late completion" {
    const runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");

    var state = AsyncProviderState{};
    try runtime.setResourceProvider(.{ .handler = delayedStyleProvider, .context = &state });

    const map = try maplibre.MapHandle.create(runtime, .{});
    try map.setStyleUrl(testing.allocator, "custom://delayed-style.json");
    const handle = try waitForProviderHandle(runtime, &state);
    defer handle.release();

    try map.close();
    try waitForRequestCancellation(runtime, handle);
    try testing.expectError(error.InvalidState, handle.complete(.{ .bytes = support.style_json }));
}

test "offline region download control emits copied status events" {
    var tmp = testing.tmpDir(.{});
    defer tmp.cleanup();
    const cwd = try std.process.currentPathAlloc(testing.io, testing.allocator);
    defer testing.allocator.free(cwd);
    const cache_path = try std.fmt.allocPrint(testing.allocator, "{s}/.zig-cache/tmp/{s}/events-cache.db", .{ cwd, tmp.sub_path[0..] });
    defer testing.allocator.free(cache_path);

    const runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{ .cache_path = cache_path }, null);
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
        _ = usleep(1000);
    }
    try testing.expect(observed);
}
