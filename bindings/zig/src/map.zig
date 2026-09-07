const std = @import("std");

const c = @import("c.zig").raw;
const completion = @import("completion.zig");
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const RuntimeHandle = runtime_module.RuntimeHandle;
const status = @import("status.zig");
const sync = @import("sync.zig");
const values = @import("values.zig");

const CustomGeometrySourceState = struct {
    fetch_tile: CustomGeometrySourceTileCallback,
    cancel_tile: ?CustomGeometrySourceTileCallback,
    release_context: ?CustomGeometrySourceReleaseCallback,
    context: ?*anyopaque,
    upcalls: sync.UpcallGate = .{},
};

const CustomMvtVectorSourceState = struct {
    fetch_tile: CustomMvtVectorSourceTileCallback,
    cancel_tile: ?CustomMvtVectorSourceTileCallback,
    release_context: ?CustomMvtVectorSourceReleaseCallback,
    context: ?*anyopaque,
    upcalls: sync.UpcallGate = .{},
};

const MapState = struct {
    runtime_registry: *runtime_module.RuntimeRegistry,
    id_value: values.MapId,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    attached_render_sessions: std.atomic.Value(usize),
    closing: bool,
};

pub const RenderSessionRegistration = struct {
    native: c.mln_map,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

var custom_geometry_state_registry_lock = std.Io.Mutex.init;
var custom_geometry_state_registry: std.ArrayList(*CustomGeometrySourceState) = .empty;
var custom_mvt_vector_state_registry_lock = std.Io.Mutex.init;
var custom_mvt_vector_state_registry: std.ArrayList(*CustomMvtVectorSourceState) = .empty;

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
};

pub const CameraDeltaKind = enum {
    move,
    scale,
    bearing,
    pitch,

    fn toRaw(self: CameraDeltaKind) u32 {
        return switch (self) {
            .move => c.MLN_CAMERA_DELTA_MOVE,
            .scale => c.MLN_CAMERA_DELTA_SCALE,
            .bearing => c.MLN_CAMERA_DELTA_BEARING,
            .pitch => c.MLN_CAMERA_DELTA_PITCH,
        };
    }
};

pub const CameraDelta = struct {
    kind: CameraDeltaKind = .move,
    offset: values.ScreenPoint = .{ .x = 0, .y = 0 },
    amount: f64 = 0,
    anchor: ?values.ScreenPoint = null,
    animation: values.AnimationOptions = .{},
};

pub const CameraSnapshot = struct {
    generation: u64,
    camera: values.CameraOptions,
};

pub const ScreenPointList = struct {
    allocator: std.mem.Allocator,
    items: []values.ScreenPoint,

    pub fn deinit(self: *ScreenPointList) void {
        self.allocator.free(self.items);
        self.items = &.{};
    }
};

pub const LatLngList = struct {
    allocator: std.mem.Allocator,
    items: []values.LatLng,

    pub fn deinit(self: *LatLngList) void {
        self.allocator.free(self.items);
        self.items = &.{};
    }
};

