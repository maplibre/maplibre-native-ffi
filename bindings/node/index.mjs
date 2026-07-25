import root from "./index.cjs";

export const MaplibreError = root.MaplibreError;
export const InvalidArgumentError = root.InvalidArgumentError;
export const InvalidStateError = root.InvalidStateError;
export const WrongThreadError = root.WrongThreadError;
export const UnsupportedFeatureError = root.UnsupportedFeatureError;
export const NativeError = root.NativeError;
export const MaplibreStatus = root.MaplibreStatus;
export const RuntimeHandle = root.RuntimeHandle;
export const ResourceRequestHandle = root.ResourceRequestHandle;
export const OfflineOperationHandle = root.OfflineOperationHandle;
export const MapHandle = root.MapHandle;
export const MapProjectionHandle = root.MapProjectionHandle;
export const RenderSessionHandle = root.RenderSessionHandle;
export const MetalOwnedTextureFrame = root.MetalOwnedTextureFrame;
export const VulkanOwnedTextureFrame = root.VulkanOwnedTextureFrame;
export const OpenGLOwnedTextureFrame = root.OpenGLOwnedTextureFrame;
export const NativePointer = root.NativePointer;
export const NativeBuffer = root.NativeBuffer;
export const cVersion = root.cVersion;
export const supportedRenderBackends = root.supportedRenderBackends;
export const supportedOpenGLContextProviders =
  root.supportedOpenGLContextProviders;
export const threadLastErrorMessage = root.threadLastErrorMessage;
export const takeNativeLeakReports = root.takeNativeLeakReports;
export const networkStatus = root.networkStatus;
export const setNetworkStatus = root.setNetworkStatus;
export const projectedMetersForLatLng = root.projectedMetersForLatLng;
export const latLngForProjectedMeters = root.latLngForProjectedMeters;
export const setLogCallback = root.setLogCallback;
export const clearLogCallback = root.clearLogCallback;
export const setAsyncLogSeverities = root.setAsyncLogSeverities;
export const restoreDefaultAsyncLogSeverities =
  root.restoreDefaultAsyncLogSeverities;

export default root;
