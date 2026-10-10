// Generated from the C headers by tools/bindgen. Do not edit.
part of 'values.dart';

final class AmbientCacheOperation extends _Enum {
  const AmbientCacheOperation.fromRawValue(super.rawValue);
  static const resetDatabase = AmbientCacheOperation.fromRawValue(1);
  static const packDatabase = AmbientCacheOperation.fromRawValue(2);
  static const invalidate = AmbientCacheOperation.fromRawValue(3);
  static const clear = AmbientCacheOperation.fromRawValue(4);
}

final class AnimationOptionField extends _Flags<AnimationOptionField> {
  const AnimationOptionField.fromRawValue(super.rawValue);
  static const duration = AnimationOptionField.fromRawValue(1);
  static const velocity = AnimationOptionField.fromRawValue(2);
  static const minZoom = AnimationOptionField.fromRawValue(4);
  static const easing = AnimationOptionField.fromRawValue(8);
  static const transitionId = AnimationOptionField.fromRawValue(16);
  @override
  AnimationOptionField _of(int rawValue) =>
      AnimationOptionField.fromRawValue(rawValue);
}

final class BoundOptionField extends _Flags<BoundOptionField> {
  const BoundOptionField.fromRawValue(super.rawValue);
  static const bounds = BoundOptionField.fromRawValue(1);
  static const minZoom = BoundOptionField.fromRawValue(2);
  static const maxZoom = BoundOptionField.fromRawValue(4);
  static const minPitch = BoundOptionField.fromRawValue(8);
  static const maxPitch = BoundOptionField.fromRawValue(16);
  static const unbounded = BoundOptionField.fromRawValue(32);
  @override
  BoundOptionField _of(int rawValue) => BoundOptionField.fromRawValue(rawValue);
}

final class CameraChangeMode extends _Enum {
  const CameraChangeMode.fromRawValue(super.rawValue);
  static const immediate = CameraChangeMode.fromRawValue(0);
  static const animated = CameraChangeMode.fromRawValue(1);
}

final class CameraDeltaKind extends _Enum {
  const CameraDeltaKind.fromRawValue(super.rawValue);
  static const move = CameraDeltaKind.fromRawValue(0);
  static const scale = CameraDeltaKind.fromRawValue(1);
  static const bearing = CameraDeltaKind.fromRawValue(2);
  static const pitch = CameraDeltaKind.fromRawValue(3);
}

final class CameraFitOptionField extends _Flags<CameraFitOptionField> {
  const CameraFitOptionField.fromRawValue(super.rawValue);
  static const padding = CameraFitOptionField.fromRawValue(1);
  static const bearing = CameraFitOptionField.fromRawValue(2);
  static const pitch = CameraFitOptionField.fromRawValue(4);
  @override
  CameraFitOptionField _of(int rawValue) =>
      CameraFitOptionField.fromRawValue(rawValue);
}

final class CameraOptionField extends _Flags<CameraOptionField> {
  const CameraOptionField.fromRawValue(super.rawValue);
  static const center = CameraOptionField.fromRawValue(1);
  static const zoom = CameraOptionField.fromRawValue(2);
  static const bearing = CameraOptionField.fromRawValue(4);
  static const pitch = CameraOptionField.fromRawValue(8);
  static const centerAltitude = CameraOptionField.fromRawValue(16);
  static const padding = CameraOptionField.fromRawValue(32);
  static const anchor = CameraOptionField.fromRawValue(64);
  static const roll = CameraOptionField.fromRawValue(128);
  static const fov = CameraOptionField.fromRawValue(256);
  @override
  CameraOptionField _of(int rawValue) =>
      CameraOptionField.fromRawValue(rawValue);
}

final class CameraUpdateMode extends _Enum {
  const CameraUpdateMode.fromRawValue(super.rawValue);
  static const jump = CameraUpdateMode.fromRawValue(0);
  static const ease = CameraUpdateMode.fromRawValue(1);
  static const fly = CameraUpdateMode.fromRawValue(2);
}

final class CommandDisposition extends _Enum {
  const CommandDisposition.fromRawValue(super.rawValue);
  static const committed = CommandDisposition.fromRawValue(0);
  static const superseded = CommandDisposition.fromRawValue(1);
  static const failed = CommandDisposition.fromRawValue(2);
  static const cancelled = CommandDisposition.fromRawValue(3);
}

final class ConstrainMode extends _Enum {
  const ConstrainMode.fromRawValue(super.rawValue);
  static const none = ConstrainMode.fromRawValue(0);
  static const heightOnly = ConstrainMode.fromRawValue(1);
  static const widthAndHeight = ConstrainMode.fromRawValue(2);
  static const screen = ConstrainMode.fromRawValue(3);
}

final class CustomGeometrySourceOptionField
    extends _Flags<CustomGeometrySourceOptionField> {
  const CustomGeometrySourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = CustomGeometrySourceOptionField.fromRawValue(1);
  static const maxZoom = CustomGeometrySourceOptionField.fromRawValue(2);
  static const tolerance = CustomGeometrySourceOptionField.fromRawValue(4);
  static const tileSize = CustomGeometrySourceOptionField.fromRawValue(8);
  static const buffer = CustomGeometrySourceOptionField.fromRawValue(16);
  static const clip = CustomGeometrySourceOptionField.fromRawValue(32);
  static const wrap = CustomGeometrySourceOptionField.fromRawValue(64);
  @override
  CustomGeometrySourceOptionField _of(int rawValue) =>
      CustomGeometrySourceOptionField.fromRawValue(rawValue);
}

final class CustomMvtVectorSourceOptionField
    extends _Flags<CustomMvtVectorSourceOptionField> {
  const CustomMvtVectorSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = CustomMvtVectorSourceOptionField.fromRawValue(1);
  static const maxZoom = CustomMvtVectorSourceOptionField.fromRawValue(2);
  @override
  CustomMvtVectorSourceOptionField _of(int rawValue) =>
      CustomMvtVectorSourceOptionField.fromRawValue(rawValue);
}

final class FeatureStateSelectorField
    extends _Flags<FeatureStateSelectorField> {
  const FeatureStateSelectorField.fromRawValue(super.rawValue);
  static const sourceLayerId = FeatureStateSelectorField.fromRawValue(1);
  static const featureId = FeatureStateSelectorField.fromRawValue(2);
  static const stateKey = FeatureStateSelectorField.fromRawValue(4);
  @override
  FeatureStateSelectorField _of(int rawValue) =>
      FeatureStateSelectorField.fromRawValue(rawValue);
}

final class FrameDemandFlag extends _Flags<FrameDemandFlag> {
  const FrameDemandFlag.fromRawValue(super.rawValue);
  static const ifNeeded = FrameDemandFlag.fromRawValue(1);
  static const present = FrameDemandFlag.fromRawValue(2);
  @override
  FrameDemandFlag _of(int rawValue) => FrameDemandFlag.fromRawValue(rawValue);
}

final class FreeCameraOptionField extends _Flags<FreeCameraOptionField> {
  const FreeCameraOptionField.fromRawValue(super.rawValue);
  static const position = FreeCameraOptionField.fromRawValue(1);
  static const orientation = FreeCameraOptionField.fromRawValue(2);
  @override
  FreeCameraOptionField _of(int rawValue) =>
      FreeCameraOptionField.fromRawValue(rawValue);
}

final class GeojsonSourceOptionField extends _Flags<GeojsonSourceOptionField> {
  const GeojsonSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = GeojsonSourceOptionField.fromRawValue(1);
  static const maxZoom = GeojsonSourceOptionField.fromRawValue(2);
  static const tolerance = GeojsonSourceOptionField.fromRawValue(4);
  static const clusterMaxZoom = GeojsonSourceOptionField.fromRawValue(8);
  static const clusterProperties = GeojsonSourceOptionField.fromRawValue(16);
  static const tileSize = GeojsonSourceOptionField.fromRawValue(32);
  static const buffer = GeojsonSourceOptionField.fromRawValue(64);
  static const clusterRadius = GeojsonSourceOptionField.fromRawValue(128);
  static const clusterMinPoints = GeojsonSourceOptionField.fromRawValue(256);
  static const lineMetrics = GeojsonSourceOptionField.fromRawValue(512);
  static const cluster = GeojsonSourceOptionField.fromRawValue(1024);
  static const synchronousTiling = GeojsonSourceOptionField.fromRawValue(2048);
  @override
  GeojsonSourceOptionField _of(int rawValue) =>
      GeojsonSourceOptionField.fromRawValue(rawValue);
}

final class GesturePhase extends _Enum {
  const GesturePhase.fromRawValue(super.rawValue);
  static const none = GesturePhase.fromRawValue(0);
  static const begin = GesturePhase.fromRawValue(1);
  static const update = GesturePhase.fromRawValue(2);
  static const end = GesturePhase.fromRawValue(3);
  static const cancel = GesturePhase.fromRawValue(4);
}

final class GpuSyncKind extends _Enum {
  const GpuSyncKind.fromRawValue(super.rawValue);
  static const cpuComplete = GpuSyncKind.fromRawValue(0);
  static const metalSharedEvent = GpuSyncKind.fromRawValue(1);
  static const vulkanTimelineSemaphore = GpuSyncKind.fromRawValue(2);
  static const openglFence = GpuSyncKind.fromRawValue(3);
  static const webgpuToken = GpuSyncKind.fromRawValue(4);
}

final class LocationIndicatorImageKind extends _Enum {
  const LocationIndicatorImageKind.fromRawValue(super.rawValue);
  static const top = LocationIndicatorImageKind.fromRawValue(0);
  static const bearing = LocationIndicatorImageKind.fromRawValue(1);
  static const shadow = LocationIndicatorImageKind.fromRawValue(2);
}

final class LogEvent extends _Enum {
  const LogEvent.fromRawValue(super.rawValue);
  static const general = LogEvent.fromRawValue(0);
  static const setup = LogEvent.fromRawValue(1);
  static const shader = LogEvent.fromRawValue(2);
  static const parseStyle = LogEvent.fromRawValue(3);
  static const parseTile = LogEvent.fromRawValue(4);
  static const render = LogEvent.fromRawValue(5);
  static const style = LogEvent.fromRawValue(6);
  static const database = LogEvent.fromRawValue(7);
  static const httpRequest = LogEvent.fromRawValue(8);
  static const sprite = LogEvent.fromRawValue(9);
  static const image = LogEvent.fromRawValue(10);
  static const graphicsBackend = LogEvent.fromRawValue(11);
  static const jni = LogEvent.fromRawValue(12);
  static const android = LogEvent.fromRawValue(13);
  static const crash = LogEvent.fromRawValue(14);
  static const glyph = LogEvent.fromRawValue(15);
  static const timing = LogEvent.fromRawValue(16);
}

final class LogSeverity extends _Enum {
  const LogSeverity.fromRawValue(super.rawValue);
  static const info = LogSeverity.fromRawValue(1);
  static const warning = LogSeverity.fromRawValue(2);
  static const error = LogSeverity.fromRawValue(3);
}