/// Immutable map state copied from the latest published generation.
///
/// Every committed map command publishes a new generation in its completion,
/// so a snapshot whose generation is at or past that value observes the
/// commit.
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
    /// True between a gesture-phase begin and its end or cancel.
    gesture_in_progress: bool,
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
/// Fixed layer metadata copied by `MapHandle.getStyleLayerInfo`.
pub const StyleLayerInfo = struct {
    allocator: std.mem.Allocator,
    /// Style-spec layer type name. It views a static native string that stays
    /// valid for the life of the process.
    layer_type: []const u8,
    /// Lowest zoom at which the layer draws; -inf with no lower bound.
    min_zoom: f64,
    /// Highest zoom at which the layer draws; +inf with no upper bound.
    max_zoom: f64,
    visibility: values.StyleLayerVisibility,
    /// Source ID, null for a layer type that takes no source.
    source_id: ?[]const u8,
    /// Source-layer ID, null when the layer sets none.
    source_layer: ?[]const u8,

    pub fn deinit(self: *StyleLayerInfo) void {
        if (self.source_id) |value| self.allocator.free(value);
        if (self.source_layer) |value| self.allocator.free(value);
        self.source_id = null;
        self.source_layer = null;
    }
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

pub const CustomMvtVectorSourceTileCallback = *const fn (
    context: ?*anyopaque,
    tile_id: CanonicalTileId,
) void;

pub const CustomMvtVectorSourceReleaseCallback = *const fn (context: ?*anyopaque) void;

/// Options for `MapHandle.addCustomMvtVectorSource`.
pub const CustomMvtVectorSourceOptions = struct {
    fetch_tile: CustomMvtVectorSourceTileCallback,
    cancel_tile: ?CustomMvtVectorSourceTileCallback = null,
    /// Invoked once with `context` after the map stops referencing this source:
    /// on an explicit removal, on a style load that leaves a style without the
    /// source, and on the map's own destruction. It runs on the runtime worker
    /// after the last tile callback returns, and never runs for an add that
    /// failed. A host frees `context` here instead of tracking style loads.
    release_context: ?CustomMvtVectorSourceReleaseCallback = null,
    context: ?*anyopaque = null,
    min_zoom: ?f64 = null,
    max_zoom: ?f64 = null,
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

/// Feature-state source, feature, and key selector.
pub const FeatureStateSelector = struct {
    source_id: []const u8,
    source_layer_id: ?[]const u8 = null,
    feature_id: ?[]const u8 = null,
    state_key: ?[]const u8 = null,
};

pub const MapHandle = enum(c.mln_map) {
    _,

    pub fn create(runtime: *RuntimeHandle, options: MapOptions) status.Error!completion.Future(MapHandle) {
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

        const diagnostic_store = runtime_lease.diagnostic_store;
        const CopyContext = struct {
            runtime: RuntimeHandle,
            diagnostic_store: ?*diagnostics.DiagnosticStore,
        };
        const StartContext = struct { runtime: c.mln_runtime, options: c.mln_map_options };
        return completion.submitWithCopyContext(MapHandle, CopyContext, diagnostic_store, struct {
            fn copyResult(result: *const c.mln_completion_result, context: *CopyContext) status.Error!MapHandle {
                const map = try completion.value(c.mln_map)(result);
                // A map this binding cannot track would otherwise stay live and
                // hold its runtime open forever, so a failed registration
                // releases it.
                errdefer releaseUntrackedMap(map);
                var runtime_handle = context.runtime;
                const registration = try runtime_module.registerMap(&runtime_handle, map);
                errdefer runtime_module.unregisterMap(registration.registry, map);
                const state = try std.heap.smp_allocator.create(MapState);
                state.* = .{
                    .runtime_registry = registration.registry,
                    .id_value = registration.id,
                    .diagnostic_store = context.diagnostic_store,
                    .attached_render_sessions = std.atomic.Value(usize).init(0),
                    .closing = false,
                };
                errdefer std.heap.smp_allocator.destroy(state);
                return registerMapState(map, state);
            }
        }.copyResult, .{
            .runtime = runtime.*,
            .diagnostic_store = diagnostic_store,
        }, StartContext{
            .runtime = runtime_lease.native,
            .options = native_options,
        }, struct {
            fn start(context: StartContext, descriptor: *const c.mln_completion) c.mln_status {
                return c.mln_map_create(context.runtime, &context.options, descriptor);
            }
        }.start);
    }

    pub fn id(self: *MapHandle) status.BindingError!values.MapId {
        return mapIdForHandle(self);
    }

    pub fn setStyleJson(
        self: *MapHandle,
        json: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_set_style_json, .{ try native(self), stringView(json) });
    }

    /// The committed command requests a map repaint.
    pub fn setFeatureState(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
        feature_state: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        return submitCommand(self, c.mln_map_set_feature_state, .{
            try native(self),
            &raw_selector,
            try temp.stringView(feature_state),
        });
    }

    /// Starts an ordered read of per-feature state. The read copies the map
    /// store, not the last rendered frame; missing feature state resolves to an
    /// empty JSON object.
    pub fn getFeatureState(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
    ) status.Error!completion.Future(values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        return submitAllocatedQuery(values.OwnedString, self, allocator, copyOwnedStringResult, c.mln_map_get_feature_state, .{
            try native(self),
            &raw_selector,
        });
    }

    /// The committed command requests a map repaint.
    pub fn removeFeatureState(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        return submitCommand(self, c.mln_map_remove_feature_state, .{
            try native(self),
            &raw_selector,
        });
    }

    pub fn setStyleUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        url: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        const url_z = try nulTerminated(allocator, url, diagnosticStore(self), "style URL contains embedded NUL");
        defer allocator.free(url_z);
        return submitCommand(self, c.mln_map_set_style_url, .{ try native(self), url_z.ptr });
    }

    pub fn loadedStyleJson(self: *MapHandle, allocator: std.mem.Allocator) status.Error!completion.Future(values.OwnedString) {
        return submitAllocatedQuery(values.OwnedString, self, allocator, copyOwnedStringResult, c.mln_map_loaded_style_json, .{try native(self)});
    }

    pub fn styleUrl(self: *MapHandle, allocator: std.mem.Allocator) status.Error!completion.Future(values.OwnedString) {
        return submitAllocatedQuery(values.OwnedString, self, allocator, copyOwnedStringResult, c.mln_map_style_url, .{try native(self)});
    }

    pub fn setLayerProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
        value: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_layer_property, .{
            try native(self),
            try temp.stringView(layer_id),
            try temp.stringView(property_name),
            try temp.stringView(value),
        });
    }

    pub fn getLayerProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        property_name: []const u8,
    ) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_get_layer_property, .{
            try native(self), try temp.stringView(layer_id), try temp.stringView(property_name),
        });
    }

    pub fn setLayerFilter(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        filter: ?[]const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var filter_view = if (filter) |value| try temp.stringView(value) else undefined;
        const filter_ptr = if (filter != null) &filter_view else null;
        return submitCommand(self, c.mln_map_set_layer_filter, .{
            try native(self),
            try temp.stringView(layer_id),
            filter_ptr,
        });
    }

    pub fn getLayerFilter(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_get_layer_filter, .{ try native(self), try temp.stringView(layer_id) });
    }

    pub fn setLayerSourceLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_layer: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        return self.setLayerStringCommand(allocator, layer_id, source_layer, c.mln_map_set_layer_source_layer);
    }

    pub fn copyLayerSourceLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!completion.Future(values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(values.OwnedString, self, allocator, copyOwnedStringResult, c.mln_map_copy_layer_source_layer, .{ try native(self), try temp.stringView(layer_id) });
    }

    pub fn setLayerSourceId(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        return self.setLayerStringCommand(allocator, layer_id, source_id, c.mln_map_set_layer_source_id);
    }

    pub fn copyLayerSourceId(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
    ) status.Error!completion.Future(values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(values.OwnedString, self, allocator, copyOwnedStringResult, c.mln_map_copy_layer_source_id, .{ try native(self), try temp.stringView(layer_id) });
    }

    pub fn setLayerMinZoom(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8, value: f64) status.Error!completion.Future(completion.CommandCompletion) {
        return self.setLayerNumberCommand(allocator, layer_id, value, c.mln_map_set_layer_min_zoom);
    }

    pub fn setLayerMaxZoom(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8, value: f64) status.Error!completion.Future(completion.CommandCompletion) {
        return self.setLayerNumberCommand(allocator, layer_id, value, c.mln_map_set_layer_max_zoom);
    }

    pub fn setLayerVisibility(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        visibility: values.StyleLayerVisibility,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_layer_visibility, .{
            try native(self),
            try temp.stringView(layer_id),
            visibility.toRaw(),
        });
    }

    /// Copies fixed layer metadata, completing with null when no layer has the
    /// ID.
    pub fn getStyleLayerInfo(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!completion.Future(?StyleLayerInfo) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?StyleLayerInfo, self, allocator, struct {
            fn copyResult(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!?StyleLayerInfo {
                const raw_result = (try optionalCompletionValue(c.mln_style_layer_result, result)) orelse return null;
                const raw = raw_result.info;
                const type_data: [*]const u8 = @ptrCast(raw.type.data orelse return error.NativeError);
                const source_id = try copyPresentView(target.*, raw_result.source_id);
                errdefer if (source_id) |owned| target.free(owned);
                const source_layer = try copyPresentView(target.*, raw_result.source_layer);
                return .{
                    .allocator = target.*,
                    .layer_type = type_data[0..raw.type.size],
                    .min_zoom = raw.min_zoom,
                    .max_zoom = raw.max_zoom,
                    .visibility = values.StyleLayerVisibility.fromRaw(raw.visibility),
                    .source_id = source_id,
                    .source_layer = source_layer,
                };
            }
        }.copyResult, c.mln_map_get_style_layer_info, .{ try native(self), try temp.stringView(layer_id) });
    }

    pub fn listStyleSourceIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!completion.Future(values.StringList) {
        return submitAllocatedQuery(values.StringList, self, allocator, copyStringListResult, c.mln_map_list_style_source_ids, .{try native(self)});
    }

    pub fn listStyleLayerIds(self: *MapHandle, allocator: std.mem.Allocator) status.Error!completion.Future(values.StringList) {
        return submitAllocatedQuery(values.StringList, self, allocator, copyStringListResult, c.mln_map_list_style_layer_ids, .{try native(self)});
    }

    pub fn addStyleSourceJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        source_json: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_style_source_json, .{
            try native(self),
            try temp.stringView(source_id),
            try temp.stringView(source_json),
        });
    }

    /// Accepts an ordered source-removal command. The completion reports a
    /// committed removal, a `NotFound` failure when no source has the ID, and
    /// an `InvalidState` failure when a layer still uses the source.
    pub fn removeStyleSource(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        return self.removeStyleObjectCommand(allocator, source_id, c.mln_map_remove_style_source);
    }

    /// Copies one source's metadata, attribution, URL, and inline TileJSON,
    /// completing with null when no source has the ID.
    pub fn getStyleSourceInfo(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(?values.StyleSourceInfo) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.StyleSourceInfo, self, allocator, copyStyleSourceInfoResult, c.mln_map_get_style_source_info, .{ try native(self), try temp.stringView(source_id) });
    }

    pub fn copyStyleSourceAttribution(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_copy_style_source_attribution, .{ try native(self), try temp.stringView(source_id) });
    }

    pub fn copyStyleSourceUrl(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_copy_style_source_url, .{ try native(self), try temp.stringView(source_id) });
    }

    /// Sets whether one style source stores fetched tiles in persistent
    /// storage. A missing source completes with `error.NotFound`.
    pub fn setStyleSourceVolatile(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        is_volatile: bool,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_style_source_volatile, .{ try native(self), try temp.stringView(source_id), is_volatile });
    }

    pub fn getStyleSourceTileUrls(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(values.StringList) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(values.StringList, self, allocator, copyStringListResult, c.mln_map_get_style_source_tile_urls, .{ try native(self), try temp.stringView(source_id) });
    }

    pub fn addStyleLayerJson(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_json: []const u8,
        before_layer_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_style_layer_json, .{
            try native(self),
            try temp.stringView(layer_json),
            stringView(before_layer_id),
        });
    }

    /// Accepts an ordered layer-removal command. The completion reports a
    /// committed removal, and a `NotFound` failure when no layer has the ID.
    pub fn removeStyleLayer(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        return self.removeStyleObjectCommand(allocator, layer_id, c.mln_map_remove_style_layer);
    }

    pub fn moveStyleLayer(self: *MapHandle, layer_id: []const u8, before_layer_id: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_move_style_layer, .{
            try native(self),
            stringView(layer_id),
            stringView(before_layer_id),
        });
    }

    pub fn getStyleLayerJson(self: *MapHandle, allocator: std.mem.Allocator, layer_id: []const u8) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_get_style_layer_json, .{ try native(self), try temp.stringView(layer_id) });
    }

    pub fn setStyleLightJson(self: *MapHandle, allocator: std.mem.Allocator, value: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_style_light_json, .{ try native(self), try temp.stringView(value) });
    }

    pub fn setStyleLightProperty(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        property_name: []const u8,
        value: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_style_light_property, .{
            try native(self),
            try temp.stringView(property_name),
            try temp.stringView(value),
        });
    }

    pub fn getStyleLightProperty(self: *MapHandle, allocator: std.mem.Allocator, property_name: []const u8) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_get_style_light_property, .{ try native(self), try temp.stringView(property_name) });
    }

    pub fn setStyleTransitionOptions(self: *MapHandle, options: values.StyleTransitionOptions) status.Error!completion.Future(completion.CommandCompletion) {
        const raw = values.styleTransitionOptionsToNative(options);
        return submitCommand(self, c.mln_map_set_style_transition_options, .{ try native(self), &raw });
    }

    pub fn getStyleTransitionOptions(self: *MapHandle) status.Error!completion.Future(values.StyleTransitionOptions) {
        return submitQuery(values.StyleTransitionOptions, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!values.StyleTransitionOptions {
                return values.styleTransitionOptionsFromNative(try completion.value(c.mln_style_transition_options)(result));
            }
        }.copyResult, c.mln_map_get_style_transition_options, .{try native(self)});
    }

    pub fn addVectorSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_vector_source_url, .{ try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null });
    }

    pub fn addVectorSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_vector_source_tiles, .{ try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null });
    }

    pub fn addRasterSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_raster_source_url, .{ try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null });
    }

    pub fn addRasterSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_raster_source_tiles, .{ try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null });
    }

    pub fn addRasterDemSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_raster_dem_source_url, .{ try native(self), try temp.stringView(source_id), try temp.stringView(url), if (options != null) &raw_options else null });
    }

    pub fn addRasterDemSourceTiles(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tiles: []const []const u8,
        options: ?values.StyleTileSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_tiles = try temp.stringViews(tiles);
        var raw_options = if (options) |value| try styleTileSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_raster_dem_source_tiles, .{ try native(self), try temp.stringView(source_id), raw_tiles.ptr, raw_tiles.len, if (options != null) &raw_options else null });
    }

    pub fn addHillshadeLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_hillshade_layer, .{ try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id) });
    }

    pub fn addColorReliefLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        source_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_color_relief_layer, .{ try native(self), try temp.stringView(layer_id), try temp.stringView(source_id), try temp.stringView(before_layer_id) });
    }

    pub fn setStyleImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
        image: values.PremultipliedRgba8Image,
        options: ?values.StyleImageOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
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
        return submitCommand(self, c.mln_map_set_style_image, .{
            try native(self),
            try temp.stringView(image_id),
            &raw_image,
            if (options != null) &raw_options else null,
        });
    }

    pub fn copyStyleImageStretches(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
    ) status.Error!completion.Future(?values.OwnedImageStretches) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedImageStretches, self, allocator, struct {
            fn copyResult(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!?values.OwnedImageStretches {
                const raw = (try optionalCompletionValue(c.mln_style_image_stretches_result, result)) orelse return null;
                const stretch_x = try target.alloc(values.ImageStretch, raw.stretch_x_count);
                errdefer target.free(stretch_x);
                const stretch_y = try target.alloc(values.ImageStretch, raw.stretch_y_count);
                errdefer target.free(stretch_y);
                if (raw.stretch_x_count != 0) {
                    const source = raw.stretch_x orelse return error.NativeError;
                    for (source[0..raw.stretch_x_count], stretch_x) |stretch, *copy| copy.* = .{ .from = stretch.from, .to = stretch.to };
                }
                if (raw.stretch_y_count != 0) {
                    const source = raw.stretch_y orelse return error.NativeError;
                    for (source[0..raw.stretch_y_count], stretch_y) |stretch, *copy| copy.* = .{ .from = stretch.from, .to = stretch.to };
                }
                return .{ .allocator = target.*, .stretch_x = stretch_x, .stretch_y = stretch_y };
            }
        }.copyResult, c.mln_map_copy_style_image_stretches, .{ try native(self), try temp.stringView(image_id) });
    }

    /// Accepts an ordered image-removal command. The completion reports a
    /// committed removal, and a `NotFound` failure when no runtime style image
    /// has the ID.
    pub fn removeStyleImage(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        return self.removeStyleObjectCommand(allocator, image_id, c.mln_map_remove_style_image);
    }

    pub fn getStyleImageInfo(self: *MapHandle, allocator: std.mem.Allocator, image_id: []const u8) status.Error!completion.Future(?values.StyleImageInfo) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitQuery(?values.StyleImageInfo, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!?values.StyleImageInfo {
                const raw = (try optionalCompletionValue(c.mln_style_image_result, result)) orelse return null;
                return values.styleImageInfoFromNative(raw.info);
            }
        }.copyResult, c.mln_map_get_style_image_info, .{ try native(self), try temp.stringView(image_id) });
    }

    pub fn copyStyleImagePremultipliedRgba8(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        image_id: []const u8,
    ) status.Error!completion.Future(?values.OwnedString) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitAllocatedQuery(?values.OwnedString, self, allocator, copyOptionalOwnedStringResult, c.mln_map_copy_style_image_premultiplied_rgba8, .{ try native(self), try temp.stringView(image_id) });
    }

    pub fn addImageSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        url: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        return submitCommand(self, c.mln_map_add_image_source_url, .{
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
            try temp.stringView(url),
        });
    }

    pub fn addImageSourceImage(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        coordinates: [4]values.LatLng,
        image: values.PremultipliedRgba8Image,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        return submitCommand(self, c.mln_map_add_image_source_image, .{
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
            &raw_image,
        });
    }

    pub fn setImageSourceUrl(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, url: []const u8) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_image_source_url, .{
            try native(self),
            try temp.stringView(source_id),
            try temp.stringView(url),
        });
    }

    pub fn setImageSourceImage(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, image: values.PremultipliedRgba8Image) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_image = values.premultipliedRgba8ImageToNative(image);
        return submitCommand(self, c.mln_map_set_image_source_image, .{
            try native(self),
            try temp.stringView(source_id),
            &raw_image,
        });
    }

    pub fn setImageSourceCoordinates(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8, coordinates: [4]values.LatLng) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(&coordinates);
        return submitCommand(self, c.mln_map_set_image_source_coordinates, .{
            try native(self),
            try temp.stringView(source_id),
            raw_coordinates.ptr,
            raw_coordinates.len,
        });
    }

    pub fn getImageSourceCoordinates(self: *MapHandle, allocator: std.mem.Allocator, source_id: []const u8) status.Error!completion.Future(?[4]values.LatLng) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitQuery(?[4]values.LatLng, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!?[4]values.LatLng {
                if (result.value_count == 0) return null;
                if (result.value_count != 4) return error.NativeError;
                const raw_coordinates = @as([*]align(1) const c.mln_lat_lng, @ptrCast(result.value orelse return error.NativeError))[0..4];
                var coordinates: [4]values.LatLng = undefined;
                for (raw_coordinates, &coordinates) |raw, *coordinate| coordinate.* = values.latLngFromNative(raw);
                return coordinates;
            }
        }.copyResult, c.mln_map_get_image_source_coordinates, .{ try native(self), try temp.stringView(source_id) });
    }

    pub fn addLocationIndicatorLayer(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        before_layer_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_location_indicator_layer, .{ try native(self), try temp.stringView(layer_id), try temp.stringView(before_layer_id) });
    }

    pub fn setLocationIndicatorLocation(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        coordinate: values.LatLng,
        altitude: f64,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_location_indicator_location, .{
            try native(self),
            try temp.stringView(layer_id),
            values.latLngToNative(coordinate),
            altitude,
        });
    }

    pub fn setLocationIndicatorBearing(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        bearing: f64,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_location_indicator_bearing, .{ try native(self), try temp.stringView(layer_id), bearing });
    }

    pub fn setLocationIndicatorAccuracyRadius(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        radius: f64,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_location_indicator_accuracy_radius, .{ try native(self), try temp.stringView(layer_id), radius });
    }

    pub fn setLocationIndicatorImageName(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        kind: values.LocationIndicatorImageKind,
        image_id: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_location_indicator_image_name, .{
            try native(self),
            try temp.stringView(layer_id),
            values.locationIndicatorImageKindToNative(kind),
            try temp.stringView(image_id),
        });
    }

    /// Adds a GeoJSON source with prepared inline data. The call borrows
    /// `data`, and the source adopts the options the data was prepared with,
    /// fixed for the lifetime of the source.
    pub fn addGeoJsonSourceData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        data: GeoJsonSourceDataHandle,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_add_geojson_source_data, .{ try native(self), try temp.stringView(source_id), @intFromEnum(data) });
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
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_geojson_source_data, .{ try native(self), try temp.stringView(source_id), @intFromEnum(data) });
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
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_geojson_source_synchronous_tiling, .{ try native(self), try temp.stringView(source_id), enabled });
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
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try styleGeoJsonSourceOptionsToNative(&temp, value) else undefined;
        return submitCommand(self, c.mln_map_add_geojson_source_url, .{
            try native(self),
            try temp.stringView(source_id),
            try temp.stringView(url),
            if (options != null) &raw_options else null,
        });
    }

    pub fn setGeoJsonSourceUrl(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        url: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_geojson_source_url, .{ try native(self), try temp.stringView(source_id), try temp.stringView(url) });
    }

    pub fn addCustomGeometrySource(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: CustomGeometrySourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        const source_state = try std.heap.smp_allocator.create(CustomGeometrySourceState);
        source_state.* = .{
            .fetch_tile = options.fetch_tile,
            .cancel_tile = options.cancel_tile,
            .release_context = options.release_context,
            .context = options.context,
        };
        errdefer std.heap.smp_allocator.destroy(source_state);

        try registerLiveCustomGeometrySourceState(source_state);
        // A failed add releases nothing, so this call owns the state it built.
        errdefer unregisterLiveCustomGeometrySourceState(source_state);

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var native_options = customGeometrySourceOptionsToNative(options, source_state);
        return submitCommand(self, c.mln_map_add_custom_geometry_source, .{ try native(self), try temp.stringView(source_id), &native_options });
    }

    pub fn setCustomGeometrySourceTileData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
        data: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_custom_geometry_source_tile_data, .{
            try native(self),
            try temp.stringView(source_id),
            canonicalTileIdToNative(tile_id),
            try temp.stringView(data),
        });
    }

    pub fn invalidateCustomGeometrySourceTile(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_invalidate_custom_geometry_source_tile, .{ try native(self), try temp.stringView(source_id), canonicalTileIdToNative(tile_id) });
    }

    pub fn invalidateCustomGeometrySourceRegion(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        bounds: values.LatLngBounds,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_invalidate_custom_geometry_source_region, .{ try native(self), try temp.stringView(source_id), values.latLngBoundsToNative(bounds) });
    }

    pub fn addCustomMvtVectorSource(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: CustomMvtVectorSourceOptions,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        const source_state = try std.heap.smp_allocator.create(CustomMvtVectorSourceState);
        source_state.* = .{
            .fetch_tile = options.fetch_tile,
            .cancel_tile = options.cancel_tile,
            .release_context = options.release_context,
            .context = options.context,
        };
        errdefer std.heap.smp_allocator.destroy(source_state);

        try registerLiveCustomMvtVectorSourceState(source_state);
        // A failed add releases nothing, so this call owns the state it built.
        errdefer unregisterLiveCustomMvtVectorSourceState(source_state);

        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var native_options = customMvtVectorSourceOptionsToNative(options, source_state);
        return submitCommand(self, c.mln_map_add_custom_mvt_vector_source, .{ try native(self), try temp.stringView(source_id), &native_options });
    }

    pub fn setCustomMvtVectorSourceTileData(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
        data: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_custom_mvt_vector_source_tile_data, .{
            try native(self),
            try temp.stringView(source_id),
            canonicalTileIdToNative(tile_id),
            try temp.stringView(data),
        });
    }

    pub fn setCustomMvtVectorSourceTileError(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
        message: []const u8,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_set_custom_mvt_vector_source_tile_error, .{
            try native(self),
            try temp.stringView(source_id),
            canonicalTileIdToNative(tile_id),
            try temp.stringView(message),
        });
    }

    pub fn invalidateCustomMvtVectorSourceTile(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        tile_id: CanonicalTileId,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, c.mln_map_invalidate_custom_mvt_vector_source_tile, .{ try native(self), try temp.stringView(source_id), canonicalTileIdToNative(tile_id) });
    }

    pub fn requestRepaint(self: *MapHandle) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_request_repaint, .{try native(self)});
    }

    pub fn setDebugOptions(self: *MapHandle, options: values.DebugOptions) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_set_debug_options, .{ try native(self), values.debugOptionsToNative(options) });
    }

    pub fn setRenderingStatsViewEnabled(self: *MapHandle, enabled: bool) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_set_rendering_stats_view_enabled, .{ try native(self), enabled });
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
            .gesture_in_progress = raw.gesture_in_progress,
            .rendering_stats_view_enabled = raw.rendering_stats_view_enabled,
            .repaint_demand = raw.repaint_demand,
            .event_mask = runtime_module.eventMaskFromRaw(raw.event_mask),
            .latest_render_update_generation = raw.latest_render_update_generation,
        };
    }

    /// Accepts a new logical extent. `scale_factor` is fixed at map creation:
    /// a different value is rejected with `error.InvalidArgument`, and only the
    /// width and height change. While a render session is attached, resize
    /// through `RenderSessionHandle.resize`, which submits this command itself.
    pub fn resize(self: *MapHandle, width: u32, height: u32, scale_factor: f64) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_resize, .{ try native(self), c.mln_logical_extent{ .width = width, .height = height, .scale_factor = scale_factor } });
    }

    pub fn dumpDebugLogs(self: *MapHandle) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_dump_debug_logs, .{try native(self)});
    }

    pub fn setViewportOptions(self: *MapHandle, options: values.ViewportOptions) status.Error!completion.Future(completion.CommandCompletion) {
        var raw_options = values.viewportOptionsToNative(options);
        return submitCommand(self, c.mln_map_set_viewport_options, .{ try native(self), &raw_options });
    }

    pub fn setTileOptions(self: *MapHandle, options: values.TileOptions) status.Error!completion.Future(completion.CommandCompletion) {
        var raw_options = values.tileOptionsToNative(options);
        return submitCommand(self, c.mln_map_set_tile_options, .{ try native(self), &raw_options });
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

    pub fn updateCamera(self: *MapHandle, update: CameraUpdate) status.Error!completion.Future(completion.CommandCompletion) {
        var raw = c.mln_camera_update_default();
        raw.mode = update.mode.toRaw();
        raw.camera = values.cameraOptionsToNative(update.camera);
        raw.animation = values.animationOptionsToNative(update.animation);
        raw.gesture_phase = update.gesture_phase.toRaw();
        return submitCommand(self, c.mln_map_update_camera, .{ try native(self), &raw });
    }

    /// Stops any running camera transition where it is. A map with no running
    /// transition commits the command anyway.
    pub fn cancelTransitions(self: *MapHandle) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_cancel_transitions, .{try native(self)});
    }

    pub fn applyCameraDelta(self: *MapHandle, delta: CameraDelta) status.Error!completion.Future(completion.CommandCompletion) {
        var raw = c.mln_camera_delta_default();
        raw.kind = delta.kind.toRaw();
        raw.offset = .{ .x = delta.offset.x, .y = delta.offset.y };
        raw.amount = delta.amount;
        if (delta.anchor) |anchor| {
            raw.has_anchor = true;
            raw.anchor = .{ .x = anchor.x, .y = anchor.y };
        }
        raw.animation = values.animationOptionsToNative(delta.animation);
        return submitCommand(self, c.mln_map_apply_camera_delta, .{ try native(self), &raw });
    }

    pub fn cameraQuery(self: *MapHandle) status.Error!completion.Future(CameraSnapshot) {
        return submitQuery(CameraSnapshot, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!CameraSnapshot {
                const raw = try completion.value(c.mln_camera_query_result)(result);
                return .{ .generation = raw.generation, .camera = values.cameraOptionsFromNative(raw.camera) };
            }
        }.copyResult, c.mln_map_camera_query, .{try native(self)});
    }

    pub fn requestStillImage(self: *MapHandle) status.Error!completion.Future(void) {
        return submitQuery(void, self, completion.unit, c.mln_map_request_still_image, .{try native(self)});
    }
    pub fn setProjectionMode(self: *MapHandle, mode: values.ProjectionMode) status.Error!completion.Future(completion.CommandCompletion) {
        var raw_mode = values.projectionModeToNative(mode);
        return submitCommand(self, c.mln_map_set_projection_mode, .{ try native(self), &raw_mode });
    }

    pub fn pixelForLatLng(self: *MapHandle, coordinate: values.LatLng) status.Error!completion.Future(values.ScreenPoint) {
        return submitQuery(values.ScreenPoint, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!values.ScreenPoint {
                return values.screenPointFromNative(try completion.value(c.mln_screen_point)(result));
            }
        }.copyResult, c.mln_map_pixel_for_lat_lng, .{ try native(self), values.latLngToNative(coordinate) });
    }

    pub fn latLngForPixel(self: *MapHandle, point: values.ScreenPoint) status.Error!completion.Future(values.LatLng) {
        return submitQuery(values.LatLng, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!values.LatLng {
                return values.latLngFromNative(try completion.value(c.mln_lat_lng)(result));
            }
        }.copyResult, c.mln_map_lat_lng_for_pixel, .{ try native(self), values.screenPointToNative(point) });
    }

    /// Converts a screen point to an unwrapped geographic coordinate.
    /// The longitude preserves the visible world copy.
    pub fn latLngForPixelUnwrapped(self: *MapHandle, point: values.ScreenPoint) status.Error!completion.Future(values.LatLng) {
        return submitQuery(values.LatLng, self, struct {
            fn copyResult(result: *const c.mln_completion_result) status.Error!values.LatLng {
                return values.latLngFromNative(try completion.value(c.mln_lat_lng)(result));
            }
        }.copyResult, c.mln_map_lat_lng_for_pixel_unwrapped, .{ try native(self), values.screenPointToNative(point) });
    }

    pub fn pixelsForLatLngs(self: *MapHandle, allocator: std.mem.Allocator, coordinates: []const values.LatLng) status.Error!completion.Future(ScreenPointList) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        return submitAllocatedQuery(ScreenPointList, self, allocator, struct {
            fn copyResult(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!ScreenPointList {
                const items = try target.alloc(values.ScreenPoint, result.value_count);
                errdefer target.free(items);
                if (result.value_count != 0) {
                    const raw = @as([*]align(1) const c.mln_screen_point, @ptrCast(result.value orelse return error.NativeError))[0..result.value_count];
                    for (raw, items) |point, *item| item.* = values.screenPointFromNative(point);
                }
                return .{ .allocator = target.*, .items = items };
            }
        }.copyResult, c.mln_map_pixels_for_lat_lngs, .{ try native(self), if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len });
    }

    pub fn latLngsForPixels(self: *MapHandle, allocator: std.mem.Allocator, points: []const values.ScreenPoint) status.Error!completion.Future(LatLngList) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        return submitAllocatedQuery(LatLngList, self, allocator, struct {
            fn copyResult(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!LatLngList {
                const items = try target.alloc(values.LatLng, result.value_count);
                errdefer target.free(items);
                if (result.value_count != 0) {
                    const raw = @as([*]align(1) const c.mln_lat_lng, @ptrCast(result.value orelse return error.NativeError))[0..result.value_count];
                    for (raw, items) |coordinate, *item| item.* = values.latLngFromNative(coordinate);
                }
                return .{ .allocator = target.*, .items = items };
            }
        }.copyResult, c.mln_map_lat_lngs_for_pixels, .{ try native(self), if (raw_points.len == 0) null else raw_points.ptr, raw_points.len });
    }

    /// Converts screen points to unwrapped geographic coordinates.
    /// Each longitude preserves its visible world copy.
    pub fn latLngsForPixelsUnwrapped(self: *MapHandle, allocator: std.mem.Allocator, points: []const values.ScreenPoint) status.Error!completion.Future(LatLngList) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_points = try temp.screenPoints(points);
        return submitAllocatedQuery(LatLngList, self, allocator, struct {
            fn copyResult(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!LatLngList {
                const items = try target.alloc(values.LatLng, result.value_count);
                errdefer target.free(items);
                if (result.value_count != 0) {
                    const raw = @as([*]align(1) const c.mln_lat_lng, @ptrCast(result.value orelse return error.NativeError))[0..result.value_count];
                    for (raw, items) |coordinate, *item| item.* = values.latLngFromNative(coordinate);
                }
                return .{ .allocator = target.*, .items = items };
            }
        }.copyResult, c.mln_map_lat_lngs_for_pixels_unwrapped, .{ try native(self), if (raw_points.len == 0) null else raw_points.ptr, raw_points.len });
    }

    pub fn cameraForLatLngBounds(self: *MapHandle, bounds: values.LatLngBounds, fit_options: ?values.CameraFitOptions) status.Error!completion.Future(values.CameraOptions) {
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        return submitQuery(values.CameraOptions, self, copyCameraOptionsResult, c.mln_map_camera_for_lat_lng_bounds, .{ try native(self), values.latLngBoundsToNative(bounds), fit_ptr });
    }

    pub fn cameraForLatLngs(self: *MapHandle, allocator: std.mem.Allocator, coordinates: []const values.LatLng, fit_options: ?values.CameraFitOptions) status.Error!completion.Future(values.CameraOptions) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_coordinates = try temp.latLngs(coordinates);
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        return submitQuery(values.CameraOptions, self, copyCameraOptionsResult, c.mln_map_camera_for_lat_lngs, .{ try native(self), if (raw_coordinates.len == 0) null else raw_coordinates.ptr, raw_coordinates.len, fit_ptr });
    }

    pub fn cameraForGeometry(self: *MapHandle, allocator: std.mem.Allocator, geometry: []const u8, fit_options: ?values.CameraFitOptions) status.Error!completion.Future(values.CameraOptions) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_fit = if (fit_options) |options| values.cameraFitOptionsToNative(options) else undefined;
        const fit_ptr = if (fit_options != null) &raw_fit else null;
        return submitQuery(values.CameraOptions, self, copyCameraOptionsResult, c.mln_map_camera_for_geometry, .{ try native(self), try temp.stringView(geometry), fit_ptr });
    }

    /// Computes geographic bounds for a camera from two viewport corners.
    ///
    /// The box is the hull of the top-left and bottom-right screen corners for
    /// that camera in the current viewport. When bearing and pitch are zero, the
    /// box equals the visible area. Those corners are the northwest and
    /// southeast of the viewport. Longitudes stay in -180 to 180.
    pub fn latLngBoundsForCamera(self: *MapHandle, camera: values.CameraOptions) status.Error!completion.Future(values.LatLngBounds) {
        var raw_camera = values.cameraOptionsToNative(camera);
        return submitQuery(values.LatLngBounds, self, copyLatLngBoundsResult, c.mln_map_lat_lng_bounds_for_camera, .{ try native(self), &raw_camera });
    }

    /// Computes geographic bounds for a camera from the four viewport corners.
    ///
    /// The axis-aligned hull of all four screen corners and the center
    /// encompasses the projected viewport. Longitudes unwrap onto the shortest
    /// path through the center. A viewport that crosses the antimeridian reports
    /// values outside -180 to 180.
    pub fn latLngBoundsForCameraUnwrapped(self: *MapHandle, camera: values.CameraOptions) status.Error!completion.Future(values.LatLngBounds) {
        var raw_camera = values.cameraOptionsToNative(camera);
        return submitQuery(values.LatLngBounds, self, copyLatLngBoundsResult, c.mln_map_lat_lng_bounds_for_camera_unwrapped, .{ try native(self), &raw_camera });
    }

    pub fn setBounds(self: *MapHandle, options: values.BoundOptions) status.Error!completion.Future(completion.CommandCompletion) {
        var raw_options = values.boundOptionsToNative(options);
        return submitCommand(self, c.mln_map_set_bounds, .{ try native(self), &raw_options });
    }

    pub fn setFreeCameraOptions(self: *MapHandle, options: values.FreeCameraOptions) status.Error!completion.Future(completion.CommandCompletion) {
        var raw_options = values.freeCameraOptionsToNative(options);
        return submitCommand(self, c.mln_map_set_free_camera_options, .{ try native(self), &raw_options });
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
    pub fn setEventMask(self: *MapHandle, mask: runtime_module.RuntimeEventMask) status.Error!completion.Future(completion.CommandCompletion) {
        return submitCommand(self, c.mln_map_set_event_mask, .{ try native(self), runtime_module.eventMaskToRaw(mask) });
    }

    fn removeStyleObjectCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        object_id: []const u8,
        comptime command: *const fn (c.mln_map, c.mln_buffer_view, [*c]const c.mln_completion) callconv(.c) c.mln_status,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, command, .{ try native(self), try temp.stringView(object_id) });
    }

    fn setLayerStringCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        value: []const u8,
        comptime command: *const fn (c.mln_map, c.mln_buffer_view, c.mln_buffer_view, [*c]const c.mln_completion) callconv(.c) c.mln_status,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, command, .{ try native(self), try temp.stringView(layer_id), try temp.stringView(value) });
    }

    fn setLayerNumberCommand(
        self: *MapHandle,
        allocator: std.mem.Allocator,
        layer_id: []const u8,
        value: f64,
        comptime command: *const fn (c.mln_map, c.mln_buffer_view, f64, [*c]const c.mln_completion) callconv(.c) c.mln_status,
    ) status.Error!completion.Future(completion.CommandCompletion) {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        return submitCommand(self, command, .{ try native(self), try temp.stringView(layer_id), value });
    }

    /// Releases the map handle and returns the future for its native teardown.
    ///
    /// The future completes after the map's accepted commands reach a terminal
    /// disposition and its native state is gone. A host that outlives its maps
    /// deinitializes the future without waiting. Closing a map that still has
    /// an attached render session reports `error.InvalidState`.
    pub fn close(self: *MapHandle) status.Error!completion.Future(void) {
        const map_close = beginMapClose(self.*) catch |err| {
            if (err == error.InvalidState) {
                if (diagnosticStore(self)) |store| {
                    try status.setBindingDiagnostic(store, "map has an attached render session");
                }
            }
            return err;
        } orelse return completion.completed(void, {});
        const teardown = completion.submit(void, map_close.diagnostic_store, completion.unit, map_close.native, struct {
            fn start(raw_map: c.mln_map, descriptor: *const c.mln_completion) c.mln_status {
                return c.mln_map_release(raw_map, descriptor);
            }
        }.start) catch |err| {
            cancelMapClose(map_close.state);
            return err;
        };
        runtime_module.unregisterMap(map_close.runtime_registry, map_close.native);
        const map_state = finishMapClose(self.*) orelse map_close.state;
        std.heap.smp_allocator.destroy(map_state);
        return teardown;
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

fn customMvtVectorSourceOptionsToNative(
    options: CustomMvtVectorSourceOptions,
    source_state: *CustomMvtVectorSourceState,
) c.mln_custom_mvt_vector_source_options {
    var raw = c.mln_custom_mvt_vector_source_options_default();
    raw.fetch_tile = customMvtVectorFetchTileTrampoline;
    raw.cancel_tile = if (options.cancel_tile != null) customMvtVectorCancelTileTrampoline else null;
    raw.release_user_data = customMvtVectorReleaseTrampoline;
    raw.user_data = source_state;
    if (options.min_zoom) |min_zoom| {
        raw.fields |= c.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
        raw.min_zoom = min_zoom;
    }
    if (options.max_zoom) |max_zoom| {
        raw.fields |= c.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
        raw.max_zoom = max_zoom;
    }
    return raw;
}

fn customGeometryFetchTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer source_state.upcalls.end();

    source_state.fetch_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn customGeometryCancelTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomGeometrySourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomGeometryUpcall(source_state)) return;
    defer source_state.upcalls.end();

    const cancel_tile = source_state.cancel_tile orelse return;
    cancel_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn beginCustomGeometryUpcall(source_state: *CustomGeometrySourceState) bool {
    std.Io.Threaded.mutexLock(&custom_geometry_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_geometry_state_registry_lock);

    for (custom_geometry_state_registry.items) |live_state| {
        if (live_state == source_state) {
            source_state.upcalls.begin();
            return true;
        }
    }
    return false;
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
    source_state.upcalls.waitUntilIdle();
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

fn customMvtVectorFetchTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomMvtVectorSourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomMvtVectorUpcall(source_state)) return;
    defer source_state.upcalls.end();

    source_state.fetch_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn customMvtVectorCancelTileTrampoline(user_data: ?*anyopaque, raw_tile_id: c.mln_canonical_tile_id) callconv(.c) void {
    const source_state: *CustomMvtVectorSourceState = @ptrCast(@alignCast(user_data orelse return));
    if (!beginCustomMvtVectorUpcall(source_state)) return;
    defer source_state.upcalls.end();

    const cancel_tile = source_state.cancel_tile orelse return;
    cancel_tile(source_state.context, canonicalTileIdFromNative(raw_tile_id));
}

