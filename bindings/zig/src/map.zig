const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const RuntimeHandle = runtime_module.RuntimeHandle;
const status = @import("status.zig");
const values = @import("values.zig");

const CustomGeometrySourceState = struct {
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback,
    release_context: ?CustomGeometrySourceReleaseCallback,
    context: ?*anyopaque,
    active_upcalls: std.atomic.Value(usize),
};

const MapState = struct {
    runtime_registry: *runtime_module.RuntimeRegistry,
    id_value: values.MapId,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    attached_render_sessions: std.atomic.Value(usize),
    closing: bool,
    runtime: RuntimeHandle,
};

pub const RenderSessionRegistration = struct {
    native: c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    runtime: RuntimeHandle,
};

var custom_geometry_state_registry_lock = std.Io.Mutex.init;
var custom_geometry_state_registry: std.ArrayList(*CustomGeometrySourceState) = .empty;

// Keyed by map handle; the C API never reuses a handle value, so a released
// handle never collides with a live key.
var map_registry_lock = std.atomic.Value(bool).init(false);
var map_registry: std.AutoHashMapUnmanaged(c.mln_map, *MapState) = .empty;

pub const MapMode = enum {
    continuous,
    static,
    tile,

    fn toRaw(self: MapMode) u32 {
        return switch (self) {
            .continuous => c.MLN_MAP_MODE_CONTINUOUS,
            .static => c.MLN_MAP_MODE_STATIC,
            .tile => c.MLN_MAP_MODE_TILE,
        };
    }
};

/// Map creation options. A null field takes the C API default.
pub const MapOptions = struct {
    width: ?u32 = null,
    height: ?u32 = null,
    scale_factor: ?f64 = null,
    mode: ?MapMode = null,
    fast_pfor_enabled: ?bool = null,
    event_mask: ?runtime_module.RuntimeEventMask = null,
};

pub const CameraUpdateMode = enum {
    jump,
    ease,
    fly,

    fn toRaw(self: CameraUpdateMode) u32 {
        return switch (self) {
            .jump => c.MLN_CAMERA_UPDATE_MODE_JUMP,
            .ease => c.MLN_CAMERA_UPDATE_MODE_EASE,
            .fly => c.MLN_CAMERA_UPDATE_MODE_FLY,
        };
    }
};

pub const GesturePhase = enum {
    none,
    begin,
    update,
    end,
    cancel,

    fn toRaw(self: GesturePhase) u32 {
        return switch (self) {
            .none => c.MLN_GESTURE_PHASE_NONE,
            .begin => c.MLN_GESTURE_PHASE_BEGIN,
            .update => c.MLN_GESTURE_PHASE_UPDATE,
            .end => c.MLN_GESTURE_PHASE_END,
            .cancel => c.MLN_GESTURE_PHASE_CANCEL,
        };
    }
};

pub const CameraUpdate = struct {
    mode: CameraUpdateMode = .jump,
    camera: values.CameraOptions = .{},
    animation: values.AnimationOptions = .{},
    gesture_phase: GesturePhase = .none,
    gesture_id: u64 = 0,
    animation_id: u64 = 0,
};

pub const CameraSnapshot = struct {
    generation: u64,
    camera: values.CameraOptions,
};

/// Immutable map state copied from the latest published generation.
///
/// Every committed map command publishes a new generation and reports it in
/// its command-finished event, so a snapshot whose generation is at or past a
/// commit's observes that commit.
pub const MapSnapshot = struct {
    generation: u64,
    camera: values.CameraOptions,
    width: u32,
    height: u32,
    scale_factor: f64,
    debug_options: values.DebugOptions,
    projection_mode: values.ProjectionMode,
    viewport: values.ViewportOptions,
    tile: values.TileOptions,
    bounds: values.BoundOptions,
    free_camera: values.FreeCameraOptions,
    /// True once every requested style and tile resource finished loading.
    fully_loaded: bool,
    rendering_stats_view_enabled: bool,
    repaint_demand: bool,
    event_mask: runtime_module.RuntimeEventMask,
    latest_render_update_generation: u64,
};

pub const CanonicalTileId = struct {
    z: u32,
    x: u32,
    y: u32,
};
pub const StyleSourceMetadata = struct {
    source_type: values.StyleSourceType,
    id_size: usize,
    is_volatile: bool,
    has_attribution: bool,
    has_url: bool,
    has_tile_json: bool,
    min_zoom: f64,
    max_zoom: f64,
    scheme: values.StyleTileScheme,
    bounds: ?values.LatLngBounds,
    tile_count: usize,
    tile_size: ?u32,
    vector_encoding: ?values.StyleVectorTileEncoding,
    raster_encoding: ?values.StyleRasterDemEncoding,
};

/// Fixed layer metadata returned by `MapHandle.getStyleLayerInfoTakeResult`.
pub const StyleLayerInfo = struct {
    /// Style-spec layer type name. It views a static native string that stays
    /// valid for the life of the process.
    layer_type: []const u8,
    /// Lowest zoom at which the layer draws; -inf with no lower bound.
    min_zoom: f64,
    /// Highest zoom at which the layer draws; +inf with no upper bound.
    max_zoom: f64,
    visibility: values.StyleLayerVisibility,
    /// Source ID byte length, null when the layer names no source. It sizes
    /// buffers for `copyLayerSourceId`.
    source_id_size: ?usize,
    /// Source-layer byte length, null when the layer names no source layer. It
    /// sizes buffers for `copyLayerSourceLayer`.
    source_layer_size: ?usize,
};

pub const CustomGeometrySourceTileCallback = *const fn (
    context: ?*anyopaque,
    tile_id: CanonicalTileId,
) void;

pub const CustomGeometrySourceReleaseCallback = *const fn (context: ?*anyopaque) void;

/// Options for `MapHandle.addCustomGeometrySource`.
pub const CustomGeometrySourceOptions = struct {
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback = null,
    /// Invoked once with `context` after the map stops referencing this source:
    /// on an explicit removal, on a style load that leaves a style without the
    /// source, and on the map's own destruction. It runs on the runtime worker
    /// after the last tile callback returns, and never runs for an add that
    /// failed. A host frees `context` here instead of tracking style loads.
    release_context: ?CustomGeometrySourceReleaseCallback = null,
    context: ?*anyopaque = null,
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
    tolerance: ?f64 = null,
    tile_size: ?u32 = null,
    buffer: ?u32 = null,
    clip: ?bool = null,
    wrap: ?bool = null,
};

/// Prepared GeoJSON source data: one parsed and tiled GeoJSON document with
/// its source options baked in, ready to install on GeoJSON sources.
pub const GeoJsonSourceDataHandle = enum(c.mln_geojson_source_data) {
    _,

    /// Parses one complete UTF-8 GeoJSON document and tiles it into a
    /// prepared index under `options`; a null takes the C API defaults. When
    /// the options enable clustering, the data must be a feature collection
    /// whose every feature carries point geometry.
    ///
    /// Callable from any thread and free of any runtime or map, so a host
    /// prepares data on a worker thread and installs it on the map owner
    /// thread. The prepared value is immutable, and reads and installs may
    /// run concurrently from any thread. As with every handle in this
    /// binding, the host orders `release()` after the installs that use the
    /// handle; a release that races an install makes the install report an
    /// invalid-argument status for a stale handle, and never touches freed
    /// memory, because the C API resolves ids under its own lock and never
    /// reuses one.
    pub fn create(
        allocator: std.mem.Allocator,
        data: []const u8,
        options: ?values.StyleGeoJsonSourceOptions,
    ) status.Error!GeoJsonSourceDataHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleGeoJsonSourceOptionsToNative(&temp, value) else undefined;
        var out: c.mln_geojson_source_data = 0;
        try status.checkStatus(
            c.mln_geojson_source_data_create(
                try temp.stringView(data),
                if (options != null) &raw_options else null,
                &out,
            ),
            null,
        );
        return @enumFromInt(out);
    }

    /// Releases the prepared data from any thread. Releasing again is a no-op,
    /// and sources the data was installed on keep their own reference, so
    /// release never invalidates a source.
    pub fn release(self: GeoJsonSourceDataHandle) void {
        c.mln_geojson_source_data_destroy(@intFromEnum(self));
    }
};