final class LogSeverityMask extends _Flags<LogSeverityMask> {
  const LogSeverityMask.fromRawValue(super.rawValue);
  static const info = LogSeverityMask.fromRawValue(2);
  static const warning = LogSeverityMask.fromRawValue(4);
  static const error = LogSeverityMask.fromRawValue(8);
  static const defaultValue = LogSeverityMask.fromRawValue(6);
  static const all = LogSeverityMask.fromRawValue(14);
  @override
  LogSeverityMask _of(int rawValue) => LogSeverityMask.fromRawValue(rawValue);
}

final class MapDebugOption extends _Flags<MapDebugOption> {
  const MapDebugOption.fromRawValue(super.rawValue);
  static const tileBorders = MapDebugOption.fromRawValue(2);
  static const parseStatus = MapDebugOption.fromRawValue(4);
  static const timestamps = MapDebugOption.fromRawValue(8);
  static const collision = MapDebugOption.fromRawValue(16);
  static const overdraw = MapDebugOption.fromRawValue(32);
  static const stencilClip = MapDebugOption.fromRawValue(64);
  static const depthBuffer = MapDebugOption.fromRawValue(128);
  @override
  MapDebugOption _of(int rawValue) => MapDebugOption.fromRawValue(rawValue);
}

final class MapMode extends _Enum {
  const MapMode.fromRawValue(super.rawValue);
  static const continuous = MapMode.fromRawValue(0);
  static const static = MapMode.fromRawValue(1);
  static const tile = MapMode.fromRawValue(2);
}

final class MapTileOptionField extends _Flags<MapTileOptionField> {
  const MapTileOptionField.fromRawValue(super.rawValue);
  static const prefetchZoomDelta = MapTileOptionField.fromRawValue(1);
  static const lodMinRadius = MapTileOptionField.fromRawValue(2);
  static const lodScale = MapTileOptionField.fromRawValue(4);
  static const lodPitchThreshold = MapTileOptionField.fromRawValue(8);
  static const lodZoomShift = MapTileOptionField.fromRawValue(16);
  static const lodMode = MapTileOptionField.fromRawValue(32);
  @override
  MapTileOptionField _of(int rawValue) =>
      MapTileOptionField.fromRawValue(rawValue);
}

final class MapViewportOptionField extends _Flags<MapViewportOptionField> {
  const MapViewportOptionField.fromRawValue(super.rawValue);
  static const northOrientation = MapViewportOptionField.fromRawValue(1);
  static const constrainMode = MapViewportOptionField.fromRawValue(2);
  static const viewportMode = MapViewportOptionField.fromRawValue(4);
  static const frustumOffset = MapViewportOptionField.fromRawValue(8);
  @override
  MapViewportOptionField _of(int rawValue) =>
      MapViewportOptionField.fromRawValue(rawValue);
}

final class NetworkStatus extends _Enum {
  const NetworkStatus.fromRawValue(super.rawValue);
  static const online = NetworkStatus.fromRawValue(1);
  static const offline = NetworkStatus.fromRawValue(2);
}

final class NorthOrientation extends _Enum {
  const NorthOrientation.fromRawValue(super.rawValue);
  static const up = NorthOrientation.fromRawValue(0);
  static const right = NorthOrientation.fromRawValue(1);
  static const down = NorthOrientation.fromRawValue(2);
  static const left = NorthOrientation.fromRawValue(3);
}

final class OfflineRegionDefinitionType extends _Enum {
  const OfflineRegionDefinitionType.fromRawValue(super.rawValue);
  static const tilePyramid = OfflineRegionDefinitionType.fromRawValue(1);
  static const geometry = OfflineRegionDefinitionType.fromRawValue(2);
}

final class OfflineRegionDownloadState extends _Enum {
  const OfflineRegionDownloadState.fromRawValue(super.rawValue);
  static const inactive = OfflineRegionDownloadState.fromRawValue(0);
  static const active = OfflineRegionDownloadState.fromRawValue(1);
}

final class OpenglClientApi extends _Enum {
  const OpenglClientApi.fromRawValue(super.rawValue);
  static const unspecified = OpenglClientApi.fromRawValue(0);
  static const gl = OpenglClientApi.fromRawValue(1);
  static const gles = OpenglClientApi.fromRawValue(2);
}

final class OpenglContextOwnership extends _Enum {
  const OpenglContextOwnership.fromRawValue(super.rawValue);
  static const shared = OpenglContextOwnership.fromRawValue(0);
  static const dedicated = OpenglContextOwnership.fromRawValue(1);
}

final class OpenglContextPlatform extends _Enum {
  const OpenglContextPlatform.fromRawValue(super.rawValue);
  static const unspecified = OpenglContextPlatform.fromRawValue(0);
  static const wgl = OpenglContextPlatform.fromRawValue(1);
  static const egl = OpenglContextPlatform.fromRawValue(2);
  static const webgl = OpenglContextPlatform.fromRawValue(3);
}

final class OpenglContextProviderFlag
    extends _Flags<OpenglContextProviderFlag> {
  const OpenglContextProviderFlag.fromRawValue(super.rawValue);
  static const wgl = OpenglContextProviderFlag.fromRawValue(1);
  static const egl = OpenglContextProviderFlag.fromRawValue(2);
  static const webgl = OpenglContextProviderFlag.fromRawValue(4);
  @override
  OpenglContextProviderFlag _of(int rawValue) =>
      OpenglContextProviderFlag.fromRawValue(rawValue);
}

final class ProjectionModeField extends _Flags<ProjectionModeField> {
  const ProjectionModeField.fromRawValue(super.rawValue);
  static const axonometric = ProjectionModeField.fromRawValue(1);
  static const xSkew = ProjectionModeField.fromRawValue(2);
  static const ySkew = ProjectionModeField.fromRawValue(4);
  @override
  ProjectionModeField _of(int rawValue) =>
      ProjectionModeField.fromRawValue(rawValue);
}

final class QueriedFeatureField extends _Flags<QueriedFeatureField> {
  const QueriedFeatureField.fromRawValue(super.rawValue);
  static const sourceId = QueriedFeatureField.fromRawValue(1);
  static const sourceLayerId = QueriedFeatureField.fromRawValue(2);
  static const state = QueriedFeatureField.fromRawValue(4);
  @override
  QueriedFeatureField _of(int rawValue) =>
      QueriedFeatureField.fromRawValue(rawValue);
}

final class RenderAbandonDisposition extends _Enum {
  const RenderAbandonDisposition.fromRawValue(super.rawValue);
  static const clean = RenderAbandonDisposition.fromRawValue(0);
  static const quarantined = RenderAbandonDisposition.fromRawValue(1);
}

final class RenderBackendFlag extends _Flags<RenderBackendFlag> {
  const RenderBackendFlag.fromRawValue(super.rawValue);
  static const metal = RenderBackendFlag.fromRawValue(1);
  static const vulkan = RenderBackendFlag.fromRawValue(2);
  static const opengl = RenderBackendFlag.fromRawValue(4);
  static const webgpu = RenderBackendFlag.fromRawValue(8);
  @override
  RenderBackendFlag _of(int rawValue) =>
      RenderBackendFlag.fromRawValue(rawValue);
}

final class RenderDriverKind extends _Enum {
  const RenderDriverKind.fromRawValue(super.rawValue);
  static const coreWorker = RenderDriverKind.fromRawValue(1);
  static const callerGraphicsThread = RenderDriverKind.fromRawValue(2);
}

final class RenderMode extends _Enum {
  const RenderMode.fromRawValue(super.rawValue);
  static const partial = RenderMode.fromRawValue(0);
  static const full = RenderMode.fromRawValue(1);
}

final class RenderResult extends _Enum {
  const RenderResult.fromRawValue(super.rawValue);
  static const rendered = RenderResult.fromRawValue(0);
  static const noUpdate = RenderResult.fromRawValue(1);
  static const sizePending = RenderResult.fromRawValue(2);
  static const targetNotReady = RenderResult.fromRawValue(3);
  static const superseded = RenderResult.fromRawValue(4);
  static const deadlineMissed = RenderResult.fromRawValue(5);
}

final class RenderSessionCapabilityFlag
    extends _Flags<RenderSessionCapabilityFlag> {
  const RenderSessionCapabilityFlag.fromRawValue(super.rawValue);
  static const frameAcquisition = RenderSessionCapabilityFlag.fromRawValue(1);
  static const readback = RenderSessionCapabilityFlag.fromRawValue(2);
  static const consumerSync = RenderSessionCapabilityFlag.fromRawValue(4);
  static const presentation = RenderSessionCapabilityFlag.fromRawValue(8);
  @override
  RenderSessionCapabilityFlag _of(int rawValue) =>
      RenderSessionCapabilityFlag.fromRawValue(rawValue);
}

final class RenderSessionState extends _Enum {
  const RenderSessionState.fromRawValue(super.rawValue);
  static const attaching = RenderSessionState.fromRawValue(1);
  static const attached = RenderSessionState.fromRawValue(2);
  static const detaching = RenderSessionState.fromRawValue(3);
  static const detached = RenderSessionState.fromRawValue(4);
  static const targetLost = RenderSessionState.fromRawValue(5);
  static const abandoned = RenderSessionState.fromRawValue(6);
}

final class RenderedFeatureQueryOptionField
    extends _Flags<RenderedFeatureQueryOptionField> {
  const RenderedFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const ids = RenderedFeatureQueryOptionField.fromRawValue(1);
  @override
  RenderedFeatureQueryOptionField _of(int rawValue) =>
      RenderedFeatureQueryOptionField.fromRawValue(rawValue);
}

final class RenderedQueryGeometryType extends _Enum {
  const RenderedQueryGeometryType.fromRawValue(super.rawValue);
  static const point = RenderedQueryGeometryType.fromRawValue(1);
  static const box = RenderedQueryGeometryType.fromRawValue(2);
  static const lineString = RenderedQueryGeometryType.fromRawValue(3);
}

final class ResourceErrorReason extends _Enum {
  const ResourceErrorReason.fromRawValue(super.rawValue);
  static const none = ResourceErrorReason.fromRawValue(0);
  static const notFound = ResourceErrorReason.fromRawValue(1);
  static const server = ResourceErrorReason.fromRawValue(2);
  static const connection = ResourceErrorReason.fromRawValue(3);
  static const rateLimit = ResourceErrorReason.fromRawValue(4);
  static const other = ResourceErrorReason.fromRawValue(5);
}

final class ResourceKind extends _Enum {
  const ResourceKind.fromRawValue(super.rawValue);
  static const unknown = ResourceKind.fromRawValue(0);
  static const style = ResourceKind.fromRawValue(1);
  static const source = ResourceKind.fromRawValue(2);
  static const tile = ResourceKind.fromRawValue(3);
  static const glyphs = ResourceKind.fromRawValue(4);
  static const spriteImage = ResourceKind.fromRawValue(5);
  static const spriteJson = ResourceKind.fromRawValue(6);
  static const image = ResourceKind.fromRawValue(7);
}

final class ResourceLoadingMethod extends _Enum {
  const ResourceLoadingMethod.fromRawValue(super.rawValue);
  static const all = ResourceLoadingMethod.fromRawValue(0);
  static const cacheOnly = ResourceLoadingMethod.fromRawValue(1);
  static const networkOnly = ResourceLoadingMethod.fromRawValue(2);
}

