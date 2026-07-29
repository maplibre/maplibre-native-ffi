import root from "./index.cjs";

export const MaplibreError = root.MaplibreError;
export const InvalidArgumentError = root.InvalidArgumentError;
export const InvalidStateError = root.InvalidStateError;
export const WrongThreadError = root.WrongThreadError;
export const UnsupportedFeatureError = root.UnsupportedFeatureError;
export const NativeError = root.NativeError;
export const MaplibreStatus = root.MaplibreStatus;
export const RuntimeHandle = root.RuntimeHandle;
export const WakeSourceHandle = root.WakeSourceHandle;
export const ResourceRequestHandle = root.ResourceRequestHandle;
export const OfflineOperationHandle = root.OfflineOperationHandle;
export const MapHandle = root.MapHandle;
export const MapAttachReference = root.MapAttachReference;
export const MapProjectionHandle = root.MapProjectionHandle;
export const RenderSessionHandle = root.RenderSessionHandle;
export const MetalOwnedTextureFrame = root.MetalOwnedTextureFrame;
export const VulkanOwnedTextureFrame = root.VulkanOwnedTextureFrame;
export const OpenGLOwnedTextureFrame = root.OpenGLOwnedTextureFrame;
export const NativePointer = root.NativePointer;
export const NativeBuffer = root.NativeBuffer;
export const RuntimeOptions = root.RuntimeOptions;
export const CameraOptions = root.CameraOptions;
export const AnimationOptions = root.AnimationOptions;
export const FreeCameraOptions = root.FreeCameraOptions;
export const CameraFitOptions = root.CameraFitOptions;
export const MapViewportOptions = root.MapViewportOptions;
export const MapTileOptions = root.MapTileOptions;
export const BoundOptions = root.BoundOptions;
export const ProjectionMode = root.ProjectionMode;
export const MapOptions = root.MapOptions;
export const RenderedFeatureQueryOptions = root.RenderedFeatureQueryOptions;
export const SourceFeatureQueryOptions = root.SourceFeatureQueryOptions;
export const TileSourceOptions = root.TileSourceOptions;
export const StyleImageOptions = root.StyleImageOptions;
export const GeoJsonSourceOptions = root.GeoJsonSourceOptions;
export const cVersion = root.cVersion;
export const supportedRenderBackends = root.supportedRenderBackends;
export const supportedOpenGLContextProviders =
  root.supportedOpenGLContextProviders;
export const renderTargetExtentPhysicalSize =
  root.renderTargetExtentPhysicalSize;
export const threadLastErrorMessage = root.threadLastErrorMessage;
export const takeNativeLeakReports = root.takeNativeLeakReports;
export const networkStatus = root.networkStatus;
export const setNetworkStatus = root.setNetworkStatus;
export const projectedMetersForLatLng = root.projectedMetersForLatLng;
export const latLngForProjectedMeters = root.latLngForProjectedMeters;
export const setLogCallback = root.setLogCallback;
export const clearLogCallback = root.clearLogCallback;
export const setAsyncLogSeverities = root.setAsyncLogSeverities;
export const setAsyncLogSeverityMask = root.setAsyncLogSeverityMask;
export const restoreDefaultAsyncLogSeverities =
  root.restoreDefaultAsyncLogSeverities;

export default root;