pub const MapHandle = enum(c.mln_map) {
    _,

    pub fn create(runtime: *RuntimeHandle, options: MapOptions) status.Error!MapHandle {
        var native_options = c.mln_map_options_default();
        if (options.width) |value| native_options.initial_extent.width = value;
        if (options.height) |value| native_options.initial_extent.height = value;
        if (options.scale_factor) |value| native_options.initial_extent.scale_factor = value;
        if (options.mode) |value| native_options.map_mode = value.toRaw();
        if (options.fast_pfor_enabled) |value| native_options.fast_pfor_enabled = value;
        if (options.event_mask) |value| {
            native_options.event_mask = runtime_module.eventMaskToRaw(value);
        }
        const runtime_lease = try runtime_module.lease(runtime);
        defer runtime_lease.release();

        var operation: c.mln_operation = 0;
        const diagnostic_store = runtime_lease.diagnostic_store;
        try status.checkStatus(
            c.mln_map_create_start(runtime_lease.native, &native_options, &operation),
            diagnostic_store,
        );
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnostic_store);
        var map: c.mln_map = 0;
        try status.checkStatus(c.mln_map_create_take_result(operation, &map), diagnostic_store);

        const map_registration = try runtime_module.registerMap(runtime, map);
        errdefer runtime_module.unregisterMap(map_registration.registry, map);

        const map_state = try std.heap.smp_allocator.create(MapState);
        map_state.* = .{
            .runtime_registry = map_registration.registry,
            .id_value = map_registration.id,
            .diagnostic_store = diagnostic_store,
            .runtime = runtime.*,
            .attached_render_sessions = std.atomic.Value(usize).init(0),
            .closing = false,
        };
        errdefer std.heap.smp_allocator.destroy(map_state);

        return try registerMapState(map, map_state);
    }

    pub fn id(self: *MapHandle) status.BindingError!values.MapId {
        return mapIdForHandle(self);
    }

    /// Accepts an ordered inline-style command and returns its runtime-wide ID.
    pub fn setStyleJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        json: []const u8,
    ) status.Error!u64 {
        _ = allocator;
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_json(try native(self), stringView(json), &command_id), diagnosticStore(self));
        return command_id;
    }

    /// Accepts an ordered style-URL command and returns its runtime-wide ID.
    pub fn setStyleUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!u64 {
        const url_z = try nulTerminated(allocator, url, diagnosticStore(self), "style URL contains embedded NUL");
        defer allocator.free(url_z);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_url(try native(self), url_z.ptr, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn loadedStyleJsonStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperation(c.mln_map_loaded_style_json_start, .map_loaded_style_json, .string);
    }

    pub fn loadedStyleJsonTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.OwnedString {
        return self.takeStyleBuffer(allocator, operation, .map_loaded_style_json, c.mln_map_loaded_style_json_take_result);
    }

    pub fn styleUrlStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperation(c.mln_map_style_url_start, .map_style_url, .string);
    }

    pub fn styleUrlTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.OwnedString {
        return self.takeStyleBuffer(allocator, operation, .map_style_url, c.mln_map_style_url_take_result);
    }

    pub fn setLayerProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
        value: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_layer_property(
            try native(self),
            try temp.stringView(layer_id),
            try temp.stringView(property_name),
            try temp.stringView(value),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getLayerPropertyStart(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
    ) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_get_layer_property_start(
            try native(self),
            try temp.stringView(layer_id),
            try temp.stringView(property_name),
            &operation,
        ), diagnosticStore(self));
        return self.wrapStyleOperation(operation, .map_get_layer_property, .optional_string);
    }

    pub fn getLayerPropertyTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!?values.OwnedString {
        return self.takeOptionalStyleBuffer(allocator, operation, .map_get_layer_property, c.mln_map_get_layer_property_take_result);
    }

    pub fn setLayerFilter(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        filter: ?[]const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var filter_view = if (filter) |value| try temp.stringView(value) else undefined;
        const filter_ptr = if (filter != null) &filter_view else null;
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_layer_filter(
            try native(self),
            try temp.stringView(layer_id),
            filter_ptr,
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getLayerFilterStart(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, layer_id, c.mln_map_get_layer_filter_start, .map_get_layer_filter, .optional_string);
    }

    pub fn getLayerFilterTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!?values.OwnedString {
        return self.takeOptionalStyleBuffer(allocator, operation, .map_get_layer_filter, c.mln_map_get_layer_filter_take_result);
    }

    pub fn setLayerSourceLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_layer: []const u8,
    ) status.Error!u64 {
        return self.setLayerStringCommand(allocator, layer_id, source_layer, c.mln_map_set_layer_source_layer);
    }

    pub fn copyLayerSourceLayerStart(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, layer_id, c.mln_map_copy_layer_source_layer_start, .map_copy_layer_source_layer, .string);
    }

    pub fn copyLayerSourceLayerTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.OwnedString {
        return self.takeStyleBuffer(allocator, operation, .map_copy_layer_source_layer, c.mln_map_copy_layer_source_layer_take_result);
    }

    pub fn setLayerSourceId(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
    ) status.Error!u64 {
        return self.setLayerStringCommand(allocator, layer_id, source_id, c.mln_map_set_layer_source_id);
    }

    pub fn copyLayerSourceIdStart(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, layer_id, c.mln_map_copy_layer_source_id_start, .map_copy_layer_source_id, .string);
    }

    pub fn copyLayerSourceIdTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.OwnedString {
        return self.takeStyleBuffer(allocator, operation, .map_copy_layer_source_id, c.mln_map_copy_layer_source_id_take_result);
    }

    pub fn setLayerMinZoom(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8, value: f64) status.Error!u64 {
        return self.setLayerNumberCommand(allocator, layer_id, value, c.mln_map_set_layer_min_zoom);
    }

    pub fn setLayerMaxZoom(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8, value: f64) status.Error!u64 {
        return self.setLayerNumberCommand(allocator, layer_id, value, c.mln_map_set_layer_max_zoom);
    }

    pub fn setLayerVisibility(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        visibility: values.StyleLayerVisibility,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_layer_visibility(
            try native(self),
            try temp.stringView(layer_id),
            visibility.toRaw(),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getStyleLayerInfoStart(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, layer_id, c.mln_map_get_style_layer_info_start, .map_get_style_layer_info, .optional_style_layer_info);
    }

    pub fn getStyleLayerInfoTakeResult(
        self: *MapHandle,
        operation: runtime_module.OperationHandle,
    ) status.Error!?StyleLayerInfo {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_style_layer_info, .optional_style_layer_info);
        defer required.lease.release();
        var raw = std.mem.zeroes(c.mln_style_layer_info);
        raw.size = @sizeOf(c.mln_style_layer_info);
        var found = false;
        try status.checkStatus(c.mln_map_get_style_layer_info_take_result(required.lease.native, &raw, &found), state.diagnostic_store);
        if (!found) return null;
        const type_data: [*]const u8 = @ptrCast(raw.type.data orelse return error.NativeError);
        return .{
            .layer_type = type_data[0..raw.type.size],
            .min_zoom = raw.min_zoom,
            .max_zoom = raw.max_zoom,
            .visibility = values.StyleLayerVisibility.fromRaw(raw.visibility),
            .source_id_size = if ((raw.fields & c.MLN_STYLE_LAYER_INFO_SOURCE_ID) != 0) raw.source_id_size else null,
            .source_layer_size = if ((raw.fields & c.MLN_STYLE_LAYER_INFO_SOURCE_LAYER) != 0) raw.source_layer_size else null,
        };
    }

    pub fn listStyleSourceIdsStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperation(c.mln_map_list_style_source_ids_start, .map_list_style_source_ids, .string_list);
    }

    pub fn listStyleSourceIdsTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.StringList {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_list_style_source_ids, .string_list);
        defer required.lease.release();
        var list: c.mln_style_id_list = 0;
        try status.checkStatus(c.mln_map_list_style_source_ids_take_result(required.lease.native, &list), state.diagnostic_store);
        defer c.mln_style_id_list_destroy(list);
        return copyStyleIdList(allocator, list, state.diagnostic_store);
    }

    pub fn listStyleLayerIdsStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperation(c.mln_map_list_style_layer_ids_start, .map_list_style_layer_ids, .string_list);
    }

    pub fn listStyleLayerIdsTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!values.StringList {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_list_style_layer_ids, .string_list);
        defer required.lease.release();
        var list: c.mln_style_id_list = 0;
        try status.checkStatus(c.mln_map_list_style_layer_ids_take_result(required.lease.native, &list), state.diagnostic_store);
        defer c.mln_style_id_list_destroy(list);
        return copyStyleIdList(allocator, list, state.diagnostic_store);
    }

    pub fn addStyleSourceJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        source_json: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_add_style_source_json(
            try native(self),
            try temp.stringView(source_id),
            try temp.stringView(source_json),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    /// Accepts an ordered source-removal command and returns its runtime-wide
    /// ID. The command's finished event reports a committed removal, a
    /// `NotFound` failure when no source has the ID, and an `InvalidState`
    /// failure when a layer still uses the source.
    pub fn removeStyleSource(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!u64 {
        return self.removeStyleObjectCommand(allocator, source_id, c.mln_map_remove_style_source);
    }

    pub fn getStyleSourceInfoStart(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, source_id, c.mln_map_get_style_source_info_start, .map_get_style_source_info, .optional_style_source_info);
    }

    pub fn getStyleSourceInfoTakeResult(
        self: *MapHandle,
        operation: runtime_module.OperationHandle,
    ) status.Error!?StyleSourceMetadata {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_style_source_info, .optional_style_source_info);
        defer required.lease.release();
        var raw = std.mem.zeroes(c.mln_style_source_info);
        raw.size = @sizeOf(c.mln_style_source_info);
        var found = false;
        try status.checkStatus(c.mln_map_get_style_source_info_take_result(required.lease.native, &raw, &found), state.diagnostic_store);
        if (!found) return null;
        return .{
            .source_type = values.styleSourceTypeFromNative(raw.type),
            .id_size = raw.id_size,
            .is_volatile = raw.is_volatile,
            .has_attribution = raw.has_attribution,
            .has_url = (raw.fields & c.MLN_STYLE_SOURCE_INFO_URL) != 0,
            .has_tile_json = (raw.fields & c.MLN_STYLE_SOURCE_INFO_TILEJSON) != 0,
            .min_zoom = raw.min_zoom,
            .max_zoom = raw.max_zoom,
            .scheme = values.StyleTileScheme.fromRaw(raw.scheme),
            .bounds = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) values.latLngBoundsFromNative(raw.bounds) else null,
            .tile_count = raw.tile_count,
            .tile_size = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0) raw.tile_size else null,
            .vector_encoding = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0) values.StyleVectorTileEncoding.fromRaw(raw.vector_encoding) else null,
            .raster_encoding = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0) values.StyleRasterDemEncoding.fromRaw(raw.raster_encoding) else null,
        };
    }

    pub fn copyStyleSourceAttributionStart(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, source_id, c.mln_map_copy_style_source_attribution_start, .map_copy_style_source_attribution, .optional_string);
    }

    pub fn copyStyleSourceAttributionTakeResult(self: *MapHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!?values.OwnedString {
        return self.takeFoundStyleBuffer(allocator, operation, .map_copy_style_source_attribution, c.mln_map_copy_style_source_attribution_take_result);
    }

    pub fn copyStyleSourceUrlStart(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, source_id, c.mln_map_copy_style_source_url_start, .map_copy_style_source_url, .optional_string);
    }

    pub fn copyStyleSourceUrlTakeResult(self: *MapHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!?values.OwnedString {
        return self.takeFoundStyleBuffer(allocator, operation, .map_copy_style_source_url, c.mln_map_copy_style_source_url_take_result);
    }

    pub fn getStyleSourceTileUrlsStart(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, source_id, c.mln_map_get_style_source_tile_urls_start, .map_get_style_source_tile_urls, .string_list);
    }

    pub fn getStyleSourceTileUrlsTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!?values.StringList {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_style_source_tile_urls, .string_list);
        defer required.lease.release();
        var list: c.mln_style_string_list = 0;
        var found = false;
        try status.checkStatus(c.mln_map_get_style_source_tile_urls_take_result(required.lease.native, &list, &found), state.diagnostic_store);
        if (!found) return null;
        defer c.mln_style_string_list_destroy(list);
        return try copyStyleStringList(allocator, list, state.diagnostic_store);
    }

    pub fn addStyleLayerJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_json: []const u8,
        before_layer_id: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_add_style_layer_json(
            try native(self),
            try temp.stringView(layer_json),
            stringView(before_layer_id),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    /// Accepts an ordered layer-removal command and returns its runtime-wide
    /// ID. The command's finished event reports a committed removal, and a
    /// `NotFound` failure when no layer has the ID.
    pub fn removeStyleLayer(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!u64 {
        return self.removeStyleObjectCommand(allocator, layer_id, c.mln_map_remove_style_layer);
    }

    pub fn moveStyleLayer(self: *MapHandle, layer_id: []const u8, before_layer_id: []const u8) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_move_style_layer(
            try native(self),
            stringView(layer_id),
            stringView(before_layer_id),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getStyleLayerJsonStart(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, layer_id, c.mln_map_get_style_layer_json_start, .map_get_style_layer_json, .optional_string);
    }

    pub fn getStyleLayerJsonTakeResult(self: *MapHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!?values.OwnedString {
        return self.takeFoundStyleBuffer(allocator, operation, .map_get_style_layer_json, c.mln_map_get_style_layer_json_take_result);
    }

    pub fn setStyleLightJson(self: *MapHandle, allocator: std.mem.Allocator, value: []const u8) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_light_json(try native(self), try temp.stringView(value), &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setStyleLightProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        property_name: []const u8,
        value: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_light_property(
            try native(self),
            try temp.stringView(property_name),
            try temp.stringView(value),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getStyleLightPropertyStart(self: *MapHandle, allocator: std.mem.Allocator, property_name: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, property_name, c.mln_map_get_style_light_property_start, .map_get_style_light_property, .optional_string);
    }

    pub fn getStyleLightPropertyTakeResult(self: *MapHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!?values.OwnedString {
        return self.takeOptionalStyleBuffer(allocator, operation, .map_get_style_light_property, c.mln_map_get_style_light_property_take_result);
    }

    pub fn setStyleTransitionOptions(self: *MapHandle, options: values.StyleTransitionOptions) status.Error!u64 {
        const raw = values.styleTransitionOptionsToNative(options);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_transition_options(try native(self), &raw, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn getStyleTransitionOptionsStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperation(c.mln_map_get_style_transition_options_start, .map_get_style_transition_options, .style_transition_options);
    }

    pub fn getStyleTransitionOptionsTakeResult(self: *MapHandle, operation: runtime_module.OperationHandle) status.Error!values.StyleTransitionOptions {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_style_transition_options, .style_transition_options);
        defer required.lease.release();
        var raw = c.mln_style_transition_options_default();
        try status.checkStatus(c.mln_map_get_style_transition_options_take_result(required.lease.native, &raw), state.diagnostic_store);
        return values.styleTransitionOptionsFromNative(raw);
    }

    pub fn addVectorSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_vector_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addVectorSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_vector_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addRasterSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_raster_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addRasterSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_raster_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addRasterDemSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_raster_dem_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addRasterDemSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_raster_dem_source_tiles(try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addHillshadeLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_hillshade_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addColorReliefLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_color_relief_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setStyleImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
        image: values.PremultipliedRgba8Image,
        options: ?values.StyleImageOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        const stretch_x_count = if (options) |value| if (value.stretch_x) |stretches| stretches.len else 0 else 0;
        const stretch_y_count = if (options) |value| if (value.stretch_y) |stretches| stretches.len else 0 else 0;
        const stretch_x = try allocator.alloc(c.mln_image_stretch, stretch_x_count);
        defer allocator.free(stretch_x);
        const stretch_y = try allocator.alloc(c.mln_image_stretch, stretch_y_count);
        defer allocator.free(stretch_y);
        var raw_options = if (options) |value| values.styleImageOptionsToNative(value, stretch_x, stretch_y) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_style_image(
            try native(self),
            try temp.stringView(image_id),
            &raw_image,
            if (options != null) &raw_options else null,
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn copyStyleImageStretchesStart(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, image_id, c.mln_map_copy_style_image_stretches_start, .map_copy_style_image_stretches, .optional_image_stretches);
    }

    pub fn copyStyleImageStretchesTakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!?values.OwnedImageStretches {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_copy_style_image_stretches, .optional_image_stretches);
        defer required.lease.release();
        var x_count: usize = 0;
        var y_count: usize = 0;
        var found = false;
        try status.checkStatus(c.mln_map_copy_style_image_stretches_take_result(
            required.lease.native,
            null,
            0,
            &x_count,
            null,
            0,
            &y_count,
            &found,
        ), state.diagnostic_store);
        if (!found) return null;
        const raw_x = try allocator.alloc(c.mln_image_stretch, x_count);
        defer allocator.free(raw_x);
        const raw_y = try allocator.alloc(c.mln_image_stretch, y_count);
        defer allocator.free(raw_y);
        try status.checkStatus(c.mln_map_copy_style_image_stretches_take_result(
            required.lease.native,
            if (raw_x.len == 0) null else raw_x.ptr,
            raw_x.len,
            &x_count,
            if (raw_y.len == 0) null else raw_y.ptr,
            raw_y.len,
            &y_count,
            &found,
        ), state.diagnostic_store);
        const stretch_x = try allocator.alloc(values.ImageStretch, x_count);
        errdefer allocator.free(stretch_x);
        const stretch_y = try allocator.alloc(values.ImageStretch, y_count);
        errdefer allocator.free(stretch_y);
        for (raw_x[0..x_count], 0..) |stretch, index| stretch_x[index] = .{ .from = stretch.from, .to = stretch.to };
        for (raw_y[0..y_count], 0..) |stretch, index| stretch_y[index] = .{ .from = stretch.from, .to = stretch.to };
        return .{ .allocator = allocator, .stretch_x = stretch_x, .stretch_y = stretch_y };
    }

    /// Accepts an ordered image-removal command and returns its runtime-wide
    /// ID. The command's finished event reports a committed removal, and a
    /// `NotFound` failure when no runtime style image has the ID.
    pub fn removeStyleImage(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!u64 {
        return self.removeStyleObjectCommand(allocator, image_id, c.mln_map_remove_style_image);
    }

    pub fn getStyleImageInfoStart(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, image_id, c.mln_map_get_style_image_info_start, .map_get_style_image_info, .optional_style_image_info);
    }

    pub fn getStyleImageInfoTakeResult(self: *MapHandle, operation: runtime_module.OperationHandle) status.Error!?values.StyleImageInfo {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_style_image_info, .optional_style_image_info);
        defer required.lease.release();
        var info = c.mln_style_image_info_default();
        var found = false;
        try status.checkStatus(c.mln_map_get_style_image_info_take_result(required.lease.native, &info, &found), state.diagnostic_store);
        return if (found) values.styleImageInfoFromNative(info) else null;
    }

    pub fn copyStyleImagePremultipliedRgba8Start(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, image_id, c.mln_map_copy_style_image_premultiplied_rgba8_start, .map_copy_style_image_pixels, .optional_string);
    }

    pub fn copyStyleImagePremultipliedRgba8TakeResult(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
    ) status.Error!?values.OwnedString {
        return self.takeFoundStyleBuffer(allocator, operation, .map_copy_style_image_pixels, c.mln_map_copy_style_image_premultiplied_rgba8_take_result);
    }

    pub fn addImageSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        url: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_add_image_source_url(
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
            try temp.stringView(url),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn addImageSourceImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        image: values.PremultipliedRgba8Image,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_add_image_source_image(
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
            &raw_image,
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn setImageSourceUrl(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, url: []const u8) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_image_source_url(
            try native(self),
            try temp.stringView(source_id),
            try temp.stringView(url),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn setImageSourceImage(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, image: values.PremultipliedRgba8Image) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_image_source_image(
            try native(self),
            try temp.stringView(source_id),
            &raw_image,
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn setImageSourceCoordinates(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, coordinates: [4]values.LatLng) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_image_source_coordinates(
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    pub fn getImageSourceCoordinatesStart(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!runtime_module.OperationHandle {
        return self.startStyleOperationWithId(allocator, source_id, c.mln_map_get_image_source_coordinates_start, .map_get_image_source_coordinates, .optional_coordinates);
    }

    pub fn getImageSourceCoordinatesTakeResult(self: *MapHandle, operation: runtime_module.OperationHandle) status.Error!?[4]values.LatLng {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_get_image_source_coordinates, .optional_coordinates);
        defer required.lease.release();
        var raw_coordinates: [4]c.mln_lat_lng = undefined;
        var count: usize = 0;
        var found = false;
        try status.checkStatus(c.mln_map_get_image_source_coordinates_take_result(
            required.lease.native,
            &raw_coordinates,
            raw_coordinates.len,
            &count,
            &found,
        ), state.diagnostic_store);
        if (!found) return null;
        if (count != raw_coordinates.len) return error.NativeError;
        var coordinates: [4]values.LatLng = undefined;
        for (raw_coordinates, &coordinates) |raw, *coordinate| coordinate.* = values.latLngFromNative(raw);
        return coordinates;
    }

    pub fn addLocationIndicatorLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_location_indicator_layer(try native(self), try temp.stringView(layer_id), try temp.stringView(before_layer_id), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setLocationIndicatorLocation(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        coordinate: values.LatLng,
        altitude: f64,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_location_indicator_location(
                try native(self),
                try temp.stringView(layer_id),
                values.latLngToNative(coordinate),
                altitude,
                &command_id,
            ),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setLocationIndicatorBearing(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        bearing: f64,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_location_indicator_bearing(try native(self), try temp.stringView(layer_id), bearing, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setLocationIndicatorAccuracyRadius(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        radius: f64,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_location_indicator_accuracy_radius(try native(self), try temp.stringView(layer_id), radius, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setLocationIndicatorImageName(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        kind: values.LocationIndicatorImageKind,
        image_id: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_location_indicator_image_name(
                try native(self),
                try temp.stringView(layer_id),
                values.locationIndicatorImageKindToNative(kind),
                try temp.stringView(image_id),
                &command_id,
            ),
            diagnosticStore(self),
        );
        return command_id;
    }

    /// Adds a GeoJSON source with prepared inline data. The call borrows
    /// `data`, and the source adopts the options the data was prepared with,
    /// fixed for the lifetime of the source.
    pub fn addGeoJsonSourceData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: GeoJsonSourceDataHandle,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_geojson_source_data(
                try native(self),
                try temp.stringView(source_id),
                @intFromEnum(data),
                &command_id,
            ),
            diagnosticStore(self),
        );
        return command_id;
    }

    /// Updates one GeoJSON source with prepared inline data. The call borrows
    /// `data`, which must have been prepared with options equal to the options
    /// the source was added with; a mismatch is rejected. Cluster aggregation
    /// expressions compare by parsed equality, so equivalent
    /// `cluster_properties` JSON matches regardless of formatting.
    pub fn setGeoJsonSourceData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: GeoJsonSourceDataHandle,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_geojson_source_data(try native(self), try temp.stringView(source_id), @intFromEnum(data), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime. While
    /// enabled, the source slices requested tiles inline during the update
    /// pass, as if its options had set `synchronous_tiling`; false restores
    /// the option the source was added with.
    pub fn setGeoJsonSourceSynchronousTiling(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        enabled: bool,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_geojson_source_synchronous_tiling(try native(self), try temp.stringView(source_id), enabled, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    /// Adds a GeoJSON source that loads from a URL. The options are fixed at
    /// creation; a later `setGeoJsonSourceUrl` call keeps them, and a later
    /// `setGeoJsonSourceData` call requires data prepared with equal options.
    pub fn addGeoJsonSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleGeoJsonSourceOptions,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleGeoJsonSourceOptionsToNative(&temp, value) else undefined;
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_geojson_source_url(
                try native(self),
                try temp.stringView(source_id),
                try temp.stringView(url),
                if (options != null) &raw_options else null,
                &command_id,
            ),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn setGeoJsonSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_geojson_source_url(try native(self), try temp.stringView(source_id), try temp.stringView(url), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn addCustomGeometrySource(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: CustomGeometrySourceOptions,
    ) status.Error!u64 {
        _ = try native(self);
        const map_state = try mapStateForHandle(self);

        const source_state = try std.heap.smp_allocator.create(CustomGeometrySourceState);
        source_state.* = .{
            .fetch_tile = options.fetch_tile,
            .cancel_tile = options.cancel_tile,
            .release_context = options.release_context,
            .context = options.context,
            .active_upcalls = std.atomic.Value(usize).init(0),
        };
        errdefer std.heap.smp_allocator.destroy(source_state);

        try registerLiveCustomGeometrySourceState(source_state);
        // A failed add releases nothing, so this call owns the state it built.
        errdefer unregisterLiveCustomGeometrySourceState(source_state);

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var native_options = customGeometrySourceOptionsToNative(options, source_state);
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_add_custom_geometry_source(try native(self), try temp.stringView(source_id), &native_options, &command_id),
            map_state.diagnostic_store,
        );
        return command_id;
    }

    pub fn setCustomGeometrySourceTileData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
        data: []const u8,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_set_custom_geometry_source_tile_data(
                try native(self),
                try temp.stringView(source_id),
                canonicalTileIdToNative(tile_id),
                try temp.stringView(data),
                &command_id,
            ),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn invalidateCustomGeometrySourceTile(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_invalidate_custom_geometry_source_tile(try native(self), try temp.stringView(source_id), canonicalTileIdToNative(tile_id), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn invalidateCustomGeometrySourceRegion(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        bounds: values.LatLngBounds,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_invalidate_custom_geometry_source_region(try native(self), try temp.stringView(source_id), values.latLngBoundsToNative(bounds), &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn requestRepaint(self: *MapHandle) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_request_repaint(try native(self), &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setDebugOptions(self: *MapHandle, options: values.DebugOptions) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_debug_options(try native(self), values.debugOptionsToNative(options), &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setRenderingStatsViewEnabled(self: *MapHandle, enabled: bool) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_rendering_stats_view_enabled(try native(self), enabled, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn snapshot(self: *MapHandle) status.Error!MapSnapshot {
        var raw = std.mem.zeroes(c.mln_map_snapshot);
        raw.size = @sizeOf(c.mln_map_snapshot);
        try status.checkStatus(c.mln_map_snapshot_get(try native(self), &raw), diagnosticStore(self));
        return .{
            .generation = raw.generation,
            .camera = values.cameraOptionsFromNative(raw.camera),
            .width = raw.logical_extent.width,
            .height = raw.logical_extent.height,
            .scale_factor = raw.logical_extent.scale_factor,
            .debug_options = values.debugOptionsFromNative(raw.debug_options),
            .projection_mode = values.projectionModeFromNative(raw.projection_mode),
            .viewport = try values.viewportOptionsFromNative(raw.viewport),
            .tile = try values.tileOptionsFromNative(raw.tile),
            .bounds = values.boundOptionsFromNative(raw.bounds),
            .free_camera = values.freeCameraOptionsFromNative(raw.free_camera),
            .fully_loaded = raw.fully_loaded,
            .rendering_stats_view_enabled = raw.rendering_stats_view_enabled,
            .repaint_demand = raw.repaint_demand,
            .event_mask = runtime_module.eventMaskFromRaw(raw.event_mask),
            .latest_render_update_generation = raw.latest_render_update_generation,
        };
    }

    pub fn getSize(self: *MapHandle) status.Error!struct { width: u32, height: u32, scale_factor: f64 } {
        const current = try self.snapshot();
        return .{ .width = current.width, .height = current.height, .scale_factor = current.scale_factor };
    }

    pub fn resize(self: *MapHandle, width: u32, height: u32, scale_factor: f64) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(
            c.mln_map_resize(try native(self), .{ .width = width, .height = height, .scale_factor = scale_factor }, &command_id),
            diagnosticStore(self),
        );
        return command_id;
    }

    pub fn dumpDebugLogs(self: *MapHandle) status.Error!u64 {
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_dump_debug_logs(try native(self), &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setViewportOptions(self: *MapHandle, options: values.ViewportOptions) status.Error!u64 {
        var raw_options = values.viewportOptionsToNative(options);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_viewport_options(try native(self), &raw_options, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setTileOptions(self: *MapHandle, options: values.TileOptions) status.Error!u64 {
        var raw_options = values.tileOptionsToNative(options);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_tile_options(try native(self), &raw_options, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn cameraSnapshot(self: *MapHandle) status.Error!CameraSnapshot {
        var camera = c.mln_camera_options_default();
        var generation: u64 = 0;
        try status.checkStatus(
            c.mln_map_camera_snapshot_get(try native(self), &camera, &generation),
            diagnosticStore(self),
        );
        return .{ .generation = generation, .camera = values.cameraOptionsFromNative(camera) };
    }

    pub fn updateCamera(self: *MapHandle, update: CameraUpdate) status.Error!u64 {
        var raw = c.mln_camera_update_default();
        raw.mode = update.mode.toRaw();
        raw.camera = values.cameraOptionsToNative(update.camera);
        raw.animation = values.animationOptionsToNative(update.animation);
        raw.gesture_phase = update.gesture_phase.toRaw();
        raw.gesture_id = update.gesture_id;
        raw.animation_id = update.animation_id;
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_update_camera(try native(self), &raw, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn cameraQueryStart(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        const state = try mapStateForHandle(self);
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_camera_query_start(try native(self), &operation), state.diagnostic_store);
        return runtime_module.OperationHandle.init(&state.runtime, operation, .map_camera_query, .camera) catch |err| {
            c.mln_operation_release(operation);
            return err;
        };
    }

    pub fn cameraQueryTakeResult(
        self: *MapHandle,
        operation: runtime_module.OperationHandle,
    ) status.Error!CameraSnapshot {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, .map_camera_query, .camera);
        defer required.lease.release();
        var result = std.mem.zeroes(c.mln_camera_query_result);
        result.size = @sizeOf(c.mln_camera_query_result);
        try status.checkStatus(c.mln_map_camera_query_take_result(required.lease.native, &result), state.diagnostic_store);
        return .{ .generation = result.generation, .camera = values.cameraOptionsFromNative(result.camera) };
    }

    pub fn requestStillImage(self: *MapHandle) status.Error!runtime_module.OperationHandle {
        const state = try mapStateForHandle(self);
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_request_still_image_start(try native(self), &operation), state.diagnostic_store);
        return runtime_module.OperationHandle.init(&state.runtime, operation, .map_still_image, .none) catch |err| {
            c.mln_operation_release(operation);
            return err;
        };
    }
    pub fn setProjectionMode(self: *MapHandle, mode: values.ProjectionMode) status.Error!u64 {
        var raw_mode = values.projectionModeToNative(mode);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_projection_mode(try native(self), &raw_mode, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn getProjectionMode(self: *MapHandle) status.Error!values.ProjectionMode {
        return (try self.snapshot()).projection_mode;
    }

    pub fn pixelForLatLng(self: *MapHandle, coordinate: values.LatLng) status.Error!values.ScreenPoint {
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_pixel_for_lat_lng_start(try native(self), values.latLngToNative(coordinate), &operation), diagnosticStore(self));
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        var point: c.mln_screen_point = undefined;
        try status.checkStatus(c.mln_map_pixel_for_lat_lng_take_result(operation, &point), diagnosticStore(self));
        return values.screenPointFromNative(point);
    }

    pub fn latLngForPixel(self: *MapHandle, point: values.ScreenPoint) status.Error!values.LatLng {
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_lat_lng_for_pixel_start(try native(self), values.screenPointToNative(point), &operation), diagnosticStore(self));
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        var coordinate: c.mln_lat_lng = undefined;
        try status.checkStatus(c.mln_map_lat_lng_for_pixel_take_result(operation, &coordinate), diagnosticStore(self));
        return values.latLngFromNative(coordinate);
    }

    pub fn pixelsForLatLngs(self: *MapHandle, allocator: std.mem.Allocator, coordinates: []const values.LatLng, out_points: []values.ScreenPoint) status.Error!void {
        if (coordinates.len != out_points.len) return error.InvalidArgument;
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_pixels_for_lat_lngs_start(try native(self), if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len, &operation), diagnosticStore(self));
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        const raw_points = try allocator.alloc(c.mln_screen_point, out_points.len);
        defer allocator.free(raw_points);
        var count: usize = 0;
        try status.checkStatus(c.mln_map_pixels_for_lat_lngs_take_result(operation, if (raw_points.len == 0) null else raw_points.ptr, raw_points.len, &count), diagnosticStore(self));
        if (count != out_points.len) return error.NativeError;
        for (raw_points, out_points) |raw_point, *out_point| out_point.* = values.screenPointFromNative(raw_point);
    }

    pub fn latLngsForPixels(self: *MapHandle, allocator: std.mem.Allocator, points: []const values.ScreenPoint, out_coordinates: []values.LatLng) status.Error!void {
        if (points.len != out_coordinates.len) return error.InvalidArgument;
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_lat_lngs_for_pixels_start(try native(self), if (raw_points.len == 0) null else raw_points.ptr, raw_points.len, &operation), diagnosticStore(self));
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        const raw_coordinates = try allocator.alloc(c.mln_lat_lng, out_coordinates.len);
        defer allocator.free(raw_coordinates);
        var count: usize = 0;
        try status.checkStatus(c.mln_map_lat_lngs_for_pixels_take_result(operation, if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len, &count), diagnosticStore(self));
        if (count != out_coordinates.len) return error.NativeError;
        for (raw_coordinates, out_coordinates) |raw_coordinate, *out_coordinate| out_coordinate.* = values.latLngFromNative(raw_coordinate);
    }

    pub fn cameraForLatLngBounds(self: *MapHandle, bounds: values.LatLngBounds, fit_options: ?values.CameraFitOptions) status.Error!values.CameraOptions {
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_camera_for_lat_lng_bounds_start(try native(self), values.latLngBoundsToNative(bounds), fit_ptr, &operation), diagnosticStore(self));
        return self.takeCameraOperation(operation, c.mln_map_camera_for_lat_lng_bounds_take_result);
    }

    pub fn cameraForLatLngs(self: *MapHandle, allocator: std.mem.Allocator, coordinates: []const values.LatLng, fit_options: ?values.CameraFitOptions) status.Error!values.CameraOptions {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_camera_for_lat_lngs_start(try native(self), if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len, fit_ptr, &operation), diagnosticStore(self));
        return self.takeCameraOperation(operation, c.mln_map_camera_for_lat_lngs_take_result);
    }

    pub fn cameraForGeometry(self: *MapHandle, allocator: std.mem.Allocator, geometry: []const u8, fit_options: ?values.CameraFitOptions) status.Error!values.CameraOptions {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_map_camera_for_geometry_start(try native(self), try temp.stringView(geometry), fit_ptr, &operation), diagnosticStore(self));
        return self.takeCameraOperation(operation, c.mln_map_camera_for_geometry_take_result);
    }

    fn takeCameraOperation(self: *MapHandle, operation: c.mln_operation, take: *const fn (c.mln_operation, *c.mln_camera_options) callconv(.c) c.mln_status) status.Error!values.CameraOptions {
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        var camera = c.mln_camera_options_default();
        try status.checkStatus(take(operation, &camera), diagnosticStore(self));
        return values.cameraOptionsFromNative(camera);
    }

    /// Computes geographic bounds for a camera from two viewport corners.
    ///
    /// The box is the hull of the top-left and bottom-right screen corners for
    /// that camera in the current viewport. When bearing and pitch are zero, the
    /// box equals the visible area. Those corners are the northwest and
    /// southeast of the viewport. Longitudes stay in -180 to 180.
    pub fn latLngBoundsForCamera(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        return self.takeBoundsOperation(&raw_camera, c.mln_map_lat_lng_bounds_for_camera_start, c.mln_map_lat_lng_bounds_for_camera_take_result);
    }

    /// Computes geographic bounds for a camera from the four viewport corners.
    ///
    /// The axis-aligned hull of all four screen corners and the center
    /// encompasses the projected viewport. Longitudes unwrap onto the shortest
    /// path through the center. A viewport that crosses the antimeridian reports
    /// values outside -180 to 180.
    pub fn latLngBoundsForCameraUnwrapped(self: *MapHandle, camera: values.CameraOptions) status.Error!values.LatLngBounds {
        var raw_camera = values.cameraOptionsToNative(camera);
        return self.takeBoundsOperation(&raw_camera, c.mln_map_lat_lng_bounds_for_camera_unwrapped_start, c.mln_map_lat_lng_bounds_for_camera_unwrapped_take_result);
    }

    fn takeBoundsOperation(self: *MapHandle, camera: *const c.mln_camera_options, start: *const fn (c.mln_map, *const c.mln_camera_options, *c.mln_operation) callconv(.c) c.mln_status, take: *const fn (c.mln_operation, *c.mln_lat_lng_bounds) callconv(.c) c.mln_status) status.Error!values.LatLngBounds {
        var operation: c.mln_operation = 0;
        try status.checkStatus(start(try native(self), camera, &operation), diagnosticStore(self));
        defer c.mln_operation_release(operation);
        try runtime_module.waitNativeOperation(operation, diagnosticStore(self));
        var bounds: c.mln_lat_lng_bounds = undefined;
        try status.checkStatus(take(operation, &bounds), diagnosticStore(self));
        return values.latLngBoundsFromNative(bounds);
    }

    pub fn setBounds(self: *MapHandle, options: values.BoundOptions) status.Error!u64 {
        var raw_options = values.boundOptionsToNative(options);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_bounds(try native(self), &raw_options, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn setFreeCameraOptions(self: *MapHandle, options: values.FreeCameraOptions) status.Error!u64 {
        var raw_options = values.freeCameraOptionsToNative(options);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_free_camera_options(try native(self), &raw_options, &command_id), diagnosticStore(self));
        return command_id;
    }

    /// Selects which map-originated event types this map queues.
    ///
    /// A map reads the fields `RuntimeEventMask.all_map_events` names and
    /// ignores the rest. An unselected type is never queued, so select every
    /// type the host reads: render-update-available is the map's only
    /// invalidation report, the still-image types are the only reports that a
    /// still-image request finished, and the camera and loading types carry
    /// transition identity and native failure text.
    ///
    /// Narrowing gates later events and keeps queued ones, so a host drains what
    /// it already caused.
    pub fn setEventMask(self: *MapHandle, mask: runtime_module.RuntimeEventMask) status.Error!u64 {
        const map_state = try mapStateForHandle(self);
        var command_id: u64 = 0;
        try status.checkStatus(c.mln_map_set_event_mask(try native(self), runtime_module.eventMaskToRaw(mask), &command_id), map_state.diagnostic_store);
        return command_id;
    }

    pub fn eventMask(self: *MapHandle) status.Error!runtime_module.RuntimeEventMask {
        return (try self.snapshot()).event_mask;
    }

    fn wrapStyleOperation(
        self: *MapHandle,
        operation: c.mln_operation,
        operation_kind: runtime_module.OperationKind,
        result_kind: runtime_module.OperationResultKind,
    ) status.Error!runtime_module.OperationHandle {
        const state = try mapStateForHandle(self);
        return runtime_module.OperationHandle.init(&state.runtime, operation, operation_kind, result_kind) catch |err| {
            c.mln_operation_release(operation);
            return err;
        };
    }

    fn startStyleOperation(
        self: *MapHandle,
        start: *const fn (c.mln_map, *c.mln_operation) callconv(.c) c.mln_status,
        operation_kind: runtime_module.OperationKind,
        result_kind: runtime_module.OperationResultKind,
    ) status.Error!runtime_module.OperationHandle {
        var operation: c.mln_operation = 0;
        try status.checkStatus(start(try native(self), &operation), diagnosticStore(self));
        return self.wrapStyleOperation(operation, operation_kind, result_kind);
    }

    fn startStyleOperationWithId(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        input_id: []const u8,
        start: *const fn (c.mln_map, c.mln_buffer_view, *c.mln_operation) callconv(.c) c.mln_status,
        operation_kind: runtime_module.OperationKind,
        result_kind: runtime_module.OperationResultKind,
    ) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var operation: c.mln_operation = 0;
        try status.checkStatus(start(try native(self), try temp.stringView(input_id), &operation), diagnosticStore(self));
        return self.wrapStyleOperation(operation, operation_kind, result_kind);
    }

    fn takeOptionalStyleBuffer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
        operation_kind: runtime_module.OperationKind,
        take: *const fn (c.mln_operation, *c.mln_buffer) callconv(.c) c.mln_status,
    ) status.Error!?values.OwnedString {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, operation_kind, .optional_string);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        try status.checkStatus(take(required.lease.native, &buffer), state.diagnostic_store);
        return native_temp.copyOwnedBuffer(allocator, buffer, state.diagnostic_store);
    }
    fn takeFoundStyleBuffer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
        operation_kind: runtime_module.OperationKind,
        take: *const fn (c.mln_operation, *c.mln_buffer, *bool) callconv(.c) c.mln_status,
    ) status.Error!?values.OwnedString {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, operation_kind, .optional_string);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        var found = false;
        try status.checkStatus(take(required.lease.native, &buffer, &found), state.diagnostic_store);
        if (!found) return null;
        return (try native_temp.copyOwnedBuffer(allocator, buffer, state.diagnostic_store)) orelse error.NativeError;
    }

    fn removeStyleObjectCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        object_id: []const u8,
        command: *const fn (c.mln_map, c.mln_buffer_view, *u64) callconv(.c) c.mln_status,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(command(try native(self), try temp.stringView(object_id), &command_id), diagnosticStore(self));
        return command_id;
    }

    fn takeStyleBuffer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        operation: runtime_module.OperationHandle,
        operation_kind: runtime_module.OperationKind,
        take: *const fn (c.mln_operation, *c.mln_buffer) callconv(.c) c.mln_status,
    ) status.Error!values.OwnedString {
        const state = try mapStateForHandle(self);
        const required = try operation.require(&state.runtime, operation_kind, .string);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        try status.checkStatus(take(required.lease.native, &buffer), state.diagnostic_store);
        return (try native_temp.copyOwnedBuffer(allocator, buffer, state.diagnostic_store)) orelse error.NativeError;
    }

    fn setLayerStringCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        value: []const u8,
        command: *const fn (c.mln_map, c.mln_buffer_view, c.mln_buffer_view, *u64) callconv(.c) c.mln_status,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(command(
            try native(self),
            try temp.stringView(layer_id),
            try temp.stringView(value),
            &command_id,
        ), diagnosticStore(self));
        return command_id;
    }

    fn setLayerNumberCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        value: f64,
        command: *const fn (c.mln_map, c.mln_buffer_view, f64, *u64) callconv(.c) c.mln_status,
    ) status.Error!u64 {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var command_id: u64 = 0;
        try status.checkStatus(command(try native(self), try temp.stringView(layer_id), value, &command_id), diagnosticStore(self));
        return command_id;
    }

    pub fn close(self: *MapHandle) status.Error!void {
        const map_close = beginMapClose(self.*) catch |err| {
            if (err == error.InvalidState) {
                if (diagnosticStore(self)) |store| {
                    try status.setBindingDiagnostic(store, "map has an attached render session");
                }
            }
            return err;
        } orelse return;
        var operation: c.mln_operation = 0;
        status.checkStatus(c.mln_map_close_start(map_close.native, &operation), map_close.diagnostic_store) catch |err| {
            cancelMapClose(map_close.state);
            return err;
        };
        defer c.mln_operation_release(operation);
        runtime_module.waitNativeOperation(operation, map_close.diagnostic_store) catch |err| {
            cancelMapClose(map_close.state);
            return err;
        };
        runtime_module.unregisterMap(map_close.runtime_registry, map_close.native);
        const map_state = finishMapClose(self.*) orelse map_close.state;
        std.heap.smp_allocator.destroy(map_state);
    }
};

fn styleTileSourceOptionsToNative(
    temp: *native_temp.TempStorage,
    options: values.StyleTileSourceOptions,
) status.Error!c.mln_style_tile_source_options {
    var raw = c.mln_style_tile_source_options_default();
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (options.attribution) |attribution| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
        raw.attribution = try temp.stringView(attribution);
    }
    if (options.scheme) |scheme| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
        raw.scheme = values.styleTileSchemeToNative(scheme);
    }
    if (options.bounds) |bounds| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
        raw.bounds = values.latLngBoundsToNative(bounds);
    }
    if (options.tile_size) |tile_size| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if (options.vector_encoding) |encoding| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
        raw.vector_encoding = values.styleVectorTileEncodingToNative(encoding);
    }
    if (options.raster_encoding) |encoding| {
        raw.fields |= c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
        raw.raster_encoding = values.styleRasterDemEncodingToNative(encoding);
    }
    return raw;
}

fn styleGeoJsonSourceOptionsToNative(
    temp: *native_temp.TempStorage,
    options: values.StyleGeoJsonSourceOptions,
) status.Error!c.mln_geojson_source_options {
    var raw = c.mln_geojson_source_options_default();
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (options.tolerance) |tolerance| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
        raw.tolerance = tolerance;
    }
    if (options.cluster_max_zoom) |cluster_max_zoom| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
        raw.cluster_max_zoom = cluster_max_zoom;
    }
    if (options.cluster_properties) |cluster_properties| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
        raw.cluster_properties = try temp.stringView(cluster_properties);
    }
    if (options.tile_size) |tile_size| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if (options.buffer) |buffer| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_BUFFER;
        raw.buffer = buffer;
    }
    if (options.cluster_radius) |cluster_radius| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
        raw.cluster_radius = cluster_radius;
    }
    if (options.cluster_min_points) |cluster_min_points| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
        raw.cluster_min_points = cluster_min_points;
    }
    if (options.line_metrics) |line_metrics| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
        raw.line_metrics = line_metrics;
    }
    if (options.cluster) |cluster| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
        raw.cluster = cluster;
    }
    if (options.synchronous_tiling) |synchronous_tiling| {
        raw.fields |= c.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
        raw.synchronous_tiling = synchronous_tiling;
    }
    return raw;
}