final class ResourcePriority extends _Enum {
  const ResourcePriority.fromRawValue(super.rawValue);
  static const regular = ResourcePriority.fromRawValue(0);
  static const low = ResourcePriority.fromRawValue(1);
}

final class ResourceProviderDecision extends _Enum {
  const ResourceProviderDecision.fromRawValue(super.rawValue);
  static const passThrough = ResourceProviderDecision.fromRawValue(0);
  static const handle = ResourceProviderDecision.fromRawValue(1);
}

final class ResourceResponseStatus extends _Enum {
  const ResourceResponseStatus.fromRawValue(super.rawValue);
  static const ok = ResourceResponseStatus.fromRawValue(0);
  static const error = ResourceResponseStatus.fromRawValue(1);
  static const noContent = ResourceResponseStatus.fromRawValue(2);
  static const notModified = ResourceResponseStatus.fromRawValue(3);
}

final class ResourceStoragePolicy extends _Enum {
  const ResourceStoragePolicy.fromRawValue(super.rawValue);
  static const permanent = ResourceStoragePolicy.fromRawValue(0);
  static const volatile = ResourceStoragePolicy.fromRawValue(1);
}

final class ResourceUsage extends _Enum {
  const ResourceUsage.fromRawValue(super.rawValue);
  static const online = ResourceUsage.fromRawValue(0);
  static const offline = ResourceUsage.fromRawValue(1);
}

final class RuntimeEventMask extends _Flags<RuntimeEventMask> {
  const RuntimeEventMask.fromRawValue(super.rawValue);
  static const none = RuntimeEventMask.fromRawValue(0);
  static const mapCameraWillChange = RuntimeEventMask.fromRawValue(2);
  static const mapCameraIsChanging = RuntimeEventMask.fromRawValue(4);
  static const mapCameraDidChange = RuntimeEventMask.fromRawValue(8);
  static const mapStyleLoaded = RuntimeEventMask.fromRawValue(16);
  static const mapLoadingStarted = RuntimeEventMask.fromRawValue(32);
  static const mapLoadingFinished = RuntimeEventMask.fromRawValue(64);
  static const mapLoadingFailed = RuntimeEventMask.fromRawValue(128);
  static const mapIdle = RuntimeEventMask.fromRawValue(256);
  static const mapRenderUpdateAvailable = RuntimeEventMask.fromRawValue(512);
  static const mapRenderError = RuntimeEventMask.fromRawValue(1024);
  static const mapStillImageFinished = RuntimeEventMask.fromRawValue(2048);
  static const mapStillImageFailed = RuntimeEventMask.fromRawValue(4096);
  static const mapRenderFrameStarted = RuntimeEventMask.fromRawValue(8192);
  static const mapRenderFrameFinished = RuntimeEventMask.fromRawValue(16384);
  static const mapRenderMapStarted = RuntimeEventMask.fromRawValue(32768);
  static const mapRenderMapFinished = RuntimeEventMask.fromRawValue(65536);
  static const mapStyleImageMissing = RuntimeEventMask.fromRawValue(131072);
  static const mapTileAction = RuntimeEventMask.fromRawValue(262144);
  static const mapCameraTransitionFinished = RuntimeEventMask.fromRawValue(
    4194304,
  );
  static const offlineRegionStatusChanged = RuntimeEventMask.fromRawValue(
    524288,
  );
  static const offlineRegionResponseError = RuntimeEventMask.fromRawValue(
    1048576,
  );
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventMask.fromRawValue(2097152);
  static const allMapEvents = RuntimeEventMask.fromRawValue(4718590);
  static const allRuntimeEvents = RuntimeEventMask.fromRawValue(3670016);
  static const all = RuntimeEventMask.fromRawValue(8388606);
  @override
  RuntimeEventMask _of(int rawValue) => RuntimeEventMask.fromRawValue(rawValue);
}

final class RuntimeEventPayloadType extends _Enum {
  const RuntimeEventPayloadType.fromRawValue(super.rawValue);
  static const none = RuntimeEventPayloadType.fromRawValue(0);
  static const renderFrame = RuntimeEventPayloadType.fromRawValue(1);
  static const renderMap = RuntimeEventPayloadType.fromRawValue(2);
  static const tileAction = RuntimeEventPayloadType.fromRawValue(4);
  static const offlineRegionStatus = RuntimeEventPayloadType.fromRawValue(5);
  static const offlineRegionResponseError =
      RuntimeEventPayloadType.fromRawValue(6);
  static const offlineRegionTileCountLimit =
      RuntimeEventPayloadType.fromRawValue(7);
  static const cameraTransitionFinished = RuntimeEventPayloadType.fromRawValue(
    9,
  );
}

final class RuntimeEventSourceType extends _Enum {
  const RuntimeEventSourceType.fromRawValue(super.rawValue);
  static const runtime = RuntimeEventSourceType.fromRawValue(0);
  static const map = RuntimeEventSourceType.fromRawValue(1);
}

final class RuntimeEventType extends _Enum {
  const RuntimeEventType.fromRawValue(super.rawValue);
  static const mapCameraWillChange = RuntimeEventType.fromRawValue(1);
  static const mapCameraIsChanging = RuntimeEventType.fromRawValue(2);
  static const mapCameraDidChange = RuntimeEventType.fromRawValue(3);
  static const mapStyleLoaded = RuntimeEventType.fromRawValue(4);
  static const mapLoadingStarted = RuntimeEventType.fromRawValue(5);
  static const mapLoadingFinished = RuntimeEventType.fromRawValue(6);
  static const mapLoadingFailed = RuntimeEventType.fromRawValue(7);
  static const mapIdle = RuntimeEventType.fromRawValue(8);
  static const mapRenderUpdateAvailable = RuntimeEventType.fromRawValue(9);
  static const mapRenderError = RuntimeEventType.fromRawValue(10);
  static const mapStillImageFinished = RuntimeEventType.fromRawValue(11);
  static const mapStillImageFailed = RuntimeEventType.fromRawValue(12);
  static const mapRenderFrameStarted = RuntimeEventType.fromRawValue(13);
  static const mapRenderFrameFinished = RuntimeEventType.fromRawValue(14);
  static const mapRenderMapStarted = RuntimeEventType.fromRawValue(15);
  static const mapRenderMapFinished = RuntimeEventType.fromRawValue(16);
  static const mapStyleImageMissing = RuntimeEventType.fromRawValue(17);
  static const mapTileAction = RuntimeEventType.fromRawValue(18);
  static const offlineRegionStatusChanged = RuntimeEventType.fromRawValue(19);
  static const offlineRegionResponseError = RuntimeEventType.fromRawValue(20);
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventType.fromRawValue(21);
  static const mapCameraTransitionFinished = RuntimeEventType.fromRawValue(22);
}

final class SourceFeatureQueryOptionField
    extends _Flags<SourceFeatureQueryOptionField> {
  const SourceFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const ids = SourceFeatureQueryOptionField.fromRawValue(1);
  @override
  SourceFeatureQueryOptionField _of(int rawValue) =>
      SourceFeatureQueryOptionField.fromRawValue(rawValue);
}

final class Status extends _Enum {
  const Status.fromRawValue(super.rawValue);
  static const ok = Status.fromRawValue(0);
  static const invalidArgument = Status.fromRawValue(-1);
  static const invalidState = Status.fromRawValue(-2);
  static const wrongThread = Status.fromRawValue(-3);
  static const unsupported = Status.fromRawValue(-4);
  static const nativeError = Status.fromRawValue(-5);
  static const cancelled = Status.fromRawValue(-6);
  static const busy = Status.fromRawValue(-7);
  static const targetLost = Status.fromRawValue(-8);
  static const notReady = Status.fromRawValue(-9);
  static const notFound = Status.fromRawValue(-10);
}

final class StyleImageOptionField extends _Flags<StyleImageOptionField> {
  const StyleImageOptionField.fromRawValue(super.rawValue);
  static const pixelRatio = StyleImageOptionField.fromRawValue(1);
  static const sdf = StyleImageOptionField.fromRawValue(2);
  static const stretchX = StyleImageOptionField.fromRawValue(4);
  static const stretchY = StyleImageOptionField.fromRawValue(8);
  static const content = StyleImageOptionField.fromRawValue(16);
  static const textFitWidth = StyleImageOptionField.fromRawValue(32);
  static const textFitHeight = StyleImageOptionField.fromRawValue(64);
  @override
  StyleImageOptionField _of(int rawValue) =>
      StyleImageOptionField.fromRawValue(rawValue);
}

final class StyleImageTextFit extends _Enum {
  const StyleImageTextFit.fromRawValue(super.rawValue);
  static const stretchOrShrink = StyleImageTextFit.fromRawValue(0);
  static const stretchOnly = StyleImageTextFit.fromRawValue(1);
  static const proportional = StyleImageTextFit.fromRawValue(2);
}

final class StyleLayerVisibility extends _Enum {
  const StyleLayerVisibility.fromRawValue(super.rawValue);
  static const visible = StyleLayerVisibility.fromRawValue(0);
  static const none = StyleLayerVisibility.fromRawValue(1);
}

final class StyleRasterDemEncoding extends _Enum {
  const StyleRasterDemEncoding.fromRawValue(super.rawValue);
  static const mapbox = StyleRasterDemEncoding.fromRawValue(0);
  static const terrarium = StyleRasterDemEncoding.fromRawValue(1);
}

final class StyleSourceInfoField extends _Flags<StyleSourceInfoField> {
  const StyleSourceInfoField.fromRawValue(super.rawValue);
  static const url = StyleSourceInfoField.fromRawValue(1);
  static const tilejson = StyleSourceInfoField.fromRawValue(2);
  static const bounds = StyleSourceInfoField.fromRawValue(4);
  static const tileSize = StyleSourceInfoField.fromRawValue(8);
  static const vectorEncoding = StyleSourceInfoField.fromRawValue(16);
  static const rasterEncoding = StyleSourceInfoField.fromRawValue(32);
  @override
  StyleSourceInfoField _of(int rawValue) =>
      StyleSourceInfoField.fromRawValue(rawValue);
}

final class StyleSourceType extends _Enum {
  const StyleSourceType.fromRawValue(super.rawValue);
  static const unknown = StyleSourceType.fromRawValue(0);
  static const vector = StyleSourceType.fromRawValue(1);
  static const raster = StyleSourceType.fromRawValue(2);
  static const rasterDem = StyleSourceType.fromRawValue(3);
  static const geojson = StyleSourceType.fromRawValue(4);
  static const image = StyleSourceType.fromRawValue(5);
  static const video = StyleSourceType.fromRawValue(6);
  static const annotations = StyleSourceType.fromRawValue(7);
  static const customVector = StyleSourceType.fromRawValue(8);
  static const customMvtVector = StyleSourceType.fromRawValue(9);
}

final class StyleTileScheme extends _Enum {
  const StyleTileScheme.fromRawValue(super.rawValue);
  static const xyz = StyleTileScheme.fromRawValue(0);
  static const tms = StyleTileScheme.fromRawValue(1);
}

