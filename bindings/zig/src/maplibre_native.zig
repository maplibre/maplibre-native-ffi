const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const logging = @import("logging.zig");
const map = @import("map.zig");
const projection = @import("projection.zig");
const render = @import("render.zig");
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
pub const NetworkStatus = runtime.NetworkStatus;
pub const AmbientCacheOperation = runtime.AmbientCacheOperation;
pub const OfflineRegionId = runtime.OfflineRegionId;
pub const OfflineOperationId = runtime.OfflineOperationId;
pub const OfflineOperationKind = runtime.OfflineOperationKind;
pub const OfflineOperationResultKind = runtime.OfflineOperationResultKind;
pub const OfflineOperationHandle = runtime.OfflineOperationHandle;
pub const OfflineTilePyramidRegionDefinition = runtime.OfflineTilePyramidRegionDefinition;
pub const OfflineGeometryRegionDefinition = runtime.OfflineGeometryRegionDefinition;
pub const OfflineRegionDefinition = runtime.OfflineRegionDefinition;
pub const OwnedOfflineTilePyramidRegionDefinition = runtime.OwnedOfflineTilePyramidRegionDefinition;
pub const OwnedOfflineGeometryRegionDefinition = runtime.OwnedOfflineGeometryRegionDefinition;
pub const OwnedOfflineRegionDefinition = runtime.OwnedOfflineRegionDefinition;
pub const OwnedOfflineRegion = runtime.OwnedOfflineRegion;
pub const OfflineRegionList = runtime.OfflineRegionList;
pub const ResourceKind = runtime.ResourceKind;
pub const ResourceTransformRequest = runtime.ResourceTransformRequest;
pub const ResourceTransformResponse = runtime.ResourceTransformResponse;
pub const ResourceTransformHandler = runtime.ResourceTransformHandler;
pub const ResourceTransform = runtime.ResourceTransform;
pub const ResourceLoadingMethod = runtime.ResourceLoadingMethod;
pub const ResourcePriority = runtime.ResourcePriority;
pub const ResourceUsage = runtime.ResourceUsage;
pub const ResourceStoragePolicy = runtime.ResourceStoragePolicy;
pub const ResourceResponseStatus = runtime.ResourceResponseStatus;
pub const ResourceProviderDecision = runtime.ResourceProviderDecision;
pub const ResourceByteRange = runtime.ResourceByteRange;
pub const ResourceRequest = runtime.ResourceRequest;
pub const ResourceResponse = runtime.ResourceResponse;
pub const ResourceProviderHandler = runtime.ResourceProviderHandler;
pub const ResourceProvider = runtime.ResourceProvider;
pub const ResourceRequestHandle = runtime.ResourceRequestHandle;
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
pub const OfflineOperationCompletedPayload = runtime.OfflineOperationCompletedPayload;
pub const UnknownPayload = runtime.UnknownPayload;

pub const LogSeverity = logging.LogSeverity;
pub const LogEvent = logging.LogEvent;
pub const LogSeverityMask = logging.LogSeverityMask;
pub const LogRecord = logging.LogRecord;
pub const LogHandler = logging.LogHandler;
pub const LogCallback = logging.LogCallback;

pub const NativePointer = render.NativePointer;
pub const RenderBackendSupport = render.RenderBackendSupport;
pub const RenderTargetExtent = render.RenderTargetExtent;
pub const MetalContextDescriptor = render.MetalContextDescriptor;
pub const VulkanContextDescriptor = render.VulkanContextDescriptor;
pub const MetalOwnedTextureDescriptor = render.MetalOwnedTextureDescriptor;
pub const MetalBorrowedTextureDescriptor = render.MetalBorrowedTextureDescriptor;
pub const VulkanOwnedTextureDescriptor = render.VulkanOwnedTextureDescriptor;
pub const VulkanBorrowedTextureDescriptor = render.VulkanBorrowedTextureDescriptor;
pub const MetalSurfaceDescriptor = render.MetalSurfaceDescriptor;
pub const VulkanSurfaceDescriptor = render.VulkanSurfaceDescriptor;
pub const TextureImageInfo = render.TextureImageInfo;
pub const FeatureStateSelector = render.FeatureStateSelector;
pub const ScreenBox = render.ScreenBox;
pub const RenderedQueryGeometry = render.RenderedQueryGeometry;
pub const RenderedFeatureQueryOptions = render.RenderedFeatureQueryOptions;
pub const SourceFeatureQueryOptions = render.SourceFeatureQueryOptions;
pub const OwnedFeature = render.OwnedFeature;
pub const QueriedFeature = render.QueriedFeature;
pub const FeatureQueryResult = render.FeatureQueryResult;
pub const OwnedFeatureCollection = render.OwnedFeatureCollection;
pub const FeatureExtensionResult = render.FeatureExtensionResult;
pub const MetalOwnedTextureFrameInfo = render.MetalOwnedTextureFrameInfo;
pub const VulkanOwnedTextureFrameInfo = render.VulkanOwnedTextureFrameInfo;
pub const OwnedImage = render.OwnedImage;
pub const RenderSessionHandle = render.RenderSessionHandle;
pub const MetalOwnedTextureFrameHandle = render.MetalOwnedTextureFrameHandle;
pub const VulkanOwnedTextureFrameHandle = render.VulkanOwnedTextureFrameHandle;