fn beginCustomMvtVectorUpcall(source_state: *CustomMvtVectorSourceState) bool {
    std.Io.Threaded.mutexLock(&custom_mvt_vector_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_mvt_vector_state_registry_lock);

    for (custom_mvt_vector_state_registry.items) |live_state| {
        if (live_state == source_state) {
            source_state.upcalls.begin();
            return true;
        }
    }
    return false;
}

fn customMvtVectorReleaseTrampoline(user_data: ?*anyopaque) callconv(.c) void {
    const source_state: *CustomMvtVectorSourceState = @ptrCast(@alignCast(user_data orelse return));
    freeCustomMvtVectorSourceState(source_state);
}

fn freeCustomMvtVectorSourceState(source_state: *CustomMvtVectorSourceState) void {
    retireLiveCustomMvtVectorSourceState(source_state);
    source_state.upcalls.waitUntilIdle();
    if (source_state.release_context) |release| release(source_state.context);
    std.heap.smp_allocator.destroy(source_state);
}

fn registerLiveCustomMvtVectorSourceState(source_state: *CustomMvtVectorSourceState) std.mem.Allocator.Error!void {
    std.Io.Threaded.mutexLock(&custom_mvt_vector_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_mvt_vector_state_registry_lock);
    try custom_mvt_vector_state_registry.append(std.heap.smp_allocator, source_state);
}