final class StyleTileSourceOptionField
    extends _Flags<StyleTileSourceOptionField> {
  const StyleTileSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = StyleTileSourceOptionField.fromRawValue(1);
  static const maxZoom = StyleTileSourceOptionField.fromRawValue(2);
  static const attribution = StyleTileSourceOptionField.fromRawValue(4);
  static const scheme = StyleTileSourceOptionField.fromRawValue(8);
  static const bounds = StyleTileSourceOptionField.fromRawValue(16);
  static const tileSize = StyleTileSourceOptionField.fromRawValue(32);
  static const vectorEncoding = StyleTileSourceOptionField.fromRawValue(64);
  static const rasterEncoding = StyleTileSourceOptionField.fromRawValue(128);
  @override
  StyleTileSourceOptionField _of(int rawValue) =>
      StyleTileSourceOptionField.fromRawValue(rawValue);
}

final class StyleTransitionOptionField
    extends _Flags<StyleTransitionOptionField> {
  const StyleTransitionOptionField.fromRawValue(super.rawValue);
  static const duration = StyleTransitionOptionField.fromRawValue(1);
  static const delay = StyleTransitionOptionField.fromRawValue(2);
  static const enablePlacementTransitions =
      StyleTransitionOptionField.fromRawValue(4);
  @override
  StyleTransitionOptionField _of(int rawValue) =>
      StyleTransitionOptionField.fromRawValue(rawValue);
}

final class StyleVectorTileEncoding extends _Enum {
  const StyleVectorTileEncoding.fromRawValue(super.rawValue);
  static const mvt = StyleVectorTileEncoding.fromRawValue(0);
  static const mlt = StyleVectorTileEncoding.fromRawValue(1);
}

final class TileLodMode extends _Enum {
  const TileLodMode.fromRawValue(super.rawValue);
  static const defaultValue = TileLodMode.fromRawValue(0);
  static const distance = TileLodMode.fromRawValue(1);
}

final class TileOperation extends _Enum {
  const TileOperation.fromRawValue(super.rawValue);
  static const requestedFromCache = TileOperation.fromRawValue(0);
  static const requestedFromNetwork = TileOperation.fromRawValue(1);
  static const loadFromNetwork = TileOperation.fromRawValue(2);
  static const loadFromCache = TileOperation.fromRawValue(3);
  static const startParse = TileOperation.fromRawValue(4);
  static const endParse = TileOperation.fromRawValue(5);
  static const error = TileOperation.fromRawValue(6);
  static const cancelled = TileOperation.fromRawValue(7);
  static const nullValue = TileOperation.fromRawValue(8);
}

final class ViewportMode extends _Enum {
  const ViewportMode.fromRawValue(super.rawValue);
  static const defaultValue = ViewportMode.fromRawValue(0);
  static const flippedY = ViewportMode.fromRawValue(1);
}

final class WebglContextKind extends _Enum {
  const WebglContextKind.fromRawValue(super.rawValue);
  static const existing = WebglContextKind.fromRawValue(0);
  static const transferredCanvas = WebglContextKind.fromRawValue(1);
}

final class MetalOwnedTextureFrame extends _Value {
  const MetalOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.texture = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    required this.pixelFormat,
  });
  final BigInt generation;
  final int width;
  final int height;
  final double scaleFactor;
  final BigInt frameId;
  final NativePointer texture;
  final NativePointer device;
  final BigInt pixelFormat;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    texture,
    device,
    pixelFormat,
  ];
}

final class OpenglOwnedTextureFrame extends _Value {
  const OpenglOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.texture = 0,
    this.target = 0,
    this.internalFormat = 0,
    this.format = 0,
    this.type = 0,
  });
  final BigInt generation;
  final int width;
  final int height;
  final double scaleFactor;
  final BigInt frameId;
  final int texture;
  final int target;
  final int internalFormat;
  final int format;
  final int type;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    texture,
    target,
    internalFormat,
    format,
    type,
  ];
}

final class GpuSync extends _Value {
  const GpuSync({
    this.kind = const GpuSyncKind.fromRawValue(0),
    required this.object,
    required this.value,
  });
  final GpuSyncKind kind;
  final BigInt object;
  final BigInt value;

  @override
  List<Object?> get _members => [kind, object, value];
}

final class RenderFrameResult extends _Value {
  const RenderFrameResult({
    this.disposition = const RenderResult.fromRawValue(0),
    required this.token,
    required this.mapUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    this.needsRepaint = false,
  });
  final RenderResult disposition;
  final BigInt token;
  final BigInt mapUpdateGeneration;
  final BigInt extentGeneration;
  final BigInt frameGeneration;
  final bool needsRepaint;

  @override
  List<Object?> get _members => [
    disposition,
    token,
    mapUpdateGeneration,
    extentGeneration,
    frameGeneration,
    needsRepaint,
  ];
}

final class VulkanOwnedTextureFrame extends _Value {
  const VulkanOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    required this.image,
    required this.imageView,
    this.device = NativePointer.nullPointer,
    this.format = 0,
    this.layout = 0,
  });
  final BigInt generation;
  final int width;
  final int height;
  final double scaleFactor;
  final BigInt frameId;
  final BigInt image;
  final BigInt imageView;
  final NativePointer device;
  final int format;
  final int layout;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    image,
    imageView,
    device,
    format,
    layout,
  ];
}

final class WebgpuOwnedTextureFrame extends _Value {
  const WebgpuOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.texture = NativePointer.nullPointer,
    this.textureView = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.format = 0,
  });
  final BigInt generation;
  final int width;
  final int height;
  final double scaleFactor;
  final BigInt frameId;
  final NativePointer texture;
  final NativePointer textureView;
  final NativePointer device;
  final int format;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    texture,
    textureView,
    device,
    format,
  ];
}

final class UnitBezier extends _Value {
  const UnitBezier(this.x1, this.y1, this.x2, this.y2);
  final double x1;
  final double y1;
  final double x2;
  final double y2;

  @override
  List<Object?> get _members => [x1, y1, x2, y2];
}

final class AnimationOptions extends _Value {
  const AnimationOptions({
    this.durationMs,
    this.velocity,
    this.minZoom,
    this.easing,
    this.transitionId,
  });
  final double? durationMs;
  final double? velocity;
  final double? minZoom;
  final UnitBezier? easing;
  final BigInt? transitionId;

  @override
  List<Object?> get _members => [
    durationMs,
    velocity,
    minZoom,
    easing,
    transitionId,
  ];
}

final class LatLng extends _Value {
  const LatLng(this.latitude, this.longitude);
  final double latitude;
  final double longitude;

  @override
  List<Object?> get _members => [latitude, longitude];
}

final class LatLngBounds extends _Value {
  const LatLngBounds({
    this.southwest = const LatLng(0, 0),
    this.northeast = const LatLng(0, 0),
  });
  final LatLng southwest;
  final LatLng northeast;

  @override
  List<Object?> get _members => [southwest, northeast];
}

final class BoundOptions extends _Value {
  const BoundOptions({
    this.bounds,
    this.minZoom,
    this.maxZoom,
    this.minPitch,
    this.maxPitch,
    this.unbounded = false,
  });
  final LatLngBounds? bounds;
  final double? minZoom;
  final double? maxZoom;
  final double? minPitch;
  final double? maxPitch;
  final bool unbounded;
  @override
  List<Object?> get _members => [
    bounds,
    minZoom,
    maxZoom,
    minPitch,
    maxPitch,
    unbounded,
  ];
}

final class ScreenPoint extends _Value {
  const ScreenPoint(this.x, this.y);
  final double x;
  final double y;

  @override
  List<Object?> get _members => [x, y];
}

final class CameraDelta extends _Value {
  const CameraDelta({
    this.kind = const CameraDeltaKind.fromRawValue(0),
    this.offset = const ScreenPoint(0, 0),
    this.amount = 0,
    this.anchor,
    this.animation = const AnimationOptions(),
  });
  final CameraDeltaKind kind;
  final ScreenPoint offset;
  final double amount;
  final ScreenPoint? anchor;
  final AnimationOptions animation;

  @override
  List<Object?> get _members => [kind, offset, amount, anchor, animation];
}

final class EdgeInsets extends _Value {
  const EdgeInsets({
    this.top = 0,
    this.left = 0,
    this.bottom = 0,
    this.right = 0,
  });
  final double top;
  final double left;
  final double bottom;
  final double right;

  @override
  List<Object?> get _members => [top, left, bottom, right];
}

final class CameraFitOptions extends _Value {
  const CameraFitOptions({this.padding, this.bearing, this.pitch});
  final EdgeInsets? padding;
  final double? bearing;
  final double? pitch;

  @override
  List<Object?> get _members => [padding, bearing, pitch];
}

final class CameraOptions extends _Value {
  const CameraOptions({
    this.center,
    this.centerAltitude,
    this.padding,
    this.anchor,
    this.zoom,
    this.bearing,
    this.pitch,
    this.roll,
    this.fieldOfView,
  });
  final LatLng? center;
  final double? centerAltitude;
  final EdgeInsets? padding;
  final ScreenPoint? anchor;
  final double? zoom;
  final double? bearing;
  final double? pitch;
  final double? roll;
  final double? fieldOfView;

  @override
  List<Object?> get _members => [
    center,
    centerAltitude,
    padding,
    anchor,
    zoom,
    bearing,
    pitch,
    roll,
    fieldOfView,
  ];
}

final class CameraUpdate extends _Value {
  const CameraUpdate({
    this.mode = const CameraUpdateMode.fromRawValue(0),
    this.camera = const CameraOptions(),
    this.animation = const AnimationOptions(),
    this.gesturePhase = const GesturePhase.fromRawValue(0),
  });
  final CameraUpdateMode mode;
  final CameraOptions camera;
  final AnimationOptions animation;
  final GesturePhase gesturePhase;

  @override
  List<Object?> get _members => [mode, camera, animation, gesturePhase];
}

final class CanonicalTileId extends _Value {
  const CanonicalTileId({this.z = 0, this.x = 0, this.y = 0});
  final int z;
  final int x;
  final int y;

  @override
  List<Object?> get _members => [z, x, y];
}

typedef CustomGeometrySourceTileCallback = void Function(CanonicalTileId);

final class CustomGeometrySourceOptions {
  const CustomGeometrySourceOptions({
    this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.tileSize,
    this.buffer,
    this.clip,
    this.wrap,
  });
  final CustomGeometrySourceTileCallback? fetchTile;
  final CustomGeometrySourceTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;
  final int? tileSize;
  final int? buffer;
  final bool? clip;
  final bool? wrap;
}

typedef CustomMvtVectorSourceTileCallback = void Function(CanonicalTileId);

final class CustomMvtVectorSourceOptions {
  const CustomMvtVectorSourceOptions({
    this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
  });
  final CustomMvtVectorSourceTileCallback? fetchTile;
  final CustomMvtVectorSourceTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
}

final class RenderingStats extends _Value {
  const RenderingStats({
    this.encodingTime = 0,
    this.renderingTime = 0,
    this.frameCount = 0,
    this.drawCallCount = 0,
    this.totalDrawCallCount = 0,
  });
  final double encodingTime;
  final double renderingTime;
  final int frameCount;
  final int drawCallCount;
  final int totalDrawCallCount;