pub const MapHandle = map.MapHandle;
pub const MapOptions = map.MapOptions;
pub const MapMode = map.MapMode;
pub const CanonicalTileId = map.CanonicalTileId;
pub const CustomGeometrySourceTileCallback = map.CustomGeometrySourceTileCallback;
pub const CustomGeometrySourceOptions = map.CustomGeometrySourceOptions;
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
pub const BoundOptions = values.BoundOptions;
pub const Vec3 = values.Vec3;
pub const Quaternion = values.Quaternion;
pub const FreeCameraOptions = values.FreeCameraOptions;
pub const ProjectionMode = values.ProjectionMode;
pub const DebugOptions = values.DebugOptions;
pub const NorthOrientation = values.NorthOrientation;
pub const ConstrainMode = values.ConstrainMode;
pub const ViewportMode = values.ViewportMode;
pub const TileLodMode = values.TileLodMode;
pub const ViewportOptions = values.ViewportOptions;
pub const TileOptions = values.TileOptions;
pub const StyleTileScheme = values.StyleTileScheme;
pub const StyleVectorTileEncoding = values.StyleVectorTileEncoding;
pub const StyleRasterDemEncoding = values.StyleRasterDemEncoding;
pub const StyleTileSourceOptions = values.StyleTileSourceOptions;
pub const PremultipliedRgba8Image = values.PremultipliedRgba8Image;
pub const StyleImageOptions = values.StyleImageOptions;
pub const StyleImageInfo = values.StyleImageInfo;
pub const OwnedStyleImage = values.OwnedStyleImage;
pub const LocationIndicatorImageKind = values.LocationIndicatorImageKind;
pub const JsonValue = values.JsonValue;
pub const JsonMember = values.JsonMember;
pub const OwnedJsonValue = values.OwnedJsonValue;
pub const OwnedJsonMember = values.OwnedJsonMember;
pub const StringList = values.StringList;
pub const Geometry = values.Geometry;
pub const OwnedGeometry = values.OwnedGeometry;
pub const FeatureIdentifier = values.FeatureIdentifier;
pub const Feature = values.Feature;
pub const GeoJson = values.GeoJson;
pub const StyleSourceType = values.StyleSourceType;
pub const StyleSourceInfo = values.StyleSourceInfo;
pub const OwnedString = values.OwnedString;

pub const projectedMetersForLatLng = projection.projectedMetersForLatLng;
pub const latLngForProjectedMeters = projection.latLngForProjectedMeters;
pub const getNetworkStatus = runtime.getNetworkStatus;
pub const setNetworkStatus = runtime.setNetworkStatus;
pub const setLogCallback = logging.setLogCallback;
pub const clearLogCallback = logging.clearLogCallback;
pub const setAsyncLogSeverityMask = logging.setAsyncLogSeverityMask;
pub const supportedRenderBackends = render.supportedRenderBackends;
pub const attachMetalOwnedTexture = render.attachMetalOwnedTexture;
pub const attachMetalBorrowedTexture = render.attachMetalBorrowedTexture;
pub const attachVulkanOwnedTexture = render.attachVulkanOwnedTexture;
pub const attachVulkanBorrowedTexture = render.attachVulkanBorrowedTexture;
pub const attachMetalSurface = render.attachMetalSurface;
pub const attachVulkanSurface = render.attachVulkanSurface;

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