fn unregisterLiveCustomMvtVectorSourceState(source_state: *CustomMvtVectorSourceState) void {
    std.Io.Threaded.mutexLock(&custom_mvt_vector_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_mvt_vector_state_registry_lock);
    removeLiveCustomMvtVectorSourceStateLocked(source_state);
}

fn retireLiveCustomMvtVectorSourceState(source_state: *CustomMvtVectorSourceState) void {
    std.Io.Threaded.mutexLock(&custom_mvt_vector_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_mvt_vector_state_registry_lock);
    removeLiveCustomMvtVectorSourceStateLocked(source_state);
}

fn removeLiveCustomMvtVectorSourceStateLocked(source_state: *CustomMvtVectorSourceState) void {
    for (custom_mvt_vector_state_registry.items, 0..) |live_state, index| {
        if (live_state == source_state) {
            _ = custom_mvt_vector_state_registry.orderedRemove(index);
            return;
        }
    }
}

fn discardCompletion(_: ?*anyopaque, _: [*c]const c.mln_completion_result) callconv(.c) void {}

// Releases a native map the binding never registered. Nothing is left to report
// the teardown to, so the completion discards it.
fn releaseUntrackedMap(map: c.mln_map) void {
    const descriptor = c.mln_completion{
        .size = @sizeOf(c.mln_completion),
        .callback = discardCompletion,
        .user_data = null,
        .release_user_data = null,
    };
    _ = c.mln_map_release(map, &descriptor);
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

fn submitCommand(handle: *MapHandle, comptime command: anytype, arguments: anytype) status.Error!completion.Future(completion.CommandCompletion) {
    const Arguments = @TypeOf(arguments);
    return completion.submit(completion.CommandCompletion, diagnosticStore(handle), completion.command, arguments, struct {
        fn start(args: Arguments, descriptor: *const c.mln_completion) c.mln_status {
            return @call(.auto, command, args ++ .{descriptor});
        }
    }.start);
}

fn submitQuery(
    comptime T: type,
    handle: *MapHandle,
    comptime copy: *const fn (*const c.mln_completion_result) status.Error!T,
    comptime query: anytype,
    arguments: anytype,
) status.Error!completion.Future(T) {
    const Arguments = @TypeOf(arguments);
    return completion.submit(T, diagnosticStore(handle), copy, arguments, struct {
        fn start(args: Arguments, descriptor: *const c.mln_completion) c.mln_status {
            return @call(.auto, query, args ++ .{descriptor});
        }
    }.start);
}

fn submitAllocatedQuery(
    comptime T: type,
    handle: *MapHandle,
    allocator: std.mem.Allocator,
    comptime copy: *const fn (*const c.mln_completion_result, *std.mem.Allocator) status.Error!T,
    comptime query: anytype,
    arguments: anytype,
) status.Error!completion.Future(T) {
    const Arguments = @TypeOf(arguments);
    return completion.submitWithCopyContext(T, std.mem.Allocator, diagnosticStore(handle), copy, allocator, arguments, struct {
        fn start(args: Arguments, descriptor: *const c.mln_completion) c.mln_status {
            return @call(.auto, query, args ++ .{descriptor});
        }
    }.start);
}

fn optionalCompletionValue(comptime T: type, result: *const c.mln_completion_result) status.Error!?T {
    if (result.value_count == 0) return null;
    return try completion.value(T)(result);
}

fn copyView(allocator: std.mem.Allocator, view: c.mln_buffer_view) status.Error![]const u8 {
    if (view.size == 0) return allocator.dupe(u8, "");
    const data = view.data orelse return error.NativeError;
    return allocator.dupe(u8, @as([*]const u8, @ptrCast(data))[0..view.size]);
}

/// Copies a borrowed view the native result marks present, reporting null for
/// an absent value, which the C API spells as an empty view.
fn copyPresentView(allocator: std.mem.Allocator, view: c.mln_buffer_view) status.Error!?[]const u8 {
    if (view.size == 0) return null;
    return try copyView(allocator, view);
}

fn copyStyleSourceInfoResult(
    result: *const c.mln_completion_result,
    allocator: *std.mem.Allocator,
) status.Error!?values.StyleSourceInfo {
    const raw_result = (try optionalCompletionValue(c.mln_style_source_result, result)) orelse return null;
    const raw = raw_result.info;
    const attribution = if (raw.has_attribution) try copyView(allocator.*, raw_result.attribution) else null;
    errdefer if (attribution) |owned| allocator.free(owned);
    const url = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_URL) != 0) try copyView(allocator.*, raw_result.url) else null;
    errdefer if (url) |owned| allocator.free(owned);

    var tile_json: ?values.StyleTileJsonInfo = null;
    errdefer if (tile_json) |json| {
        for (json.tile_urls) |tile_url| allocator.free(tile_url);
        allocator.free(json.tile_urls);
    };
    if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_TILEJSON) != 0) {
        const tile_urls = try allocator.alloc([]const u8, raw_result.tile_url_count);
        var initialized: usize = 0;
        errdefer {
            for (tile_urls[0..initialized]) |tile_url| allocator.free(tile_url);
            allocator.free(tile_urls);
        }
        if (raw_result.tile_url_count != 0) {
            if (raw_result.tile_urls == null) return error.NativeError;
            const views = raw_result.tile_urls[0..raw_result.tile_url_count];
            for (views, tile_urls) |view, *tile_url| {
                tile_url.* = try copyView(allocator.*, view);
                initialized += 1;
            }
        }
        tile_json = .{
            .tile_urls = tile_urls,
            .min_zoom = raw.min_zoom,
            .max_zoom = raw.max_zoom,
            .scheme = values.StyleTileScheme.fromRaw(raw.scheme),
            .bounds = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) values.latLngBoundsFromNative(raw.bounds) else null,
        };
    }

    return .{
        .allocator = allocator.*,
        .source_type = values.styleSourceTypeFromNative(raw.type),
        .id_size = raw.id_size,
        .is_volatile = raw.is_volatile,
        .attribution = attribution,
        .url = url,
        .tile_json = tile_json,
        .tile_size = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0) raw.tile_size else null,
        .vector_encoding = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0) values.StyleVectorTileEncoding.fromRaw(raw.vector_encoding) else null,
        .raster_encoding = if ((raw.fields & c.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0) values.StyleRasterDemEncoding.fromRaw(raw.raster_encoding) else null,
    };
}