  @override
  List<Object?> get _members => [
    encodingTime,
    renderingTime,
    frameCount,
    drawCallCount,
    totalDrawCallCount,
  ];
}

final class RuntimeEventRenderFrame extends _Value {
  const RuntimeEventRenderFrame({
    this.mode = const RenderMode.fromRawValue(0),
    this.needsRepaint = false,
    this.placementChanged = false,
    this.stats = const RenderingStats(),
  });
  final RenderMode mode;
  final bool needsRepaint;
  final bool placementChanged;
  final RenderingStats stats;

  @override
  List<Object?> get _members => [mode, needsRepaint, placementChanged, stats];
}

final class RuntimeEventRenderMap extends _Value {
  const RuntimeEventRenderMap({this.mode = const RenderMode.fromRawValue(0)});
  final RenderMode mode;

  @override
  List<Object?> get _members => [mode];
}

final class TileId extends _Value {
  const TileId({
    this.overscaledZ = 0,
    this.wrap = 0,
    this.canonicalZ = 0,
    this.canonicalX = 0,
    this.canonicalY = 0,
  });
  final int overscaledZ;
  final int wrap;
  final int canonicalZ;
  final int canonicalX;
  final int canonicalY;

  @override
  List<Object?> get _members => [
    overscaledZ,
    wrap,
    canonicalZ,
    canonicalX,
    canonicalY,
  ];
}

final class RuntimeEventTileAction extends _Value {
  const RuntimeEventTileAction({
    this.operation = const TileOperation.fromRawValue(0),
    this.tileId = const TileId(),
  });
  final TileOperation operation;
  final TileId tileId;

  @override
  List<Object?> get _members => [operation, tileId];
}

final class OfflineRegionStatus extends _Value {
  const OfflineRegionStatus({
    this.downloadState = const OfflineRegionDownloadState.fromRawValue(0),
    required this.completedResourceCount,
    required this.completedResourceSize,
    required this.completedTileCount,
    required this.requiredTileCount,
    required this.completedTileSize,
    required this.requiredResourceCount,
    this.requiredResourceCountIsPrecise = false,
    this.complete = false,
  });
  final OfflineRegionDownloadState downloadState;
  final BigInt completedResourceCount;
  final BigInt completedResourceSize;
  final BigInt completedTileCount;
  final BigInt requiredTileCount;
  final BigInt completedTileSize;
  final BigInt requiredResourceCount;
  final bool requiredResourceCountIsPrecise;
  final bool complete;

  @override
  List<Object?> get _members => [
    downloadState,
    completedResourceCount,
    completedResourceSize,
    completedTileCount,
    requiredTileCount,
    completedTileSize,
    requiredResourceCount,
    requiredResourceCountIsPrecise,
    complete,
  ];
}

final class RuntimeEventOfflineRegionStatus extends _Value {
  const RuntimeEventOfflineRegionStatus({
    this.regionId = 0,
    required this.status,
  });
  final int regionId;
  final OfflineRegionStatus status;

  @override
  List<Object?> get _members => [regionId, status];
}

final class RuntimeEventOfflineRegionResponseError extends _Value {
  const RuntimeEventOfflineRegionResponseError({
    this.regionId = 0,
    this.reason = const ResourceErrorReason.fromRawValue(0),
  });
  final int regionId;
  final ResourceErrorReason reason;

  @override
  List<Object?> get _members => [regionId, reason];
}

final class RuntimeEventOfflineRegionTileCountLimit extends _Value {
  const RuntimeEventOfflineRegionTileCountLimit({
    this.regionId = 0,
    required this.limit,
  });
  final int regionId;
  final BigInt limit;

  @override
  List<Object?> get _members => [regionId, limit];
}

final class RuntimeEventCameraTransitionFinished extends _Value {
  const RuntimeEventCameraTransitionFinished({required this.transitionId});
  final BigInt transitionId;

  @override
  List<Object?> get _members => [transitionId];
}

final class RuntimeEvent {
  const RuntimeEvent({
    required this.type,
    this.sourceType = const RuntimeEventSourceType.fromRawValue(0),
    required this.source,
    this.code = 0,
    required this.messageOffset,
    this.messageSize = 0,
    this.message = '',
    required this.payload,
  });
  final RuntimeEventType type;
  final RuntimeEventSourceType sourceType;
  final BigInt source;
  final int code;
  final BigInt messageOffset;
  final int messageSize;
  final String message;
  final RuntimeEventPayload payload;
}

sealed class RuntimeEventPayload {
  const RuntimeEventPayload._();
}

final class RuntimeEventPayloadRenderFrame extends RuntimeEventPayload {
  const RuntimeEventPayloadRenderFrame(this.value) : super._();
  final RuntimeEventRenderFrame value;
}

final class RuntimeEventPayloadRenderMap extends RuntimeEventPayload {
  const RuntimeEventPayloadRenderMap(this.value) : super._();
  final RuntimeEventRenderMap value;
}

final class RuntimeEventPayloadTileAction extends RuntimeEventPayload {
  const RuntimeEventPayloadTileAction(this.value) : super._();
  final RuntimeEventTileAction value;
}

final class RuntimeEventPayloadOfflineRegionStatus extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionStatus(this.value) : super._();
  final RuntimeEventOfflineRegionStatus value;
}

final class RuntimeEventPayloadOfflineRegionResponseError
    extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionResponseError(this.value) : super._();
  final RuntimeEventOfflineRegionResponseError value;
}

final class RuntimeEventPayloadOfflineRegionTileCountLimit
    extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionTileCountLimit(this.value) : super._();
  final RuntimeEventOfflineRegionTileCountLimit value;
}

final class RuntimeEventPayloadCameraTransitionFinished
    extends RuntimeEventPayload {
  const RuntimeEventPayloadCameraTransitionFinished(this.value) : super._();
  final RuntimeEventCameraTransitionFinished value;
}

final class RuntimeEventPayloadNone extends RuntimeEventPayload {
  const RuntimeEventPayloadNone() : super._();
}

final class RuntimeEventPayloadUnknown extends RuntimeEventPayload {
  RuntimeEventPayloadUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
}

final class RuntimeEventBatchView extends _Value {
  RuntimeEventBatchView({required List<RuntimeEvent> events})
    : events = List.unmodifiable(events);
  final List<RuntimeEvent> events;

  @override
  List<Object?> get _members => [events];
}

final class FrameDemand extends _Value {
  const FrameDemand({
    this.flags = FrameDemandFlag.ifNeeded,
    required this.token,
    required this.coalescingBoundary,
    required this.timeoutNs,
  });
  final FrameDemandFlag flags;
  final BigInt token;
  final BigInt coalescingBoundary;
  final BigInt timeoutNs;

  @override
  List<Object?> get _members => [flags, token, coalescingBoundary, timeoutNs];
}

final class Vec3 extends _Value {
  const Vec3(this.x, this.y, this.z);
  final double x;
  final double y;
  final double z;

  @override
  List<Object?> get _members => [x, y, z];
}

final class Quaternion extends _Value {
  const Quaternion(this.x, this.y, this.z, this.w);
  final double x;
  final double y;
  final double z;
  final double w;

  @override
  List<Object?> get _members => [x, y, z, w];
}

final class FreeCameraOptions extends _Value {
  const FreeCameraOptions({this.position, this.orientation});
  final Vec3? position;
  final Quaternion? orientation;

  @override
  List<Object?> get _members => [position, orientation];
}

final class GeojsonSourceOptions extends _Value {
  GeojsonSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.clusterMaxZoom,
    Uint8List? clusterProperties,
    this.tileSize,
    this.buffer,
    this.clusterRadius,
    this.clusterMinPoints,
    this.lineMetrics,
    this.cluster,
    this.synchronousTiling,
  }) : clusterProperties = clusterProperties == null
           ? null
           : Uint8List.fromList(clusterProperties).asUnmodifiableView();
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;
  final double? clusterMaxZoom;
  final Uint8List? clusterProperties;
  final int? tileSize;
  final int? buffer;
  final int? clusterRadius;
  final int? clusterMinPoints;
  final bool? lineMetrics;
  final bool? cluster;
  final bool? synchronousTiling;

  @override
  List<Object?> get _members => [
    minZoom,
    maxZoom,
    tolerance,
    clusterMaxZoom,
    clusterProperties,
    tileSize,
    buffer,
    clusterRadius,
    clusterMinPoints,
    lineMetrics,
    cluster,
    synchronousTiling,
  ];
}

final class ProjectedMeters extends _Value {
  const ProjectedMeters({this.northing = 0, this.easting = 0});
  final double northing;
  final double easting;

  @override
  List<Object?> get _members => [northing, easting];
}

typedef LogCallback = void Function(LogSeverity, LogEvent, int, String);

final class PremultipliedRgba8Image extends _Value {
  PremultipliedRgba8Image({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    Uint8List? pixels,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
       ).asUnmodifiableView();
  final int width;
  final int height;
  final int stride;
  final Uint8List pixels;

  @override
  List<Object?> get _members => [width, height, stride, pixels];
}

final class StyleTileSourceOptions extends _Value {
  const StyleTileSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.attribution,
    this.scheme,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });
  final double? minZoom;
  final double? maxZoom;
  final String? attribution;
  final StyleTileScheme? scheme;
  final LatLngBounds? bounds;
  final int? tileSize;
  final StyleVectorTileEncoding? vectorEncoding;
  final StyleRasterDemEncoding? rasterEncoding;

  @override
  List<Object?> get _members => [
    minZoom,
    maxZoom,
    attribution,
    scheme,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
}

final class CameraQueryResult extends _Value {
  const CameraQueryResult({
    required this.generation,
    this.camera = const CameraOptions(),
  });
  final BigInt generation;
  final CameraOptions camera;

  @override
  List<Object?> get _members => [generation, camera];
}

final class ImageStretch extends _Value {
  const ImageStretch(this.from, this.to);
  final double from;
  final double to;

  @override
  List<Object?> get _members => [from, to];
}

final class StyleImageStretchesResult extends _Value {
  StyleImageStretchesResult({
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
  }) : stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;

  @override
  List<Object?> get _members => [stretchX, stretchY];
}

