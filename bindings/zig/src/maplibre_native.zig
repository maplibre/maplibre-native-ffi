const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map = @import("map.zig");
const projection = @import("projection.zig");
const runtime = @import("runtime.zig");
const status = @import("status.zig");
const values = @import("values.zig");

pub const Diagnostic = diagnostics.Diagnostic;
pub const DiagnosticStore = diagnostics.DiagnosticStore;
pub const NativeStatusError = status.NativeStatusError;
pub const BindingError = status.BindingError;
pub const Error = status.Error;

pub const RuntimeHandle = runtime.RuntimeHandle;
pub const RuntimeOptions = runtime.RuntimeOptions;
pub const RuntimeEvent = runtime.RuntimeEvent;
pub const OwnedRuntimeEvent = runtime.OwnedRuntimeEvent;
pub const RuntimeEventPayload = runtime.RuntimeEventPayload;
pub const RuntimeEventType = runtime.RuntimeEventType;
pub const RuntimeEventSourceType = runtime.RuntimeEventSourceType;
pub const RuntimeEventPayloadType = runtime.RuntimeEventPayloadType;
pub const RenderMode = runtime.RenderMode;
pub const RenderingStats = runtime.RenderingStats;
pub const RenderFramePayload = runtime.RenderFramePayload;
pub const RenderMapPayload = runtime.RenderMapPayload;
pub const StyleImageMissingPayload = runtime.StyleImageMissingPayload;
pub const TileOperation = runtime.TileOperation;
pub const TileId = runtime.TileId;
pub const TileActionPayload = runtime.TileActionPayload;
pub const OfflineRegionDownloadState = runtime.OfflineRegionDownloadState;
pub const OfflineRegionStatus = runtime.OfflineRegionStatus;
pub const OfflineRegionStatusPayload = runtime.OfflineRegionStatusPayload;
pub const ResourceErrorReason = runtime.ResourceErrorReason;
pub const OfflineRegionResponseErrorPayload = runtime.OfflineRegionResponseErrorPayload;
pub const OfflineRegionTileCountLimitPayload = runtime.OfflineRegionTileCountLimitPayload;
pub const UnknownPayload = runtime.UnknownPayload;

pub const MapHandle = map.MapHandle;
pub const MapOptions = map.MapOptions;
pub const MapMode = map.MapMode;
pub const MapProjectionHandle = projection.MapProjectionHandle;

pub const LatLng = values.LatLng;
pub const ScreenPoint = values.ScreenPoint;
pub const EdgeInsets = values.EdgeInsets;
pub const LatLngBounds = values.LatLngBounds;
pub const ProjectedMeters = values.ProjectedMeters;
pub const MapId = values.MapId;
pub const UnitBezier = values.UnitBezier;
pub const CameraOptions = values.CameraOptions;
pub const AnimationOptions = values.AnimationOptions;
pub const CameraFitOptions = values.CameraFitOptions;
pub const ProjectionMode = values.ProjectionMode;
pub const DebugOptions = values.DebugOptions;
pub const NorthOrientation = values.NorthOrientation;
pub const ConstrainMode = values.ConstrainMode;
pub const ViewportMode = values.ViewportMode;
pub const TileLodMode = values.TileLodMode;
pub const ViewportOptions = values.ViewportOptions;
pub const TileOptions = values.TileOptions;
pub const JsonValue = values.JsonValue;
pub const JsonMember = values.JsonMember;
pub const OwnedJsonValue = values.OwnedJsonValue;
pub const OwnedJsonMember = values.OwnedJsonMember;
pub const StringList = values.StringList;
pub const Geometry = values.Geometry;
pub const FeatureIdentifier = values.FeatureIdentifier;
pub const Feature = values.Feature;
pub const GeoJson = values.GeoJson;
pub const StyleSourceType = values.StyleSourceType;
pub const StyleSourceInfo = values.StyleSourceInfo;
pub const OwnedString = values.OwnedString;

pub const projectedMetersForLatLng = projection.projectedMetersForLatLng;
pub const latLngForProjectedMeters = projection.latLngForProjectedMeters;

/// Returns the C ABI contract version reported by the linked native library.
pub fn cAbiVersion() u32 {
    return c.mln_c_version();
}

/// Validates that the linked native library exposes the C ABI version supported
/// by this Zig package.
pub fn validateAbiVersion(diagnostic_store: ?*DiagnosticStore) Error!void {
    return status.validateAbiVersion(diagnostic_store);
}

comptime {
    _ = c.MLN_STATUS_OK;
}