fn copyOwnedStringResult(result: *const c.mln_completion_result, allocator: *std.mem.Allocator) status.Error!values.OwnedString {
    const view = try completion.value(c.mln_buffer_view)(result);
    return .{ .allocator = allocator.*, .value = try copyView(allocator.*, view) };
}

fn copyOptionalOwnedStringResult(result: *const c.mln_completion_result, allocator: *std.mem.Allocator) status.Error!?values.OwnedString {
    const view = (try optionalCompletionValue(c.mln_buffer_view, result)) orelse return null;
    return .{ .allocator = allocator.*, .value = try copyView(allocator.*, view) };
}

fn copyStringListResult(result: *const c.mln_completion_result, allocator: *std.mem.Allocator) status.Error!values.StringList {
    const items = try allocator.alloc([]const u8, result.value_count);
    errdefer allocator.free(items);
    var initialized: usize = 0;
    errdefer for (items[0..initialized]) |item| allocator.free(item);
    if (result.value_count != 0) {
        const pointer = result.value orelse return error.NativeError;
        const views = @as([*]align(1) const c.mln_buffer_view, @ptrCast(pointer))[0..result.value_count];
        for (views, items) |view, *item| {
            item.* = try copyView(allocator.*, view);
            initialized += 1;
        }
    }
    return .{ .allocator = allocator.*, .items = items };
}