fn customGeometrySourceOptionsToNative(
    options: CustomGeometrySourceOptions,
    source_state: *CustomGeometrySourceState,
) c.mln_custom_geometry_source_options {
    var raw = c.mln_custom_geometry_source_options_default();
    raw.fetch_tile = customGeometryFetchTileTrampoline;
    raw.cancel_tile = if (options.cancel_tile != null) customGeometryCancelTileTrampoline else null;
    raw.release_user_data = customGeometryReleaseTrampoline;
    raw.user_data = source_state;
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    if (options.tolerance) |tolerance| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
        raw.tolerance = tolerance;
    }
    if (options.tile_size) |tile_size| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
        raw.tile_size = tile_size;
    }
    if (options.buffer) |buffer| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
        raw.buffer = buffer;
    }
    if (options.clip) |clip| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
        raw.clip = clip;
    }
    if (options.wrap) |wrap| {
        raw.fields |= c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
        raw.wrap = wrap;
    }
    return raw;
}

fn customGeometryFetchTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer endCustomGeometryUpcall(source_state);

    source_state.fetch_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn customGeometryCancelTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer endCustomGeometryUpcall(source_state);

    const cancel_tile = source_state.cancel_tile orelse return;
    cancel_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn beginCustomGeometryUpcall(source_state: *CustomGeometrySourceState) bool {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);

    for (custom_geometry_state_registry.items) |live_state| {
        if (live_state == source_state) {
            _ = source_state.active_upcalls.fetchAdd(1, .seq_cst);
            return true;
        }
    }
    return false;
}