final class LogicalExtent extends _Value {
  const LogicalExtent({
    this.width = 256,
    this.height = 256,
    this.scaleFactor = 1.0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  @override
  List<Object?> get _members => [width, height, scaleFactor];
}

final class MapOptions extends _Value {
  const MapOptions({
    this.initialExtent = const LogicalExtent(),
    this.mapMode = const MapMode.fromRawValue(0),
    this.fastPforEnabled = false,
    this.eventMask = RuntimeEventMask.all,
  });
  final LogicalExtent initialExtent;
  final MapMode mapMode;
  final bool fastPforEnabled;
  final RuntimeEventMask eventMask;

  @override
  List<Object?> get _members => [
    initialExtent,
    mapMode,
    fastPforEnabled,
    eventMask,
  ];
}

final class FeatureStateSelector extends _Value {
  const FeatureStateSelector({
    required this.sourceId,
    this.sourceLayerId,
    this.featureId,
    this.stateKey,
  });
  final String sourceId;
  final String? sourceLayerId;
  final String? featureId;
  final String? stateKey;

  @override
  List<Object?> get _members => [sourceId, sourceLayerId, featureId, stateKey];
}

final class ImageContent extends _Value {
  const ImageContent({
    this.left = 0,
    this.top = 0,
    this.right = 0,
    this.bottom = 0,
  });
  final double left;
  final double top;
  final double right;
  final double bottom;

  @override
  List<Object?> get _members => [left, top, right, bottom];
}

final class StyleImageInfo extends _Value {
  const StyleImageInfo({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    this.byteLength = 0,
    this.stretchXCount = 0,
    this.stretchYCount = 0,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
    this.pixelRatio = 1.0,
    this.sdf = false,
  });
  final int width;
  final int height;
  final int stride;
  final int byteLength;
  final int stretchXCount;
  final int stretchYCount;
  final ImageContent? content;
  final StyleImageTextFit? textFitWidth;
  final StyleImageTextFit? textFitHeight;
  final double pixelRatio;
  final bool sdf;

  @override
  List<Object?> get _members => [
    width,
    height,
    stride,
    byteLength,
    stretchXCount,
    stretchYCount,
    content,
    textFitWidth,
    textFitHeight,
    pixelRatio,
    sdf,
  ];
}

final class StyleImageResult extends _Value {
  StyleImageResult({
    this.info = const StyleImageInfo(),
    Uint8List? pixels,
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
       ).asUnmodifiableView(),
       stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final StyleImageInfo info;
  final Uint8List pixels;
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;

  @override
  List<Object?> get _members => [info, pixels, stretchX, stretchY];
}

final class StyleLayerInfo extends _Value {
  const StyleLayerInfo({
    required this.type,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.visibility = const StyleLayerVisibility.fromRawValue(0),
  });
  final String type;
  final double minZoom;
  final double maxZoom;
  final StyleLayerVisibility visibility;

  @override
  List<Object?> get _members => [type, minZoom, maxZoom, visibility];
}

final class StyleLayerResult extends _Value {
  const StyleLayerResult({required this.info, this.sourceId, this.sourceLayer});
  final StyleLayerInfo info;
  final String? sourceId;
  final String? sourceLayer;

  @override
  List<Object?> get _members => [info, sourceId, sourceLayer];
}

final class StyleSourceTileInfo extends _Value {
  const StyleSourceTileInfo({
    this.tileCount = 0,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.scheme = const StyleTileScheme.fromRawValue(0),
  });
  final int tileCount;
  final double minZoom;
  final double maxZoom;
  final StyleTileScheme scheme;

  @override
  List<Object?> get _members => [tileCount, minZoom, maxZoom, scheme];
}

final class StyleSourceInfo extends _Value {
  const StyleSourceInfo({
    this.type = const StyleSourceType.fromRawValue(0),
    this.idSize = 0,
    this.isVolatile = false,
    this.attributionSize,
    this.urlSize,
    this.tilejson,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });
  final StyleSourceType type;
  final int idSize;
  final bool isVolatile;
  final int? attributionSize;
  final int? urlSize;
  final StyleSourceTileInfo? tilejson;
  final LatLngBounds? bounds;
  final int? tileSize;
  final StyleVectorTileEncoding? vectorEncoding;
  final StyleRasterDemEncoding? rasterEncoding;

  @override
  List<Object?> get _members => [
    type,
    idSize,
    isVolatile,
    attributionSize,
    urlSize,
    tilejson,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
}

final class StyleSourceResult extends _Value {
  StyleSourceResult({
    this.info = const StyleSourceInfo(),
    this.attribution,
    this.url,
    List<String>? tileUrls,
  }) : tileUrls = tileUrls == null ? null : List.unmodifiable(tileUrls);
  final StyleSourceInfo info;
  final String? attribution;
  final String? url;
  final List<String>? tileUrls;

  @override
  List<Object?> get _members => [info, attribution, url, tileUrls];
}

final class StyleSourceTileUrlsResult extends _Value {
  StyleSourceTileUrlsResult({required List<String> tileUrls})
    : tileUrls = List.unmodifiable(tileUrls);
  final List<String> tileUrls;

  @override
  List<Object?> get _members => [tileUrls];
}

final class StyleTransitionOptions extends _Value {
  const StyleTransitionOptions({
    this.durationMs,
    this.delayMs,
    this.enablePlacementTransitions,
  });
  final double? durationMs;
  final double? delayMs;
  final bool? enablePlacementTransitions;

  @override
  List<Object?> get _members => [
    durationMs,
    delayMs,
    enablePlacementTransitions,
  ];
}

final class StyleLayerEntry extends _Value {
  const StyleLayerEntry({
    required this.id,
    required this.type,
    this.sourceId,
    this.sourceLayer,
  });
  final String id;
  final String type;
  final String? sourceId;
  final String? sourceLayer;

  @override
  List<Object?> get _members => [id, type, sourceId, sourceLayer];
}

final class ProjectionMode extends _Value {
  const ProjectionMode({this.axonometric, this.xSkew, this.ySkew});
  final bool? axonometric;
  final double? xSkew;
  final double? ySkew;

  @override
  List<Object?> get _members => [axonometric, xSkew, ySkew];
}

final class StyleImageOptions extends _Value {
  StyleImageOptions({
    List<ImageStretch>? stretchX,
    List<ImageStretch>? stretchY,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
    this.pixelRatio,
    this.sdf,
  }) : stretchX = stretchX == null ? null : List.unmodifiable(stretchX),
       stretchY = stretchY == null ? null : List.unmodifiable(stretchY);
  final List<ImageStretch>? stretchX;
  final List<ImageStretch>? stretchY;
  final ImageContent? content;
  final StyleImageTextFit? textFitWidth;
  final StyleImageTextFit? textFitHeight;
  final double? pixelRatio;
  final bool? sdf;

  @override
  List<Object?> get _members => [
    stretchX,
    stretchY,
    content,
    textFitWidth,
    textFitHeight,
    pixelRatio,
    sdf,
  ];
}

final class MapTileOptions extends _Value {
  const MapTileOptions({
    this.prefetchZoomDelta,
    this.lodMinRadius,
    this.lodScale,
    this.lodPitchThreshold,
    this.lodZoomShift,
    this.lodMode,
  });
  final int? prefetchZoomDelta;
  final double? lodMinRadius;
  final double? lodScale;
  final double? lodPitchThreshold;
  final double? lodZoomShift;
  final TileLodMode? lodMode;

  @override
  List<Object?> get _members => [
    prefetchZoomDelta,
    lodMinRadius,
    lodScale,
    lodPitchThreshold,
    lodZoomShift,
    lodMode,
  ];
}

final class MapViewportOptions extends _Value {
  const MapViewportOptions({
    this.northOrientation,
    this.constrainMode,
    this.viewportMode,
    this.frustumOffset,
  });
  final NorthOrientation? northOrientation;
  final ConstrainMode? constrainMode;
  final ViewportMode? viewportMode;
  final EdgeInsets? frustumOffset;

  @override
  List<Object?> get _members => [
    northOrientation,
    constrainMode,
    viewportMode,
    frustumOffset,
  ];
}

final class MapSnapshot extends _Value {
  const MapSnapshot({
    this.debugOptions = const MapDebugOption.fromRawValue(0),
    required this.generation,
    this.camera = const CameraOptions(),
    this.logicalExtent = const LogicalExtent(),
    this.projectionMode = const ProjectionMode(),
    this.viewport = const MapViewportOptions(),
    this.fullyLoaded = false,
    this.renderingStatsViewEnabled = false,
    this.repaintDemand = false,
    this.gestureInProgress = false,
    this.eventMask = const RuntimeEventMask.fromRawValue(0),
    required this.latestRenderUpdateGeneration,
    this.tile = const MapTileOptions(),
    this.bounds = const BoundOptions(),
    this.freeCamera = const FreeCameraOptions(),
  });
  final MapDebugOption debugOptions;
  final BigInt generation;
  final CameraOptions camera;
  final LogicalExtent logicalExtent;
  final ProjectionMode projectionMode;
  final MapViewportOptions viewport;
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;
  final bool repaintDemand;
  final bool gestureInProgress;
  final RuntimeEventMask eventMask;
  final BigInt latestRenderUpdateGeneration;
  final MapTileOptions tile;
  final BoundOptions bounds;
  final FreeCameraOptions freeCamera;

  @override
  List<Object?> get _members => [
    debugOptions,
    generation,
    camera,
    logicalExtent,
    projectionMode,
    viewport,
    fullyLoaded,
    renderingStatsViewEnabled,
    repaintDemand,
    gestureInProgress,
    eventMask,
    latestRenderUpdateGeneration,
    tile,
    bounds,
    freeCamera,
  ];
}

final class RenderTargetExtent extends _Value {
  const RenderTargetExtent({
    this.width = 256,
    this.height = 256,
    this.scaleFactor = 1.0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  @override
  List<Object?> get _members => [width, height, scaleFactor];
}

final class MetalBorrowedTextureDescriptor extends _Value {
  const MetalBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.texture = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final int physicalWidth;
  final int physicalHeight;
  final NativePointer texture;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    texture,
  ];
}

typedef WakeCallback = void Function();

final class Wake {
  const Wake({this.callback});
  final WakeCallback? callback;
}

final class RenderSessionAttachOptions extends _Value {
  const RenderSessionAttachOptions({
    this.driver = RenderDriverKind.callerGraphicsThread,
    this.requestedTextureRingDepth = 1,
    this.frameWake = const Wake(),
    this.driverWorkWake = const Wake(),
  });
  final RenderDriverKind driver;
  final int requestedTextureRingDepth;
  final Wake frameWake;
  final Wake driverWorkWake;

  @override
  List<Object?> get _members => [
    driver,
    requestedTextureRingDepth,
    frameWake,
    driverWorkWake,
  ];
}

final class MetalContextDescriptor extends _Value {
  const MetalContextDescriptor({this.device = NativePointer.nullPointer});
  final NativePointer device;

  @override
  List<Object?> get _members => [device];
}

final class MetalOwnedTextureDescriptor extends _Value {
  const MetalOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final MetalContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

final class MetalSurfaceDescriptor extends _Value {
  const MetalSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
    this.layer = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final MetalContextDescriptor context;
  final NativePointer layer;

  @override
  List<Object?> get _members => [extent, context, layer];
}

final class WglContextDescriptor extends _Value {
  const WglContextDescriptor({
    this.deviceContext = NativePointer.nullPointer,
    this.shareContext = NativePointer.nullPointer,
    this.getProcAddress = NativePointer.nullPointer,
  });
  final NativePointer deviceContext;
  final NativePointer shareContext;
  final NativePointer getProcAddress;

  @override
  List<Object?> get _members => [deviceContext, shareContext, getProcAddress];
}

final class EglContextDescriptor extends _Value {
  const EglContextDescriptor({
    this.display = NativePointer.nullPointer,
    this.config = NativePointer.nullPointer,
    this.shareContext = NativePointer.nullPointer,
    this.clientApi = const OpenglClientApi.fromRawValue(0),
    this.getProcAddress = NativePointer.nullPointer,
  });
  final NativePointer display;
  final NativePointer config;
  final NativePointer shareContext;
  final OpenglClientApi clientApi;
  final NativePointer getProcAddress;

  @override
  List<Object?> get _members => [
    display,
    config,
    shareContext,
    clientApi,
    getProcAddress,
  ];
}

final class WebglContextDescriptor extends _Value {
  const WebglContextDescriptor({
    this.kind = const WebglContextKind.fromRawValue(0),
    this.context = 0,
    required this.canvasSelector,
  });
  final WebglContextKind kind;
  final int context;
  final String canvasSelector;

  @override
  List<Object?> get _members => [kind, context, canvasSelector];
}

final class OpenglContextDescriptor {
  const OpenglContextDescriptor({
    this.ownership = const OpenglContextOwnership.fromRawValue(0),
    required this.data,
  });
  final OpenglContextOwnership ownership;
  final OpenglContextDescriptorData data;
}

sealed class OpenglContextDescriptorData {
  const OpenglContextDescriptorData._();
}

final class OpenglContextDescriptorDataWgl extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataWgl(this.value) : super._();
  final WglContextDescriptor value;
}

final class OpenglContextDescriptorDataEgl extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataEgl(this.value) : super._();
  final EglContextDescriptor value;
}

final class OpenglContextDescriptorDataWebgl
    extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataWebgl(this.value) : super._();
  final WebglContextDescriptor value;
}

final class OpenglContextDescriptorDataUnknown
    extends OpenglContextDescriptorData {
  OpenglContextDescriptorDataUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
}

final class OpenglBorrowedTextureDescriptor extends _Value {
  const OpenglBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    required this.context,
    this.texture = 0,
    this.target = 0,
  });
  final RenderTargetExtent extent;
  final int physicalWidth;
  final int physicalHeight;
  final OpenglContextDescriptor context;
  final int texture;
  final int target;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    texture,
    target,
  ];
}