fn copyCameraOptionsResult(result: *const c.mln_completion_result) status.Error!values.CameraOptions {
    return values.cameraOptionsFromNative(try completion.value(c.mln_camera_options)(result));
}

fn copyLatLngBoundsResult(result: *const c.mln_completion_result) status.Error!values.LatLngBounds {
    return values.latLngBoundsFromNative(try completion.value(c.mln_lat_lng_bounds)(result));
}

fn featureStateSelectorToNative(
    temp: *native_temp.TempStorage,
    selector: FeatureStateSelector,
) status.Error!c.mln_feature_state_selector {
    var raw = c.mln_feature_state_selector{
        .size = @sizeOf(c.mln_feature_state_selector),
        .fields = 0,
        .source_id = try temp.stringView(selector.source_id),
        .source_layer_id = .{ .data = null, .size = 0 },
        .feature_id = .{ .data = null, .size = 0 },
        .state_key = .{ .data = null, .size = 0 },
    };
    if (selector.source_layer_id) |source_layer_id| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
        raw.source_layer_id = try temp.stringView(source_layer_id);
    }
    if (selector.feature_id) |feature_id| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
        raw.feature_id = try temp.stringView(feature_id);
    }
    if (selector.state_key) |state_key| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
        raw.state_key = try temp.stringView(state_key);
    }
    return raw;
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