fn endCustomGeometryUpcall(source_state: *CustomGeometrySourceState) void {
    _ = source_state.active_upcalls.fetchSub(1, .seq_cst);
}

fn canonicalTileIdToNative(tile_id: CanonicalTileId) c.mln_canonical_tile_id {
    return .{ .z = tile_id.z, .x = tile_id.x, .y = tile_id.y };
}

fn canonicalTileIdFromNative(tile_id: c.mln_canonical_tile_id) CanonicalTileId {
    return .{ .z = tile_id.z, .x = tile_id.x, .y = tile_id.y };
}

// The C API invokes this once on the runtime worker after it stops
// referencing the state: on an explicit removal, when a style load leaves a
// style without the source, and when the map is destroyed.
fn customGeometryReleaseTrampoline(user_data: ?*anyopaque) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    freeCustomGeometrySourceState(source_state);
}

fn freeCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    retireLiveCustomGeometrySourceState(source_state);
    waitForCustomGeometryUpcalls(source_state);
    if (source_state.release_context) |release| release(source_state.context);
    std.heap.smp_allocator.destroy(source_state);
}

fn registerLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) std.mem.Allocator.Error!void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    try custom_geometry_state_registry.append(std.heap.smp_allocator, source_state);
}

fn unregisterLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    removeLiveCustomGeometrySourceStateLocked(source_state);
}