final class OpenglOwnedTextureDescriptor extends _Value {
  const OpenglOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
  });
  final RenderTargetExtent extent;
  final OpenglContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

final class OpenglSurfaceDescriptor extends _Value {
  const OpenglSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
    this.surface = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final OpenglContextDescriptor context;
  final NativePointer surface;

  @override
  List<Object?> get _members => [extent, context, surface];
}

final class RenderAbandonResult extends _Value {
  const RenderAbandonResult({
    this.disposition = const RenderAbandonDisposition.fromRawValue(0),
    this.quarantinedResourceCount = 0,
  });
  final RenderAbandonDisposition disposition;
  final int quarantinedResourceCount;

  @override
  List<Object?> get _members => [disposition, quarantinedResourceCount];
}

final class RenderSessionCapabilities extends _Value {
  const RenderSessionCapabilities({
    required this.driver,
    this.textureRingDepth = 0,
    this.flags = const RenderSessionCapabilityFlag.fromRawValue(0),
  });
  final RenderDriverKind driver;
  final int textureRingDepth;
  final RenderSessionCapabilityFlag flags;

  @override
  List<Object?> get _members => [driver, textureRingDepth, flags];
}

final class RenderSessionSnapshot extends _Value {
  const RenderSessionSnapshot({
    required this.state,
    required this.driver,
    this.latestResult = const RenderResult.fromRawValue(0),
    this.extent = const RenderTargetExtent(),
    required this.generation,
    required this.mapUpdateGeneration,
    required this.renderedUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    required this.latestDemandToken,
    this.pendingDemandCount = 0,
    this.acquiredFrameCount = 0,
    this.targetReady = false,
    this.pendingChanges = false,
  });
  final RenderSessionState state;
  final RenderDriverKind driver;
  final RenderResult latestResult;
  final RenderTargetExtent extent;
  final BigInt generation;
  final BigInt mapUpdateGeneration;
  final BigInt renderedUpdateGeneration;
  final BigInt extentGeneration;
  final BigInt frameGeneration;
  final BigInt latestDemandToken;
  final int pendingDemandCount;
  final int acquiredFrameCount;
  final bool targetReady;
  final bool pendingChanges;

  @override
  List<Object?> get _members => [
    state,
    driver,
    latestResult,
    extent,
    generation,
    mapUpdateGeneration,
    renderedUpdateGeneration,
    extentGeneration,
    frameGeneration,
    latestDemandToken,
    pendingDemandCount,
    acquiredFrameCount,
    targetReady,
    pendingChanges,
  ];
}

final class ScreenBox extends _Value {
  const ScreenBox({
    this.min = const ScreenPoint(0, 0),
    this.max = const ScreenPoint(0, 0),
  });
  final ScreenPoint min;
  final ScreenPoint max;

  @override
  List<Object?> get _members => [min, max];
}

final class ScreenLineString extends _Value {
  ScreenLineString({required List<ScreenPoint> points})
    : points = List.unmodifiable(points);
  final List<ScreenPoint> points;

  @override
  List<Object?> get _members => [points];
}

sealed class RenderedQueryGeometry {
  const RenderedQueryGeometry._();
  const factory RenderedQueryGeometry.point(ScreenPoint value) =
      RenderedQueryGeometryPoint;
  const factory RenderedQueryGeometry.box(ScreenBox value) =
      RenderedQueryGeometryBox;
  const factory RenderedQueryGeometry.lineString(ScreenLineString value) =
      RenderedQueryGeometryLineString;
}

