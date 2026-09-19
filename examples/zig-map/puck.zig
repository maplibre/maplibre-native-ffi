//! The location-puck plugin layer: registration-free styling and the
//! owner-thread side of its keyboard tweaks. Property changes transition on
//! the host, so every tweak glides without demo-side animation code.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

const layer_id = "puck";
const entry_point = "mln_location_puck_register";

const layer_json =
    "{\"id\":\"puck\",\"type\":\"location-puck\",\"paint\":{" ++
    "\"position\":[37.7749,-122.4194]," ++
    "\"bearing\":0," ++
    "\"bearing-visible\":0," ++
    "\"accuracy-radius\":150," ++
    "\"bearing-accuracy\":25," ++
    "\"bearing-accuracy-radius\":90," ++
    "\"accuracy-border-width\":1.5," ++
    "\"pulse-radius\":0" ++
    "}}";

pub fn load(allocator: std.mem.Allocator, plugin_path: []const u8) !void {
    maplibre.loadPlugin(allocator, plugin_path, entry_point) catch |err| {
        std.debug.print("location-puck plugin load failed ({s}): {s}\n", .{ @errorName(err), plugin_path });
        return types.AppError.StyleLoadFailed;
    };
}

/// Adds the source-free layer once the base style is live. Runs on the map's
/// owner thread.
pub fn addLayer(allocator: std.mem.Allocator, map: *maplibre.MapHandle, diagnostic_store: ?*const maplibre.DiagnosticStore) !void {
    map.addStyleLayerJson(allocator, layer_json, "") catch |err| {
        diagnostics.logError("location-puck layer add failed", err, diagnostic_store);
        return types.AppError.StyleLoadFailed;
    };
    std.debug.print("location-puck layer added\n", .{});
}

pub fn moveBy(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    dx: f64,
    dy: f64,
) !void {
    if (!try layerExists(map)) return;
    const position = try readPosition(allocator, map, diagnostic_store);
    const pixel = map.pixelForLatLng(position) catch |err| {
        diagnostics.logError("puck projection failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
    const target = map.latLngForPixel(.{ .x = pixel.x + dx, .y = pixel.y + dy }) catch |err| {
        diagnostics.logError("puck projection failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
    try writeProperty(allocator, map, diagnostic_store, "position", "[{d},{d}]", .{ target.latitude, target.longitude });
}

pub fn rotateBy(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    delta: f64,
) !void {
    if (!try layerExists(map)) return;
    const bearing = try readNumber(allocator, map, diagnostic_store, "bearing", 0);
    try writeProperty(allocator, map, diagnostic_store, "bearing", "{d}", .{bearing + delta});
    // Rotating reveals the arrow and sector when they are hidden.
    if ((try readNumber(allocator, map, diagnostic_store, "bearing-visible", 0)) == 0) {
        try writeProperty(allocator, map, diagnostic_store, "bearing-visible", "1", .{});
    }
}

pub fn scaleAccuracyBy(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    scale: f64,
) !void {
    if (!try layerExists(map)) return;
    const radius = try readNumber(allocator, map, diagnostic_store, "accuracy-radius", 150);
    try writeProperty(allocator, map, diagnostic_store, "accuracy-radius", "{d}", .{std.math.clamp(radius * scale, 0.0, 5000.0)});
}

pub fn toggleBearing(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
) !void {
    if (!try layerExists(map)) return;
    const visible = try readNumber(allocator, map, diagnostic_store, "bearing-visible", 0);
    try writeProperty(allocator, map, diagnostic_store, "bearing-visible", "{d}", .{1.0 - std.math.clamp(visible, 0.0, 1.0)});
}

pub fn togglePulse(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
) !void {
    if (!try layerExists(map)) return;
    const radius = try readNumber(allocator, map, diagnostic_store, "pulse-radius", 0);
    try writeProperty(allocator, map, diagnostic_store, "pulse-radius", "{d}", .{if (radius > 0) @as(f64, 0) else 60.0});
}

/// A tweak before the style finishes loading finds no layer; drop it rather
/// than failing the application.
fn layerExists(map: *maplibre.MapHandle) !bool {
    return map.styleLayerExists(layer_id) catch false;
}

fn readPosition(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
) !maplibre.LatLng {
    var text = try readPropertyText(allocator, map, diagnostic_store, "position");
    defer if (text) |*owned| owned.deinit();
    if (text == null) return .{ .latitude = 37.7749, .longitude = -122.4194 };
    const parsed = std.json.parseFromSlice([2]f64, allocator, text.?.value, .{}) catch return error.CameraCommandFailed;
    defer parsed.deinit();
    return .{ .latitude = parsed.value[0], .longitude = parsed.value[1] };
}

fn readNumber(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    name: []const u8,
    default: f64,
) !f64 {
    var text = try readPropertyText(allocator, map, diagnostic_store, name);
    defer if (text) |*owned| owned.deinit();
    if (text == null) return default;
    const parsed = std.json.parseFromSlice(f64, allocator, text.?.value, .{}) catch return default;
    defer parsed.deinit();
    return parsed.value;
}

fn readPropertyText(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    name: []const u8,
) !?maplibre.OwnedString {
    return map.getLayerProperty(allocator, layer_id, name) catch |err| {
        diagnostics.logError("puck property read failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}

fn writeProperty(
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
    name: []const u8,
    comptime fmt: []const u8,
    args: anytype,
) !void {
    const value = try std.fmt.allocPrint(allocator, fmt, args);
    defer allocator.free(value);
    map.setLayerProperty(allocator, layer_id, name, value) catch |err| {
        diagnostics.logError("puck property write failed", err, diagnostic_store);
        return types.AppError.CameraCommandFailed;
    };
}