fn retireLiveCustomGeometrySourceState(source_state: *CustomGeometrySourceState) void {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    removeLiveCustomGeometrySourceStateLocked(source_state);
}

fn removeLiveCustomGeometrySourceStateLocked(source_state: *CustomGeometrySourceState) void {
    for (custom_geometry_state_registry.items, 0..) |live_state, index| {
        if (live_state == source_state) {
            _ = custom_geometry_state_registry.orderedRemove(index);
            return;
        }
    }
}

fn waitForCustomGeometryUpcalls(source_state: *CustomGeometrySourceState) void {
    while (source_state.active_upcalls.load(.seq_cst) != 0) {
        std.Thread.yield() catch {};
    }
}

fn mapStateForHandle(handle: *MapHandle) status.BindingError!*MapState {
    return mapState(handle.*) orelse error.ClosedHandle;
}

fn mapIdForHandle(handle: *MapHandle) status.BindingError!values.MapId {
    lockMapRegistry();
    defer unlockMapRegistry();

    const map_state = mapStateLocked(handle.*) orelse return error.ClosedHandle;
    return map_state.id_value;
}

fn registerMapState(map: c.mln_map, map_state: *MapState) std.mem.Allocator.Error!MapHandle {
    lockMapRegistry();
    defer unlockMapRegistry();

    try map_registry.put(std.heap.smp_allocator, map, map_state);
    return @enumFromInt(map);
}