final class RenderedQueryGeometryPoint extends RenderedQueryGeometry {
  const RenderedQueryGeometryPoint(this.value) : super._();
  final ScreenPoint value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryPoint && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderedQueryGeometryBox extends RenderedQueryGeometry {
  const RenderedQueryGeometryBox(this.value) : super._();
  final ScreenBox value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryBox && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderedQueryGeometryLineString extends RenderedQueryGeometry {
  const RenderedQueryGeometryLineString(this.value) : super._();
  final ScreenLineString value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryLineString && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderedFeatureQueryOptions extends _Value {
  RenderedFeatureQueryOptions({List<String>? layerIds, Uint8List? filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds),
      filter = filter == null
          ? null
          : Uint8List.fromList(filter).asUnmodifiableView();
  final List<String>? layerIds;
  final Uint8List? filter;

  @override
  List<Object?> get _members => [layerIds, filter];
}

final class QueriedFeature extends _Value {
  QueriedFeature({
    Uint8List? feature,
    this.sourceId,
    this.sourceLayerId,
    Uint8List? state,
  }) : feature = Uint8List.fromList(
         feature ?? const <int>[],
       ).asUnmodifiableView(),
       state = state == null
           ? null
           : Uint8List.fromList(state).asUnmodifiableView();
  final Uint8List feature;
  final String? sourceId;
  final String? sourceLayerId;
  final Uint8List? state;

  @override
  List<Object?> get _members => [feature, sourceId, sourceLayerId, state];
}

final class SourceFeatureQueryOptions extends _Value {
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, Uint8List? filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds),
      filter = filter == null
          ? null
          : Uint8List.fromList(filter).asUnmodifiableView();
  final List<String>? sourceLayerIds;
  final Uint8List? filter;

  @override
  List<Object?> get _members => [sourceLayerIds, filter];
}

final class ResourceResponse extends _Value {
  ResourceResponse({
    this.status = const ResourceResponseStatus.fromRawValue(0),
    this.errorReason = const ResourceErrorReason.fromRawValue(0),
    Uint8List? bytes,
    this.errorMessage,
    this.mustRevalidate = false,
    this.modifiedUnixMs,
    this.expiresUnixMs,
    this.etag,
    this.retryAfterUnixMs,
  }) : bytes = Uint8List.fromList(bytes ?? const <int>[]).asUnmodifiableView();
  final ResourceResponseStatus status;
  final ResourceErrorReason errorReason;
  final Uint8List bytes;
  final String? errorMessage;
  final bool mustRevalidate;
  final int? modifiedUnixMs;
  final int? expiresUnixMs;
  final String? etag;
  final int? retryAfterUnixMs;

  @override
  List<Object?> get _members => [
    status,
    errorReason,
    bytes,
    errorMessage,
    mustRevalidate,
    modifiedUnixMs,
    expiresUnixMs,
    etag,
    retryAfterUnixMs,
  ];
}

typedef ResourceRequestCancelCallback = void Function();

final class RuntimeOptions extends _Value {
  const RuntimeOptions({
    this.flags = 0,
    this.assetPath,
    this.cachePath,
    this.eventMask = RuntimeEventMask.all,
    this.eventWake = const Wake(),
  });
  final int flags;
  final String? assetPath;
  final String? cachePath;
  final RuntimeEventMask eventMask;
  final Wake eventWake;

  @override
  List<Object?> get _members => [
    flags,
    assetPath,
    cachePath,
    eventMask,
    eventWake,
  ];
}

final class OfflineTilePyramidRegionDefinition extends _Value {
  const OfflineTilePyramidRegionDefinition({
    required this.styleUrl,
    this.bounds = const LatLngBounds(),
    this.minZoom = 0,
    this.maxZoom = 0,
    this.pixelRatio = 0,
    this.includeIdeographs = false,
  });
  final String styleUrl;
  final LatLngBounds bounds;
  final double minZoom;
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  @override
  List<Object?> get _members => [
    styleUrl,
    bounds,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
}

final class OfflineGeometryRegionDefinition extends _Value {
  OfflineGeometryRegionDefinition({
    required this.styleUrl,
    Uint8List? geometry,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.pixelRatio = 0,
    this.includeIdeographs = false,
  }) : geometry = Uint8List.fromList(
         geometry ?? const <int>[],
       ).asUnmodifiableView();
  final String styleUrl;
  final Uint8List geometry;
  final double minZoom;
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  @override
  List<Object?> get _members => [
    styleUrl,
    geometry,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
}

sealed class OfflineRegionDefinition {
  const OfflineRegionDefinition._();
  const factory OfflineRegionDefinition.tilePyramid(
    OfflineTilePyramidRegionDefinition value,
  ) = OfflineRegionDefinitionTilePyramid;
  const factory OfflineRegionDefinition.geometry(
    OfflineGeometryRegionDefinition value,
  ) = OfflineRegionDefinitionGeometry;
}

final class OfflineRegionDefinitionTilePyramid extends OfflineRegionDefinition {
  const OfflineRegionDefinitionTilePyramid(this.value) : super._();
  final OfflineTilePyramidRegionDefinition value;
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDefinitionTilePyramid && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class OfflineRegionDefinitionGeometry extends OfflineRegionDefinition {
  const OfflineRegionDefinitionGeometry(this.value) : super._();
  final OfflineGeometryRegionDefinition value;
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDefinitionGeometry && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class OfflineRegionInfo extends _Value {
  OfflineRegionInfo({
    this.id = 0,
    required this.definition,
    Uint8List? metadata,
  }) : metadata = Uint8List.fromList(
         metadata ?? const <int>[],
       ).asUnmodifiableView();
  final int id;
  final OfflineRegionDefinition definition;
  final Uint8List metadata;

  @override
  List<Object?> get _members => [id, definition, metadata];
}

final class AdapterUrlMatchFlags extends _Flags<AdapterUrlMatchFlags> {
  const AdapterUrlMatchFlags.fromRawValue(super.rawValue);
  static const flagsNone = AdapterUrlMatchFlags.fromRawValue(0);
  static const glob = AdapterUrlMatchFlags.fromRawValue(1);
  @override
  AdapterUrlMatchFlags _of(int rawValue) =>
      AdapterUrlMatchFlags.fromRawValue(rawValue);
}

final class AdapterHttpHeader extends _Value {
  const AdapterHttpHeader({this.name, this.value});
  final String? name;
  final String? value;

  @override
  List<Object?> get _members => [name, value];
}

final class AdapterHttpHeaderTransformRule extends _Value {
  AdapterHttpHeaderTransformRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.url,
    required List<AdapterHttpHeader> headers,
  }) : headers = List.unmodifiable(headers);
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? url;
  final List<AdapterHttpHeader> headers;

  @override
  List<Object?> get _members => [kind, flags, url, headers];
}

final class AdapterHttpHeaderTransformRules extends _Value {
  AdapterHttpHeaderTransformRules({
    required List<AdapterHttpHeaderTransformRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterHttpHeaderTransformRule> rules;

  @override
  List<Object?> get _members => [rules];
}

sealed class HttpHeaderTransform {
  const HttpHeaderTransform._();
  const factory HttpHeaderTransform.empty() = HttpHeaderTransformEmpty;
  const factory HttpHeaderTransform.httpHeaderTransformRules(
    AdapterHttpHeaderTransformRules value,
  ) = HttpHeaderTransformHttpHeaderTransformRules;
}

final class HttpHeaderTransformEmpty extends HttpHeaderTransform {
  const HttpHeaderTransformEmpty() : super._();
}

final class HttpHeaderTransformHttpHeaderTransformRules
    extends HttpHeaderTransform {
  const HttpHeaderTransformHttpHeaderTransformRules(this.value) : super._();
  final AdapterHttpHeaderTransformRules value;
}

final class AdapterResourceProviderRule extends _Value {
  const AdapterResourceProviderRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.requestedUrl,
    required this.response,
  });
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? requestedUrl;
  final ResourceResponse response;

  @override
  List<Object?> get _members => [kind, flags, requestedUrl, response];
}

final class AdapterResourceProviderRules extends _Value {
  AdapterResourceProviderRules({
    required List<AdapterResourceProviderRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterResourceProviderRule> rules;

  @override
  List<Object?> get _members => [rules];
}

final class AdapterResourceRouteFlags
    extends _Flags<AdapterResourceRouteFlags> {
  const AdapterResourceRouteFlags.fromRawValue(super.rawValue);
  static const flagsNone = AdapterResourceRouteFlags.fromRawValue(0);
  static const matchGlob = AdapterResourceRouteFlags.fromRawValue(1);
  static const useRequestedUrl = AdapterResourceRouteFlags.fromRawValue(2);
  @override
  AdapterResourceRouteFlags _of(int rawValue) =>
      AdapterResourceRouteFlags.fromRawValue(rawValue);
}

final class AdapterResourceRoute extends _Value {
  const AdapterResourceRoute({
    this.kind = 0,
    this.flags = const AdapterResourceRouteFlags.fromRawValue(0),
    this.url,
  });
  final int kind;
  final AdapterResourceRouteFlags flags;
  final String? url;

  @override
  List<Object?> get _members => [kind, flags, url];
}

final class ResourceRequest extends _Value {
  ResourceRequest({
    this.requestedUrl,
    this.resolvedUrl,
    this.kind = const ResourceKind.fromRawValue(0),
    this.loadingMethod = const ResourceLoadingMethod.fromRawValue(0),
    this.priority = const ResourcePriority.fromRawValue(0),
    this.usage = const ResourceUsage.fromRawValue(0),
    this.storagePolicy = const ResourceStoragePolicy.fromRawValue(0),
    this.range,
    this.priorModifiedUnixMs,
    this.priorExpiresUnixMs,
    this.priorEtag,
    Uint8List? priorData,
  }) : priorData = Uint8List.fromList(
         priorData ?? const <int>[],
       ).asUnmodifiableView();
  final String? requestedUrl;
  final String? resolvedUrl;
  final ResourceKind kind;
  final ResourceLoadingMethod loadingMethod;
  final ResourcePriority priority;
  final ResourceUsage usage;
  final ResourceStoragePolicy storagePolicy;
  final ({BigInt rangeStart, BigInt rangeEnd})? range;
  final int? priorModifiedUnixMs;
  final int? priorExpiresUnixMs;
  final String? priorEtag;
  final Uint8List priorData;

  @override
  List<Object?> get _members => [
    requestedUrl,
    resolvedUrl,
    kind,
    loadingMethod,
    priority,
    usage,
    storagePolicy,
    range,
    priorModifiedUnixMs,
    priorExpiresUnixMs,
    priorEtag,
    priorData,
  ];
}

typedef ResourceProviderCallback =
    void Function(ResourceRequest, ResourceRequestHandle);

final class AdapterRoutedResourceProvider extends _Value {
  AdapterRoutedResourceProvider({
    required List<AdapterResourceRoute> routes,
    required this.callback,
  }) : routes = List.unmodifiable(routes);
  final List<AdapterResourceRoute> routes;
  final ResourceProviderCallback callback;

  @override
  List<Object?> get _members => [routes, callback];
}

sealed class ResourceProvider {
  const ResourceProvider._();
  const factory ResourceProvider.empty() = ResourceProviderEmpty;
  const factory ResourceProvider.resourceProviderRules(
    AdapterResourceProviderRules value,
  ) = ResourceProviderResourceProviderRules;
  const factory ResourceProvider.routedResourceProvider(
    AdapterRoutedResourceProvider value,
  ) = ResourceProviderRoutedResourceProvider;
}

final class ResourceProviderEmpty extends ResourceProvider {
  const ResourceProviderEmpty() : super._();
}

final class ResourceProviderResourceProviderRules extends ResourceProvider {
  const ResourceProviderResourceProviderRules(this.value) : super._();
  final AdapterResourceProviderRules value;
}

final class ResourceProviderRoutedResourceProvider extends ResourceProvider {
  const ResourceProviderRoutedResourceProvider(this.value) : super._();
  final AdapterRoutedResourceProvider value;
}

final class AdapterResourceRewriteRule extends _Value {
  const AdapterResourceRewriteRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.url,
    this.replacementUrl,
  });
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? url;
  final String? replacementUrl;

  @override
  List<Object?> get _members => [kind, flags, url, replacementUrl];
}

final class AdapterResourceRewriteRules extends _Value {
  AdapterResourceRewriteRules({required List<AdapterResourceRewriteRule> rules})
    : rules = List.unmodifiable(rules);
  final List<AdapterResourceRewriteRule> rules;

  @override
  List<Object?> get _members => [rules];
}

sealed class ResourceTransform {
  const ResourceTransform._();
  const factory ResourceTransform.empty() = ResourceTransformEmpty;
  const factory ResourceTransform.resourceRewriteRules(
    AdapterResourceRewriteRules value,
  ) = ResourceTransformResourceRewriteRules;
}

final class ResourceTransformEmpty extends ResourceTransform {
  const ResourceTransformEmpty() : super._();
}

final class ResourceTransformResourceRewriteRules extends ResourceTransform {
  const ResourceTransformResourceRewriteRules(this.value) : super._();
  final AdapterResourceRewriteRules value;
}

final class TextureImageInfo extends _Value {
  const TextureImageInfo({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    this.byteLength = 0,
  });
  final int width;
  final int height;
  final int stride;
  final int byteLength;

  @override
  List<Object?> get _members => [width, height, stride, byteLength];
}

final class TextureReadbackResult extends _Value {
  TextureReadbackResult({Uint8List? data, this.info = const TextureImageInfo()})
    : data = Uint8List.fromList(data ?? const <int>[]).asUnmodifiableView();
  final Uint8List data;
  final TextureImageInfo info;

  @override
  List<Object?> get _members => [data, info];
}

final class VulkanContextDescriptor extends _Value {
  const VulkanContextDescriptor({
    this.instance = NativePointer.nullPointer,
    this.physicalDevice = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.graphicsQueue = NativePointer.nullPointer,
    this.graphicsQueueFamilyIndex = 0,
    this.getInstanceProcAddr = NativePointer.nullPointer,
    this.getDeviceProcAddr = NativePointer.nullPointer,
  });
  final NativePointer instance;
  final NativePointer physicalDevice;
  final NativePointer device;
  final NativePointer graphicsQueue;
  final int graphicsQueueFamilyIndex;
  final NativePointer getInstanceProcAddr;
  final NativePointer getDeviceProcAddr;

  @override
  List<Object?> get _members => [
    instance,
    physicalDevice,
    device,
    graphicsQueue,
    graphicsQueueFamilyIndex,
    getInstanceProcAddr,
    getDeviceProcAddr,
  ];
}

final class VulkanBorrowedTextureDescriptor extends _Value {
  const VulkanBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.context = const VulkanContextDescriptor(),
    required this.image,
    required this.imageView,
    this.format = 0,
    this.initialLayout = 0,
    this.finalLayout = 5,
  });
  final RenderTargetExtent extent;
  final int physicalWidth;
  final int physicalHeight;
  final VulkanContextDescriptor context;
  final BigInt image;
  final BigInt imageView;
  final int format;
  final int initialLayout;
  final int finalLayout;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    image,
    imageView,
    format,
    initialLayout,
    finalLayout,
  ];
}

final class VulkanOwnedTextureDescriptor extends _Value {
  const VulkanOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final VulkanContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

final class VulkanSurfaceDescriptor extends _Value {
  const VulkanSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
    required this.surface,
  });
  final RenderTargetExtent extent;
  final VulkanContextDescriptor context;
  final BigInt surface;

  @override
  List<Object?> get _members => [extent, context, surface];
}

final class WebgpuContextDescriptor extends _Value {
  const WebgpuContextDescriptor({
    this.instance = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.queue = NativePointer.nullPointer,
  });
  final NativePointer instance;
  final NativePointer device;
  final NativePointer queue;

  @override
  List<Object?> get _members => [instance, device, queue];
}

final class WebgpuBorrowedTextureDescriptor extends _Value {
  const WebgpuBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.context = const WebgpuContextDescriptor(),
    this.texture = NativePointer.nullPointer,
    this.textureView = NativePointer.nullPointer,
    this.format = 0,
  });
  final RenderTargetExtent extent;
  final int physicalWidth;
  final int physicalHeight;
  final WebgpuContextDescriptor context;
  final NativePointer texture;
  final NativePointer textureView;
  final int format;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    texture,
    textureView,
    format,
  ];
}

final class WebgpuOwnedTextureDescriptor extends _Value {
  const WebgpuOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const WebgpuContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final WebgpuContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

final class WebgpuSurfaceDescriptor extends _Value {
  const WebgpuSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const WebgpuContextDescriptor(),
    this.surface = NativePointer.nullPointer,
    this.format = 0,
  });
  final RenderTargetExtent extent;
  final WebgpuContextDescriptor context;
  final NativePointer surface;
  final int format;

  @override
  List<Object?> get _members => [extent, context, surface, format];
}