fn liveCustomMvtVectorSourceCountForTesting() usize {
    std.Io.Threaded.mutexLock(&custom_mvt_vector_state_registry_lock);
    defer std.Io.Threaded.mutexUnlock(&custom_mvt_vector_state_registry_lock);
    return custom_mvt_vector_state_registry.items.len;
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
        .upcalls = .{},
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
    try std.testing.expectEqual(@as(usize, 0), source_state.upcalls.activeCount());
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
    var map_future = try MapHandle.create(runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    errdefer closeMapForTesting(&map) catch {};
    try expectCommittedForTesting(try map.setStyleJson(test_style_json));
    try std.testing.expect(try waitForRuntimeEventForTesting(runtime, .map_style_loaded));
    return map;
}

fn closeMapForTesting(map: *MapHandle) !void {
    var teardown = try map.close();
    defer teardown.deinit();
    try teardown.wait(null);
}

fn waitForCommandDispositionForTesting(future_value: completion.Future(completion.CommandCompletion)) !completion.CommandDisposition {
    var future = future_value;
    defer future.deinit();
    return (try future.wait(null)).disposition;
}

fn expectCommittedForTesting(future_value: completion.Future(completion.CommandCompletion)) !void {
    try std.testing.expect(std.meta.eql(
        try waitForCommandDispositionForTesting(future_value),
        completion.CommandDisposition.committed,
    ));
}

fn waitForRuntimeEventForTesting(runtime: *RuntimeHandle, event_type: runtime_module.RuntimeEventType) !bool {
    var attempts: usize = 0;
    while (attempts < 200) : (attempts += 1) {
        var batch = try runtime.drainEvents(std.testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
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
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer closeMapForTesting(&map) catch @panic("map close failed");

    const baseline = liveCustomGeometrySourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    try expectCommittedForTesting(try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    }));
    try std.testing.expectEqual(baseline + 1, liveCustomGeometrySourceCountForTesting());

    try expectCommittedForTesting(try map.removeStyleSource(std.testing.allocator, "custom"));
    try std.testing.expectEqual(baseline, liveCustomGeometrySourceCountForTesting());
}

test "an explicit custom MVT vector source removal releases the callback state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer closeMapForTesting(&map) catch @panic("map close failed");

    const baseline = liveCustomMvtVectorSourceCountForTesting();
    var state = TestCustomGeometryCallbackState{};
    try expectCommittedForTesting(try map.addCustomMvtVectorSource(std.testing.allocator, "custom-mvt", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .context = &state,
    }));
    try std.testing.expectEqual(baseline + 1, liveCustomMvtVectorSourceCountForTesting());

    try expectCommittedForTesting(try map.removeStyleSource(std.testing.allocator, "custom-mvt"));
    try std.testing.expectEqual(baseline, liveCustomMvtVectorSourceCountForTesting());
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
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");
    var provider_future = try runtime.setResourceProvider(.{ .handler = testStyleJsonProvider });
    defer provider_future.deinit();
    _ = try provider_future.wait(null);

    var map = try createLoadedMapForTesting(&runtime);
    defer closeMapForTesting(&map) catch @panic("map close failed");
    var mask_future = try map.setEventMask(narrowed);
    defer mask_future.deinit();
    _ = try mask_future.wait(null);

    var state = TestCustomGeometryCallbackState{};
    try expectCommittedForTesting(try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .release_context = testReleaseCustomGeometryContext,
        .context = &state,
    }));
    var add_barrier = try runtime.barrier();
    defer add_barrier.deinit();
    _ = try add_barrier.wait(null);
    try std.testing.expectEqual(@as(usize, 0), state.release_count);
    // The mask stays what the host set, because the binding adds nothing to it.
    try std.testing.expectEqual(narrowed, (try map.snapshot()).event_mask);

    try expectCommittedForTesting(try map.setStyleUrl(std.testing.allocator, "custom://style.json"));
    var released = false;
    for (0..200) |_| {
        var batch = try runtime.drainEvents(std.testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            try std.testing.expect(!std.meta.eql(
                event.event_type,
                runtime_module.RuntimeEventType.map_style_loaded,
            ));
        }
        if (state.release_count == 1) {
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
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    var map_open = true;
    defer if (map_open) closeMapForTesting(&map) catch @panic("map close failed");

    var state = TestCustomGeometryCallbackState{};
    try expectCommittedForTesting(try map.addCustomGeometrySource(std.testing.allocator, "custom", .{
        .fetch_tile = testFetchCustomGeometryTile,
        .release_context = testReleaseCustomGeometryContext,
        .context = &state,
    }));

    try closeMapForTesting(&map);
    map_open = false;
    var close_barrier = try runtime.barrier();
    defer close_barrier.deinit();
    _ = try close_barrier.wait(null);
    try std.testing.expectEqual(@as(usize, 1), state.release_count);
}

// A rejected add releases nothing, so the state belongs to the failing call.
test "a rejected custom geometry source add releases its own callback state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");
    var map = try createLoadedMapForTesting(&runtime);
    defer closeMapForTesting(&map) catch @panic("map close failed");

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
    defer runtime_module.closeRuntimeForTesting(&runtime) catch @panic("runtime close failed");

    var first_future = try MapHandle.create(&runtime, .{});

    defer first_future.deinit();

    var first = try first_future.wait(null);
    const released = @intFromEnum(first);
    try closeMapForTesting(&first);

    var stale_snapshot = std.mem.zeroes(c.mln_map_snapshot);
    stale_snapshot.size = @sizeOf(c.mln_map_snapshot);
    try std.testing.expectError(
        error.InvalidArgument,
        status.checkStatus(c.mln_map_snapshot_get(released, &stale_snapshot), null),
    );

    var second_future = try MapHandle.create(&runtime, .{});

    defer second_future.deinit();

    var second = try second_future.wait(null);
    defer closeMapForTesting(&second) catch @panic("map close failed");

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