fn mapState(handle: MapHandle) ?*MapState {
    lockMapRegistry();
    defer unlockMapRegistry();
    return mapStateLocked(handle);
}

fn mapStateLocked(handle: MapHandle) ?*MapState {
    return map_registry.get(@intFromEnum(handle));
}

const MapClose = struct {
    state: *MapState,
    native: c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    runtime_registry: *runtime_module.RuntimeRegistry,
};

fn beginMapClose(handle: MapHandle) status.BindingError!?MapClose {
    lockMapRegistry();
    defer unlockMapRegistry();

    const map_state = map_registry.get(@intFromEnum(handle)) orelse return null;
    if (map_state.closing) return error.ActiveBorrow;
    if (map_state.attached_render_sessions.load(.seq_cst) != 0) return error.InvalidState;
    map_state.closing = true;
    return .{
        .state = map_state,
        .native = @intFromEnum(handle),
        .diagnostic_store = map_state.diagnostic_store,
        .runtime_registry = map_state.runtime_registry,
    };
}

fn cancelMapClose(map_state: *MapState) void {
    lockMapRegistry();
    defer unlockMapRegistry();

    map_state.closing = false;
}

fn finishMapClose(handle: MapHandle) ?*MapState {
    lockMapRegistry();
    defer unlockMapRegistry();

    const entry = map_registry.fetchRemove(@intFromEnum(handle)) orelse return null;
    return entry.value;
}

fn lockMapRegistry() void {
    while (map_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockMapRegistry() void {
    map_registry_lock.store(false, .seq_cst);
}

pub fn native(handle: *MapHandle) status.BindingError!c.mln_map {
    const map_state = mapState(handle.*) orelse return error.ClosedHandle;
    if (map_state.closing) return error.ActiveBorrow;
    return @intFromEnum(handle.*);
}

pub fn diagnosticStore(handle: *MapHandle) ?*diagnostics.DiagnosticStore {
    const map_state = mapState(handle.*) orelse return null;
    return map_state.diagnostic_store;
}

pub fn registerRenderSession(handle: *MapHandle) status.BindingError!RenderSessionRegistration {
    lockMapRegistry();
    defer unlockMapRegistry();

    const map_state = mapStateLocked(handle.*) orelse return error.ClosedHandle;
    if (map_state.closing) return error.ActiveBorrow;
    _ = map_state.attached_render_sessions.fetchAdd(1, .seq_cst);
    return .{
        .native = @intFromEnum(handle.*),
        .diagnostic_store = map_state.diagnostic_store,
        .runtime = map_state.runtime,
    };
}

pub fn unregisterRenderSession(handle: MapHandle) void {
    const map_state = mapState(handle) orelse return;
    _ = map_state.attached_render_sessions.fetchSub(1, .seq_cst);
}

// Callback states the tile trampolines still route through. The C API's release
// callback is what retires one, so a released state leaves this count.
fn liveCustomGeometrySourceCountForTesting() usize {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);
    return custom_geometry_state_registry.items.len;
}

fn copyStyleIdList(
    allocator: std.mem.Allocator,
    list: c.mln_style_id_list,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!values.StringList {
    var count: usize = 0;
    try status.checkStatus(c.mln_style_id_list_count(list, &count), diagnostic_store);
    const items = try allocator.alloc([]const u8, count);
    var initialized: usize = 0;
    errdefer {
        for (items[0..initialized]) |item| allocator.free(item);
        allocator.free(items);
    }
    for (items, 0..) |*item, index| {
        var view = c.mln_buffer_view{ .data = null, .size = 0 };
        try status.checkStatus(c.mln_style_id_list_get(list, index, &view), diagnostic_store);
        item.* = if (view.size == 0) try allocator.dupe(u8, "") else blk: {
            const data: [*]const u8 = @ptrCast(view.data orelse return error.NativeError);
            break :blk try allocator.dupe(u8, data[0..view.size]);
        };
        initialized += 1;
    }
    return .{ .allocator = allocator, .items = items };
}

fn copyStyleStringList(
    allocator: std.mem.Allocator,
    list: c.mln_style_string_list,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!values.StringList {
    var count: usize = 0;
    try status.checkStatus(c.mln_style_string_list_count(list, &count), diagnostic_store);
    const items = try allocator.alloc([]const u8, count);
    var initialized: usize = 0;
    errdefer {
        for (items[0..initialized]) |item| allocator.free(item);
        allocator.free(items);
    }
    for (items, 0..) |*item, index| {
        var view = c.mln_buffer_view{ .data = null, .size = 0 };
        try status.checkStatus(c.mln_style_string_list_get(list, index, &view), diagnostic_store);
        item.* = if (view.size == 0) try allocator.dupe(u8, "") else blk: {
            const data: [*]const u8 = @ptrCast(view.data orelse return error.NativeError);
            break :blk try allocator.dupe(u8, data[0..view.size]);
        };
        initialized += 1;
    }
    return .{ .allocator = allocator, .items = items };
}

fn stringView(value: []const u8) c.mln_buffer_view {
    return .{ .data = if (value.len == 0) null else value.ptr, .size = value.len };
}

fn nulTerminated(
    allocator: std.mem.Allocator,
    value: []const u8,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    diagnostic_message: []const u8,
) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) {
        try status.setBindingDiagnostic(diagnostic_store, diagnostic_message);
        return error.InvalidString;
    }
    return allocator.dupeZ(u8, value);
}

const TestCustomGeometryCallbackState = struct {
    fetch_count: usize = 0,
    cancel_count: usize = 0,
    release_count: usize = 0,
    last_tile: CanonicalTileId = .{ .z = 0, .x = 0, .y = 0 },
};

fn testReleaseCustomGeometryContext(context: ?*anyopaque) void {
    const test_state: *TestCustomGeometryCallbackState = @ptrCast(@alignCast(context.?));
    test_state.release_count += 1;
}

fn testFetchCustomGeometryTile(context: ?*anyopaque, tile_id: CanonicalTileId) void {
    const test_state: *TestCustomGeometryCallbackState = @ptrCast(@alignCast(context.?));
    test_state.fetch_count += 1;
    test_state.last_tile = tile_id;
}

fn testCancelCustomGeometryTile(context: ?*anyopaque, tile_id: CanonicalTileId) void {
    const test_state: *TestCustomGeometryCallbackState = @ptrCast(@alignCast(context.?));
    test_state.cancel_count += 1;
    test_state.last_tile = tile_id;
}

test "custom geometry trampolines route semantic tile ids" {
    var test_state = TestCustomGeometryCallbackState{};
    var source_state = CustomGeometrySourceState{
        .fetch_tile = testFetchCustomGeometryTile,
        .cancel_tile = testCancelCustomGeometryTile,
        .release_context = null,
        .context = &test_state,
        .active_upcalls = std.atomic.Value(usize).init(0),
    };
    try registerLiveCustomGeometrySourceState(&source_state);
    defer unregisterLiveCustomGeometrySourceState(&source_state);

    customGeometryFetchTileTrampoline(&source_state, .{ .z = 3, .x = 4, .y = 5 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 3, .x = 4, .y = 5 }, test_state.last_tile);

    customGeometryCancelTileTrampoline(&source_state, .{ .z = 6, .x = 7, .y = 8 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);
    try std.testing.expectEqual(CanonicalTileId{ .z = 6, .x = 7, .y = 8 }, test_state.last_tile);

    source_state.cancel_tile = null;
    customGeometryCancelTileTrampoline(&source_state, .{ .z = 9, .x = 10, .y = 11 });
    try std.testing.expectEqual(@as(usize, 1), test_state.cancel_count);

    retireLiveCustomGeometrySourceState(&source_state);
    customGeometryFetchTileTrampoline(&source_state, .{ .z = 12, .x = 13, .y = 14 });
    try std.testing.expectEqual(@as(usize, 1), test_state.fetch_count);
    try std.testing.expectEqual(@as(usize, 0), source_state.active_upcalls.load(.seq_cst));
}

const test_style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {
    \\    "point": {
    \\      "type": "geojson",
    \\      "data": {"type":"FeatureCollection","features":[]}
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

fn createLoadedMapForTesting(runtime: *RuntimeHandle) !MapHandle {
    var map = try MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    _ = try map.setStyleJson(std.testing.allocator, test_style_json);
    try std.testing.expect(try waitForRuntimeEventForTesting(runtime, .map_style_loaded));
    return map;
}

fn waitForCommandDispositionForTesting(runtime: *RuntimeHandle, command_id: u64) !runtime_module.CommandDisposition {
    var attempts: usize = 0;
    while (attempts < 200) : (attempts += 1) {
        while (true) {
            var batch = try runtime.drainEvents(std.testing.allocator, 1);
            defer batch.deinit();
            if (batch.len() == 0) break;
            const event = try batch.at(0);
            switch (event.payload) {
                .command_finished => |payload| {
                    if (payload.command_id == command_id) return payload.disposition;
                },
                else => {},
            }
        }
        try std.testing.io.sleep(.fromMilliseconds(10), .awake);
    }
    return error.EventNotObserved;
}

fn waitForRuntimeEventForTesting(runtime: *RuntimeHandle, event_type: runtime_module.RuntimeEventType) !bool {
    var attempts: usize = 0;
    while (attempts < 200) : (attempts += 1) {
        // One event per drain, so an event this wait is not looking for stays
        // queued rather than being dropped with the batch that carried it.
        while (true) {
            var batch = try runtime.drainEvents(std.testing.allocator, 1);
            defer batch.deinit();
            if (batch.len() == 0) break;
            const event = try batch.at(0);
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try std.testing.io.sleep(.fromMilliseconds(10), .awake);
    }
    return false;
}

fn testStyleJsonProvider(
    context: ?*anyopaque,
    request: runtime_module.ResourceRequest,
    maybe_handle: ?runtime_module.ResourceRequestHandle,
) runtime_module.ResourceProviderDecision {
    _ = context;
    if (!std.mem.eql(u8, request.requested_url, "custom://style.json")) return .pass_through;
    const handle = maybe_handle orelse return .pass_through;
    handle.complete(.{ .bytes = test_style_json }) catch {
        handle.release();
        return .pass_through;
    };
    handle.release();
    return .handle;
}

// The C API releases a source's callback state once it stops referencing it, so
// the tests below watch the live-state count rather than a style load.
test "an explicit source removal releases the callback state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");

    const baseline = liveCustomGeometrySourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    _ = try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    });
    try std.testing.expectEqual(baseline + 1, liveCustomGeometrySourceCountForTesting());

    const remove_id = try map.removeStyleSource(std.testing.allocator, "custom");
    const disposition = try waitForCommandDispositionForTesting(&runtime, remove_id);
    try std.testing.expect(std.meta.eql(disposition, runtime_module.CommandDisposition.committed));
    try std.testing.expectEqual(baseline, liveCustomGeometrySourceCountForTesting());
}

// A map whose mask clears style-loaded still releases the state, because the
// release runs on the C API's own reconciliation rather than on an event this
// binding subscribes to.
test "a style load that drops a source releases the callback state unsubscribed" {
    const narrowed = blk: {
        var mask = runtime_module.RuntimeEventMask.all;
        mask.map_style_loaded = false;
        break :blk mask;
    };

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    _ = try runtime.setResourceProvider(.{ .handler = testStyleJsonProvider });

    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");
    _ = try map.setEventMask(narrowed);
    const mask_barrier = try runtime.barrierStart();
    defer mask_barrier.release();
    try std.testing.expect(try mask_barrier.wait(-1));

    const baseline = liveCustomGeometrySourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    _ = try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .release_context = testReleaseCustomGeometryContext,
        .context = &state,
    });
    try std.testing.expectEqual(baseline + 1, liveCustomGeometrySourceCountForTesting());
    try std.testing.expectEqual(@as(usize, 0), state.release_count);
    // The mask stays what the host set, because the binding adds nothing to it.
    try std.testing.expectEqual(narrowed, try map.eventMask());

    _ = try map.setStyleUrl(std.testing.allocator, "custom://style.json");
    var released = false;
    for (0..200) |_| {
        var batch = try runtime.drainEvents(std.testing.allocator, 0);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            try std.testing.expect(!std.meta.eql(
                event.event_type,
                runtime_module.RuntimeEventType.map_style_loaded,
            ));
        }
        if (liveCustomGeometrySourceCountForTesting() == baseline) {
            released = true;
            break;
        }
        try std.testing.io.sleep(.fromMilliseconds(10), .awake);
    }
    try std.testing.expect(released);
    // The host's own release runs once, which is the signal it would otherwise
    // have to reconstruct from style-loaded events.
    try std.testing.expectEqual(@as(usize, 1), state.release_count);
}

test "closing a map releases its surviving source callback states" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    var map_open = true;
    defer if (map_open) map.close() catch @panic("map close failed");

    const baseline = liveCustomGeometrySourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    _ = try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    });
    try std.testing.expectEqual(baseline + 1, liveCustomGeometrySourceCountForTesting());

    try map.close();
    map_open = false;
    try std.testing.expectEqual(baseline, liveCustomGeometrySourceCountForTesting());
}

// A rejected add releases nothing, so the state belongs to the failing call.
test "a rejected custom geometry source add releases its own callback state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer map.close() catch @panic("map close failed");

    const baseline = liveCustomGeometrySourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    try std.testing.expectError(error.InvalidArgument, map.addCustomGeometrySource(
        std.testing.allocator,
        "bad-zoom",
        .{
            .fetch_tile = testFetchCustomGeometryTile,
            .release_context = testReleaseCustomGeometryContext,
            .context = &state,
            .min_zoom = -1,
        },
    ));
    try std.testing.expectEqual(baseline, liveCustomGeometrySourceCountForTesting());
    try std.testing.expectEqual(@as(usize, 0), state.release_count);
}

test "a released map id is stale and map snapshots work from another thread" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    var first = try MapHandle.create(&runtime, .{});
    const released = @intFromEnum(first);
    try first.close();

    var stale_snapshot = std.mem.zeroes(c.mln_map_snapshot);
    stale_snapshot.size = @sizeOf(c.mln_map_snapshot);
    try std.testing.expectError(
        error.InvalidArgument,
        status.checkStatus(c.mln_map_snapshot_get(released, &stale_snapshot), null),
    );

    var second = try MapHandle.create(&runtime, .{});
    defer second.close() catch @panic("map close failed");

    const CrossThread = struct {
        map: *MapHandle,
        result: status.Error!MapSnapshot = undefined,

        fn run(self: *@This()) void {
            self.result = self.map.snapshot();
        }
    };

    var context = CrossThread{ .map = &second };
    const thread = try std.Thread.spawn(.{}, CrossThread.run, .{&context});
    thread.join();
    _ = try context.result;
}
