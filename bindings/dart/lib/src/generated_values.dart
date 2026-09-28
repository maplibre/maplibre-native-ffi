// Generated from the C headers by tools/bindgen. Do not edit.
import 'dart:typed_data';
import 'render/native_pointer.dart';

final class AmbientCacheOperation {
  const AmbientCacheOperation.fromRawValue(this.rawValue);
  final int rawValue;
  static const resetDatabase = AmbientCacheOperation.fromRawValue(1);
  static const packDatabase = AmbientCacheOperation.fromRawValue(2);
  static const invalidate = AmbientCacheOperation.fromRawValue(3);
  static const clear = AmbientCacheOperation.fromRawValue(4);
  @override
  bool operator ==(Object other) =>
      other is AmbientCacheOperation && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class AnimationOptionField {
  const AnimationOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const duration = AnimationOptionField.fromRawValue(1);
  static const velocity = AnimationOptionField.fromRawValue(2);
  static const minZoom = AnimationOptionField.fromRawValue(4);
  static const easing = AnimationOptionField.fromRawValue(8);
  static const transitionId = AnimationOptionField.fromRawValue(16);
  AnimationOptionField operator |(AnimationOptionField other) =>
      AnimationOptionField.fromRawValue(rawValue | other.rawValue);
  AnimationOptionField operator &(AnimationOptionField other) =>
      AnimationOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(AnimationOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is AnimationOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class BoundOptionField {
  const BoundOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const bounds = BoundOptionField.fromRawValue(1);
  static const minZoom = BoundOptionField.fromRawValue(2);
  static const maxZoom = BoundOptionField.fromRawValue(4);
  static const minPitch = BoundOptionField.fromRawValue(8);
  static const maxPitch = BoundOptionField.fromRawValue(16);
  static const unbounded = BoundOptionField.fromRawValue(32);
  BoundOptionField operator |(BoundOptionField other) =>
      BoundOptionField.fromRawValue(rawValue | other.rawValue);
  BoundOptionField operator &(BoundOptionField other) =>
      BoundOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(BoundOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is BoundOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CameraChangeMode {
  const CameraChangeMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const immediate = CameraChangeMode.fromRawValue(0);
  static const animated = CameraChangeMode.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is CameraChangeMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CameraDeltaKind {
  const CameraDeltaKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const move = CameraDeltaKind.fromRawValue(0);
  static const scale = CameraDeltaKind.fromRawValue(1);
  static const bearing = CameraDeltaKind.fromRawValue(2);
  static const pitch = CameraDeltaKind.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is CameraDeltaKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CameraFitOptionField {
  const CameraFitOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const padding = CameraFitOptionField.fromRawValue(1);
  static const bearing = CameraFitOptionField.fromRawValue(2);
  static const pitch = CameraFitOptionField.fromRawValue(4);
  CameraFitOptionField operator |(CameraFitOptionField other) =>
      CameraFitOptionField.fromRawValue(rawValue | other.rawValue);
  CameraFitOptionField operator &(CameraFitOptionField other) =>
      CameraFitOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(CameraFitOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is CameraFitOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CameraOptionField {
  const CameraOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const center = CameraOptionField.fromRawValue(1);
  static const zoom = CameraOptionField.fromRawValue(2);
  static const bearing = CameraOptionField.fromRawValue(4);
  static const pitch = CameraOptionField.fromRawValue(8);
  static const centerAltitude = CameraOptionField.fromRawValue(16);
  static const padding = CameraOptionField.fromRawValue(32);
  static const anchor = CameraOptionField.fromRawValue(64);
  static const roll = CameraOptionField.fromRawValue(128);
  static const fov = CameraOptionField.fromRawValue(256);
  CameraOptionField operator |(CameraOptionField other) =>
      CameraOptionField.fromRawValue(rawValue | other.rawValue);
  CameraOptionField operator &(CameraOptionField other) =>
      CameraOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(CameraOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is CameraOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CameraUpdateMode {
  const CameraUpdateMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const jump = CameraUpdateMode.fromRawValue(0);
  static const ease = CameraUpdateMode.fromRawValue(1);
  static const fly = CameraUpdateMode.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is CameraUpdateMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CommandDisposition {
  const CommandDisposition.fromRawValue(this.rawValue);
  final int rawValue;
  static const committed = CommandDisposition.fromRawValue(0);
  static const superseded = CommandDisposition.fromRawValue(1);
  static const failed = CommandDisposition.fromRawValue(2);
  static const cancelled = CommandDisposition.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is CommandDisposition && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ConstrainMode {
  const ConstrainMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const none = ConstrainMode.fromRawValue(0);
  static const heightOnly = ConstrainMode.fromRawValue(1);
  static const widthAndHeight = ConstrainMode.fromRawValue(2);
  static const screen = ConstrainMode.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is ConstrainMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CustomGeometrySourceOptionField {
  const CustomGeometrySourceOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const minZoom = CustomGeometrySourceOptionField.fromRawValue(1);
  static const maxZoom = CustomGeometrySourceOptionField.fromRawValue(2);
  static const tolerance = CustomGeometrySourceOptionField.fromRawValue(4);
  static const tileSize = CustomGeometrySourceOptionField.fromRawValue(8);
  static const buffer = CustomGeometrySourceOptionField.fromRawValue(16);
  static const clip = CustomGeometrySourceOptionField.fromRawValue(32);
  static const wrap = CustomGeometrySourceOptionField.fromRawValue(64);
  CustomGeometrySourceOptionField operator |(
    CustomGeometrySourceOptionField other,
  ) => CustomGeometrySourceOptionField.fromRawValue(rawValue | other.rawValue);
  CustomGeometrySourceOptionField operator &(
    CustomGeometrySourceOptionField other,
  ) => CustomGeometrySourceOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(CustomGeometrySourceOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is CustomGeometrySourceOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class CustomMvtVectorSourceOptionField {
  const CustomMvtVectorSourceOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const minZoom = CustomMvtVectorSourceOptionField.fromRawValue(1);
  static const maxZoom = CustomMvtVectorSourceOptionField.fromRawValue(2);
  CustomMvtVectorSourceOptionField operator |(
    CustomMvtVectorSourceOptionField other,
  ) => CustomMvtVectorSourceOptionField.fromRawValue(rawValue | other.rawValue);
  CustomMvtVectorSourceOptionField operator &(
    CustomMvtVectorSourceOptionField other,
  ) => CustomMvtVectorSourceOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(CustomMvtVectorSourceOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is CustomMvtVectorSourceOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class FeatureStateSelectorField {
  const FeatureStateSelectorField.fromRawValue(this.rawValue);
  final int rawValue;
  static const sourceLayerId = FeatureStateSelectorField.fromRawValue(1);
  static const featureId = FeatureStateSelectorField.fromRawValue(2);
  static const stateKey = FeatureStateSelectorField.fromRawValue(4);
  FeatureStateSelectorField operator |(FeatureStateSelectorField other) =>
      FeatureStateSelectorField.fromRawValue(rawValue | other.rawValue);
  FeatureStateSelectorField operator &(FeatureStateSelectorField other) =>
      FeatureStateSelectorField.fromRawValue(rawValue & other.rawValue);
  bool contains(FeatureStateSelectorField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is FeatureStateSelectorField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class FrameDemandFlag {
  const FrameDemandFlag.fromRawValue(this.rawValue);
  final int rawValue;
  static const ifNeeded = FrameDemandFlag.fromRawValue(1);
  static const present = FrameDemandFlag.fromRawValue(2);
  FrameDemandFlag operator |(FrameDemandFlag other) =>
      FrameDemandFlag.fromRawValue(rawValue | other.rawValue);
  FrameDemandFlag operator &(FrameDemandFlag other) =>
      FrameDemandFlag.fromRawValue(rawValue & other.rawValue);
  bool contains(FrameDemandFlag other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is FrameDemandFlag && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class FreeCameraOptionField {
  const FreeCameraOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const position = FreeCameraOptionField.fromRawValue(1);
  static const orientation = FreeCameraOptionField.fromRawValue(2);
  FreeCameraOptionField operator |(FreeCameraOptionField other) =>
      FreeCameraOptionField.fromRawValue(rawValue | other.rawValue);
  FreeCameraOptionField operator &(FreeCameraOptionField other) =>
      FreeCameraOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(FreeCameraOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is FreeCameraOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class GeojsonSourceOptionField {
  const GeojsonSourceOptionField.fromRawValue(this.rawValue);
  final int rawValue;
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
  GeojsonSourceOptionField operator |(GeojsonSourceOptionField other) =>
      GeojsonSourceOptionField.fromRawValue(rawValue | other.rawValue);
  GeojsonSourceOptionField operator &(GeojsonSourceOptionField other) =>
      GeojsonSourceOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(GeojsonSourceOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is GeojsonSourceOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class GesturePhase {
  const GesturePhase.fromRawValue(this.rawValue);
  final int rawValue;
  static const none = GesturePhase.fromRawValue(0);
  static const begin = GesturePhase.fromRawValue(1);
  static const update = GesturePhase.fromRawValue(2);
  static const end = GesturePhase.fromRawValue(3);
  static const cancel = GesturePhase.fromRawValue(4);
  @override
  bool operator ==(Object other) =>
      other is GesturePhase && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class GpuSyncKind {
  const GpuSyncKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const cpuComplete = GpuSyncKind.fromRawValue(0);
  static const metalSharedEvent = GpuSyncKind.fromRawValue(1);
  static const vulkanTimelineSemaphore = GpuSyncKind.fromRawValue(2);
  static const openglFence = GpuSyncKind.fromRawValue(3);
  static const webgpuToken = GpuSyncKind.fromRawValue(4);
  @override
  bool operator ==(Object other) =>
      other is GpuSyncKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class LocationIndicatorImageKind {
  const LocationIndicatorImageKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const top = LocationIndicatorImageKind.fromRawValue(0);
  static const bearing = LocationIndicatorImageKind.fromRawValue(1);
  static const shadow = LocationIndicatorImageKind.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is LocationIndicatorImageKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class LogEvent {
  const LogEvent.fromRawValue(this.rawValue);
  final int rawValue;
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
  @override
  bool operator ==(Object other) =>
      other is LogEvent && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class LogSeverity {
  const LogSeverity.fromRawValue(this.rawValue);
  final int rawValue;
  static const info = LogSeverity.fromRawValue(1);
  static const warning = LogSeverity.fromRawValue(2);
  static const error = LogSeverity.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is LogSeverity && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class LogSeverityMask {
  const LogSeverityMask.fromRawValue(this.rawValue);
  final int rawValue;
  static const info = LogSeverityMask.fromRawValue(2);
  static const warning = LogSeverityMask.fromRawValue(4);
  static const error = LogSeverityMask.fromRawValue(8);
  static const defaultValue = LogSeverityMask.fromRawValue(6);
  static const all = LogSeverityMask.fromRawValue(14);
  LogSeverityMask operator |(LogSeverityMask other) =>
      LogSeverityMask.fromRawValue(rawValue | other.rawValue);
  LogSeverityMask operator &(LogSeverityMask other) =>
      LogSeverityMask.fromRawValue(rawValue & other.rawValue);
  bool contains(LogSeverityMask other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is LogSeverityMask && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class MapDebugOption {
  const MapDebugOption.fromRawValue(this.rawValue);
  final int rawValue;
  static const tileBorders = MapDebugOption.fromRawValue(2);
  static const parseStatus = MapDebugOption.fromRawValue(4);
  static const timestamps = MapDebugOption.fromRawValue(8);
  static const collision = MapDebugOption.fromRawValue(16);
  static const overdraw = MapDebugOption.fromRawValue(32);
  static const stencilClip = MapDebugOption.fromRawValue(64);
  static const depthBuffer = MapDebugOption.fromRawValue(128);
  MapDebugOption operator |(MapDebugOption other) =>
      MapDebugOption.fromRawValue(rawValue | other.rawValue);
  MapDebugOption operator &(MapDebugOption other) =>
      MapDebugOption.fromRawValue(rawValue & other.rawValue);
  bool contains(MapDebugOption other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is MapDebugOption && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class MapMode {
  const MapMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const continuous = MapMode.fromRawValue(0);
  static const static = MapMode.fromRawValue(1);
  static const tile = MapMode.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is MapMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class MapTileOptionField {
  const MapTileOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const prefetchZoomDelta = MapTileOptionField.fromRawValue(1);
  static const lodMinRadius = MapTileOptionField.fromRawValue(2);
  static const lodScale = MapTileOptionField.fromRawValue(4);
  static const lodPitchThreshold = MapTileOptionField.fromRawValue(8);
  static const lodZoomShift = MapTileOptionField.fromRawValue(16);
  static const lodMode = MapTileOptionField.fromRawValue(32);
  MapTileOptionField operator |(MapTileOptionField other) =>
      MapTileOptionField.fromRawValue(rawValue | other.rawValue);
  MapTileOptionField operator &(MapTileOptionField other) =>
      MapTileOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(MapTileOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is MapTileOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class MapViewportOptionField {
  const MapViewportOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const northOrientation = MapViewportOptionField.fromRawValue(1);
  static const constrainMode = MapViewportOptionField.fromRawValue(2);
  static const viewportMode = MapViewportOptionField.fromRawValue(4);
  static const frustumOffset = MapViewportOptionField.fromRawValue(8);
  MapViewportOptionField operator |(MapViewportOptionField other) =>
      MapViewportOptionField.fromRawValue(rawValue | other.rawValue);
  MapViewportOptionField operator &(MapViewportOptionField other) =>
      MapViewportOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(MapViewportOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is MapViewportOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class NetworkStatus {
  const NetworkStatus.fromRawValue(this.rawValue);
  final int rawValue;
  static const online = NetworkStatus.fromRawValue(1);
  static const offline = NetworkStatus.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is NetworkStatus && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class NorthOrientation {
  const NorthOrientation.fromRawValue(this.rawValue);
  final int rawValue;
  static const up = NorthOrientation.fromRawValue(0);
  static const right = NorthOrientation.fromRawValue(1);
  static const down = NorthOrientation.fromRawValue(2);
  static const left = NorthOrientation.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is NorthOrientation && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OfflineRegionDefinitionType {
  const OfflineRegionDefinitionType.fromRawValue(this.rawValue);
  final int rawValue;
  static const tilePyramid = OfflineRegionDefinitionType.fromRawValue(1);
  static const geometry = OfflineRegionDefinitionType.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDefinitionType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OfflineRegionDownloadState {
  const OfflineRegionDownloadState.fromRawValue(this.rawValue);
  final int rawValue;
  static const inactive = OfflineRegionDownloadState.fromRawValue(0);
  static const active = OfflineRegionDownloadState.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDownloadState && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OpenglClientApi {
  const OpenglClientApi.fromRawValue(this.rawValue);
  final int rawValue;
  static const unspecified = OpenglClientApi.fromRawValue(0);
  static const gl = OpenglClientApi.fromRawValue(1);
  static const gles = OpenglClientApi.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is OpenglClientApi && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OpenglContextOwnership {
  const OpenglContextOwnership.fromRawValue(this.rawValue);
  final int rawValue;
  static const shared = OpenglContextOwnership.fromRawValue(0);
  static const dedicated = OpenglContextOwnership.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is OpenglContextOwnership && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OpenglContextPlatform {
  const OpenglContextPlatform.fromRawValue(this.rawValue);
  final int rawValue;
  static const unspecified = OpenglContextPlatform.fromRawValue(0);
  static const wgl = OpenglContextPlatform.fromRawValue(1);
  static const egl = OpenglContextPlatform.fromRawValue(2);
  static const webgl = OpenglContextPlatform.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is OpenglContextPlatform && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class OpenglContextProviderFlag {
  const OpenglContextProviderFlag.fromRawValue(this.rawValue);
  final int rawValue;
  static const wgl = OpenglContextProviderFlag.fromRawValue(1);
  static const egl = OpenglContextProviderFlag.fromRawValue(2);
  static const webgl = OpenglContextProviderFlag.fromRawValue(4);
  OpenglContextProviderFlag operator |(OpenglContextProviderFlag other) =>
      OpenglContextProviderFlag.fromRawValue(rawValue | other.rawValue);
  OpenglContextProviderFlag operator &(OpenglContextProviderFlag other) =>
      OpenglContextProviderFlag.fromRawValue(rawValue & other.rawValue);
  bool contains(OpenglContextProviderFlag other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is OpenglContextProviderFlag && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ProjectionModeField {
  const ProjectionModeField.fromRawValue(this.rawValue);
  final int rawValue;
  static const axonometric = ProjectionModeField.fromRawValue(1);
  static const xSkew = ProjectionModeField.fromRawValue(2);
  static const ySkew = ProjectionModeField.fromRawValue(4);
  ProjectionModeField operator |(ProjectionModeField other) =>
      ProjectionModeField.fromRawValue(rawValue | other.rawValue);
  ProjectionModeField operator &(ProjectionModeField other) =>
      ProjectionModeField.fromRawValue(rawValue & other.rawValue);
  bool contains(ProjectionModeField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is ProjectionModeField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class QueriedFeatureField {
  const QueriedFeatureField.fromRawValue(this.rawValue);
  final int rawValue;
  static const sourceId = QueriedFeatureField.fromRawValue(1);
  static const sourceLayerId = QueriedFeatureField.fromRawValue(2);
  static const state = QueriedFeatureField.fromRawValue(4);
  QueriedFeatureField operator |(QueriedFeatureField other) =>
      QueriedFeatureField.fromRawValue(rawValue | other.rawValue);
  QueriedFeatureField operator &(QueriedFeatureField other) =>
      QueriedFeatureField.fromRawValue(rawValue & other.rawValue);
  bool contains(QueriedFeatureField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is QueriedFeatureField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderAbandonDisposition {
  const RenderAbandonDisposition.fromRawValue(this.rawValue);
  final int rawValue;
  static const clean = RenderAbandonDisposition.fromRawValue(0);
  static const quarantined = RenderAbandonDisposition.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is RenderAbandonDisposition && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderBackendFlag {
  const RenderBackendFlag.fromRawValue(this.rawValue);
  final int rawValue;
  static const metal = RenderBackendFlag.fromRawValue(1);
  static const vulkan = RenderBackendFlag.fromRawValue(2);
  static const opengl = RenderBackendFlag.fromRawValue(4);
  static const webgpu = RenderBackendFlag.fromRawValue(8);
  RenderBackendFlag operator |(RenderBackendFlag other) =>
      RenderBackendFlag.fromRawValue(rawValue | other.rawValue);
  RenderBackendFlag operator &(RenderBackendFlag other) =>
      RenderBackendFlag.fromRawValue(rawValue & other.rawValue);
  bool contains(RenderBackendFlag other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is RenderBackendFlag && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderDriverKind {
  const RenderDriverKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const coreWorker = RenderDriverKind.fromRawValue(1);
  static const callerGraphicsThread = RenderDriverKind.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is RenderDriverKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderMode {
  const RenderMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const partial = RenderMode.fromRawValue(0);
  static const full = RenderMode.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is RenderMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderResult {
  const RenderResult.fromRawValue(this.rawValue);
  final int rawValue;
  static const rendered = RenderResult.fromRawValue(0);
  static const noUpdate = RenderResult.fromRawValue(1);
  static const sizePending = RenderResult.fromRawValue(2);
  static const targetNotReady = RenderResult.fromRawValue(3);
  static const superseded = RenderResult.fromRawValue(4);
  static const deadlineMissed = RenderResult.fromRawValue(5);
  @override
  bool operator ==(Object other) =>
      other is RenderResult && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderSessionCapabilityFlag {
  const RenderSessionCapabilityFlag.fromRawValue(this.rawValue);
  final int rawValue;
  static const frameAcquisition = RenderSessionCapabilityFlag.fromRawValue(1);
  static const readback = RenderSessionCapabilityFlag.fromRawValue(2);
  static const consumerSync = RenderSessionCapabilityFlag.fromRawValue(4);
  static const presentation = RenderSessionCapabilityFlag.fromRawValue(8);
  RenderSessionCapabilityFlag operator |(RenderSessionCapabilityFlag other) =>
      RenderSessionCapabilityFlag.fromRawValue(rawValue | other.rawValue);
  RenderSessionCapabilityFlag operator &(RenderSessionCapabilityFlag other) =>
      RenderSessionCapabilityFlag.fromRawValue(rawValue & other.rawValue);
  bool contains(RenderSessionCapabilityFlag other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is RenderSessionCapabilityFlag && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderSessionState {
  const RenderSessionState.fromRawValue(this.rawValue);
  final int rawValue;
  static const attaching = RenderSessionState.fromRawValue(1);
  static const attached = RenderSessionState.fromRawValue(2);
  static const detaching = RenderSessionState.fromRawValue(3);
  static const detached = RenderSessionState.fromRawValue(4);
  static const targetLost = RenderSessionState.fromRawValue(5);
  static const abandoned = RenderSessionState.fromRawValue(6);
  @override
  bool operator ==(Object other) =>
      other is RenderSessionState && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderedFeatureQueryOptionField {
  const RenderedFeatureQueryOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const ids = RenderedFeatureQueryOptionField.fromRawValue(1);
  RenderedFeatureQueryOptionField operator |(
    RenderedFeatureQueryOptionField other,
  ) => RenderedFeatureQueryOptionField.fromRawValue(rawValue | other.rawValue);
  RenderedFeatureQueryOptionField operator &(
    RenderedFeatureQueryOptionField other,
  ) => RenderedFeatureQueryOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(RenderedFeatureQueryOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is RenderedFeatureQueryOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RenderedQueryGeometryType {
  const RenderedQueryGeometryType.fromRawValue(this.rawValue);
  final int rawValue;
  static const point = RenderedQueryGeometryType.fromRawValue(1);
  static const box = RenderedQueryGeometryType.fromRawValue(2);
  static const lineString = RenderedQueryGeometryType.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceErrorReason {
  const ResourceErrorReason.fromRawValue(this.rawValue);
  final int rawValue;
  static const none = ResourceErrorReason.fromRawValue(0);
  static const notFound = ResourceErrorReason.fromRawValue(1);
  static const server = ResourceErrorReason.fromRawValue(2);
  static const connection = ResourceErrorReason.fromRawValue(3);
  static const rateLimit = ResourceErrorReason.fromRawValue(4);
  static const other = ResourceErrorReason.fromRawValue(5);
  @override
  bool operator ==(Object other) =>
      other is ResourceErrorReason && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceKind {
  const ResourceKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const unknown = ResourceKind.fromRawValue(0);
  static const style = ResourceKind.fromRawValue(1);
  static const source = ResourceKind.fromRawValue(2);
  static const tile = ResourceKind.fromRawValue(3);
  static const glyphs = ResourceKind.fromRawValue(4);
  static const spriteImage = ResourceKind.fromRawValue(5);
  static const spriteJson = ResourceKind.fromRawValue(6);
  static const image = ResourceKind.fromRawValue(7);
  @override
  bool operator ==(Object other) =>
      other is ResourceKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceLoadingMethod {
  const ResourceLoadingMethod.fromRawValue(this.rawValue);
  final int rawValue;
  static const all = ResourceLoadingMethod.fromRawValue(0);
  static const cacheOnly = ResourceLoadingMethod.fromRawValue(1);
  static const networkOnly = ResourceLoadingMethod.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is ResourceLoadingMethod && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourcePriority {
  const ResourcePriority.fromRawValue(this.rawValue);
  final int rawValue;
  static const regular = ResourcePriority.fromRawValue(0);
  static const low = ResourcePriority.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is ResourcePriority && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceProviderDecision {
  const ResourceProviderDecision.fromRawValue(this.rawValue);
  final int rawValue;
  static const passThrough = ResourceProviderDecision.fromRawValue(0);
  static const handle = ResourceProviderDecision.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is ResourceProviderDecision && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceResponseStatus {
  const ResourceResponseStatus.fromRawValue(this.rawValue);
  final int rawValue;
  static const ok = ResourceResponseStatus.fromRawValue(0);
  static const error = ResourceResponseStatus.fromRawValue(1);
  static const noContent = ResourceResponseStatus.fromRawValue(2);
  static const notModified = ResourceResponseStatus.fromRawValue(3);
  @override
  bool operator ==(Object other) =>
      other is ResourceResponseStatus && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceStoragePolicy {
  const ResourceStoragePolicy.fromRawValue(this.rawValue);
  final int rawValue;
  static const permanent = ResourceStoragePolicy.fromRawValue(0);
  static const volatile = ResourceStoragePolicy.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is ResourceStoragePolicy && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceUsage {
  const ResourceUsage.fromRawValue(this.rawValue);
  final int rawValue;
  static const online = ResourceUsage.fromRawValue(0);
  static const offline = ResourceUsage.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is ResourceUsage && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RuntimeEventMask {
  const RuntimeEventMask.fromRawValue(this.rawValue);
  final int rawValue;
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
  RuntimeEventMask operator |(RuntimeEventMask other) =>
      RuntimeEventMask.fromRawValue(rawValue | other.rawValue);
  RuntimeEventMask operator &(RuntimeEventMask other) =>
      RuntimeEventMask.fromRawValue(rawValue & other.rawValue);
  bool contains(RuntimeEventMask other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is RuntimeEventMask && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RuntimeEventPayloadType {
  const RuntimeEventPayloadType.fromRawValue(this.rawValue);
  final int rawValue;
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
  @override
  bool operator ==(Object other) =>
      other is RuntimeEventPayloadType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RuntimeEventSourceType {
  const RuntimeEventSourceType.fromRawValue(this.rawValue);
  final int rawValue;
  static const runtime = RuntimeEventSourceType.fromRawValue(0);
  static const map = RuntimeEventSourceType.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is RuntimeEventSourceType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class RuntimeEventType {
  const RuntimeEventType.fromRawValue(this.rawValue);
  final int rawValue;
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
  @override
  bool operator ==(Object other) =>
      other is RuntimeEventType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class SourceFeatureQueryOptionField {
  const SourceFeatureQueryOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const ids = SourceFeatureQueryOptionField.fromRawValue(1);
  SourceFeatureQueryOptionField operator |(
    SourceFeatureQueryOptionField other,
  ) => SourceFeatureQueryOptionField.fromRawValue(rawValue | other.rawValue);
  SourceFeatureQueryOptionField operator &(
    SourceFeatureQueryOptionField other,
  ) => SourceFeatureQueryOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(SourceFeatureQueryOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is SourceFeatureQueryOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class Status {
  const Status.fromRawValue(this.rawValue);
  final int rawValue;
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
  @override
  bool operator ==(Object other) =>
      other is Status && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleImageOptionField {
  const StyleImageOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const pixelRatio = StyleImageOptionField.fromRawValue(1);
  static const sdf = StyleImageOptionField.fromRawValue(2);
  static const stretchX = StyleImageOptionField.fromRawValue(4);
  static const stretchY = StyleImageOptionField.fromRawValue(8);
  static const content = StyleImageOptionField.fromRawValue(16);
  static const textFitWidth = StyleImageOptionField.fromRawValue(32);
  static const textFitHeight = StyleImageOptionField.fromRawValue(64);
  StyleImageOptionField operator |(StyleImageOptionField other) =>
      StyleImageOptionField.fromRawValue(rawValue | other.rawValue);
  StyleImageOptionField operator &(StyleImageOptionField other) =>
      StyleImageOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(StyleImageOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is StyleImageOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleImageTextFit {
  const StyleImageTextFit.fromRawValue(this.rawValue);
  final int rawValue;
  static const stretchOrShrink = StyleImageTextFit.fromRawValue(0);
  static const stretchOnly = StyleImageTextFit.fromRawValue(1);
  static const proportional = StyleImageTextFit.fromRawValue(2);
  @override
  bool operator ==(Object other) =>
      other is StyleImageTextFit && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleLayerVisibility {
  const StyleLayerVisibility.fromRawValue(this.rawValue);
  final int rawValue;
  static const visible = StyleLayerVisibility.fromRawValue(0);
  static const none = StyleLayerVisibility.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is StyleLayerVisibility && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleRasterDemEncoding {
  const StyleRasterDemEncoding.fromRawValue(this.rawValue);
  final int rawValue;
  static const mapbox = StyleRasterDemEncoding.fromRawValue(0);
  static const terrarium = StyleRasterDemEncoding.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is StyleRasterDemEncoding && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleSourceInfoField {
  const StyleSourceInfoField.fromRawValue(this.rawValue);
  final int rawValue;
  static const url = StyleSourceInfoField.fromRawValue(1);
  static const tilejson = StyleSourceInfoField.fromRawValue(2);
  static const bounds = StyleSourceInfoField.fromRawValue(4);
  static const tileSize = StyleSourceInfoField.fromRawValue(8);
  static const vectorEncoding = StyleSourceInfoField.fromRawValue(16);
  static const rasterEncoding = StyleSourceInfoField.fromRawValue(32);
  StyleSourceInfoField operator |(StyleSourceInfoField other) =>
      StyleSourceInfoField.fromRawValue(rawValue | other.rawValue);
  StyleSourceInfoField operator &(StyleSourceInfoField other) =>
      StyleSourceInfoField.fromRawValue(rawValue & other.rawValue);
  bool contains(StyleSourceInfoField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is StyleSourceInfoField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleSourceType {
  const StyleSourceType.fromRawValue(this.rawValue);
  final int rawValue;
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
  @override
  bool operator ==(Object other) =>
      other is StyleSourceType && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleTileScheme {
  const StyleTileScheme.fromRawValue(this.rawValue);
  final int rawValue;
  static const xyz = StyleTileScheme.fromRawValue(0);
  static const tms = StyleTileScheme.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is StyleTileScheme && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleTileSourceOptionField {
  const StyleTileSourceOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const minZoom = StyleTileSourceOptionField.fromRawValue(1);
  static const maxZoom = StyleTileSourceOptionField.fromRawValue(2);
  static const attribution = StyleTileSourceOptionField.fromRawValue(4);
  static const scheme = StyleTileSourceOptionField.fromRawValue(8);
  static const bounds = StyleTileSourceOptionField.fromRawValue(16);
  static const tileSize = StyleTileSourceOptionField.fromRawValue(32);
  static const vectorEncoding = StyleTileSourceOptionField.fromRawValue(64);
  static const rasterEncoding = StyleTileSourceOptionField.fromRawValue(128);
  StyleTileSourceOptionField operator |(StyleTileSourceOptionField other) =>
      StyleTileSourceOptionField.fromRawValue(rawValue | other.rawValue);
  StyleTileSourceOptionField operator &(StyleTileSourceOptionField other) =>
      StyleTileSourceOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(StyleTileSourceOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is StyleTileSourceOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleTransitionOptionField {
  const StyleTransitionOptionField.fromRawValue(this.rawValue);
  final int rawValue;
  static const duration = StyleTransitionOptionField.fromRawValue(1);
  static const delay = StyleTransitionOptionField.fromRawValue(2);
  static const enablePlacementTransitions =
      StyleTransitionOptionField.fromRawValue(4);
  StyleTransitionOptionField operator |(StyleTransitionOptionField other) =>
      StyleTransitionOptionField.fromRawValue(rawValue | other.rawValue);
  StyleTransitionOptionField operator &(StyleTransitionOptionField other) =>
      StyleTransitionOptionField.fromRawValue(rawValue & other.rawValue);
  bool contains(StyleTransitionOptionField other) =>
      (rawValue & other.rawValue) == other.rawValue;
  @override
  bool operator ==(Object other) =>
      other is StyleTransitionOptionField && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class StyleVectorTileEncoding {
  const StyleVectorTileEncoding.fromRawValue(this.rawValue);
  final int rawValue;
  static const mvt = StyleVectorTileEncoding.fromRawValue(0);
  static const mlt = StyleVectorTileEncoding.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is StyleVectorTileEncoding && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class TileLodMode {
  const TileLodMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const defaultValue = TileLodMode.fromRawValue(0);
  static const distance = TileLodMode.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is TileLodMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class TileOperation {
  const TileOperation.fromRawValue(this.rawValue);
  final int rawValue;
  static const requestedFromCache = TileOperation.fromRawValue(0);
  static const requestedFromNetwork = TileOperation.fromRawValue(1);
  static const loadFromNetwork = TileOperation.fromRawValue(2);
  static const loadFromCache = TileOperation.fromRawValue(3);
  static const startParse = TileOperation.fromRawValue(4);
  static const endParse = TileOperation.fromRawValue(5);
  static const error = TileOperation.fromRawValue(6);
  static const cancelled = TileOperation.fromRawValue(7);
  static const nullValue = TileOperation.fromRawValue(8);
  @override
  bool operator ==(Object other) =>
      other is TileOperation && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ViewportMode {
  const ViewportMode.fromRawValue(this.rawValue);
  final int rawValue;
  static const defaultValue = ViewportMode.fromRawValue(0);
  static const flippedY = ViewportMode.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is ViewportMode && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class WebglContextKind {
  const WebglContextKind.fromRawValue(this.rawValue);
  final int rawValue;
  static const existing = WebglContextKind.fromRawValue(0);
  static const transferredCanvas = WebglContextKind.fromRawValue(1);
  @override
  bool operator ==(Object other) =>
      other is WebglContextKind && other.rawValue == rawValue;
  @override
  int get hashCode => rawValue.hashCode;
}

final class ResourceRequest {
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
  bool operator ==(Object other) =>
      other is ResourceRequest &&
      _generatedValueEquals(other.requestedUrl, requestedUrl) &&
      _generatedValueEquals(other.resolvedUrl, resolvedUrl) &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.loadingMethod, loadingMethod) &&
      _generatedValueEquals(other.priority, priority) &&
      _generatedValueEquals(other.usage, usage) &&
      _generatedValueEquals(other.storagePolicy, storagePolicy) &&
      _generatedValueEquals(other.range, range) &&
      _generatedValueEquals(other.priorModifiedUnixMs, priorModifiedUnixMs) &&
      _generatedValueEquals(other.priorExpiresUnixMs, priorExpiresUnixMs) &&
      _generatedValueEquals(other.priorEtag, priorEtag) &&
      _generatedValueEquals(other.priorData, priorData);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(requestedUrl),
    _generatedValueHash(resolvedUrl),
    _generatedValueHash(kind),
    _generatedValueHash(loadingMethod),
    _generatedValueHash(priority),
    _generatedValueHash(usage),
    _generatedValueHash(storagePolicy),
    _generatedValueHash(range),
    _generatedValueHash(priorModifiedUnixMs),
    _generatedValueHash(priorExpiresUnixMs),
    _generatedValueHash(priorEtag),
    _generatedValueHash(priorData),
  ]);
}

final class MetalOwnedTextureFrame {
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
  bool operator ==(Object other) =>
      other is MetalOwnedTextureFrame &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor) &&
      _generatedValueEquals(other.frameId, frameId) &&
      _generatedValueEquals(other.texture, texture) &&
      _generatedValueEquals(other.device, device) &&
      _generatedValueEquals(other.pixelFormat, pixelFormat);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(generation),
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
    _generatedValueHash(frameId),
    _generatedValueHash(texture),
    _generatedValueHash(device),
    _generatedValueHash(pixelFormat),
  ]);
}

final class OpenglOwnedTextureFrame {
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
  bool operator ==(Object other) =>
      other is OpenglOwnedTextureFrame &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor) &&
      _generatedValueEquals(other.frameId, frameId) &&
      _generatedValueEquals(other.texture, texture) &&
      _generatedValueEquals(other.target, target) &&
      _generatedValueEquals(other.internalFormat, internalFormat) &&
      _generatedValueEquals(other.format, format) &&
      _generatedValueEquals(other.type, type);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(generation),
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
    _generatedValueHash(frameId),
    _generatedValueHash(texture),
    _generatedValueHash(target),
    _generatedValueHash(internalFormat),
    _generatedValueHash(format),
    _generatedValueHash(type),
  ]);
}

final class GpuSync {
  const GpuSync({
    this.kind = const GpuSyncKind.fromRawValue(0),
    required this.object,
    required this.value,
  });
  final GpuSyncKind kind;
  final BigInt object;
  final BigInt value;

  @override
  bool operator ==(Object other) =>
      other is GpuSync &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.object, object) &&
      _generatedValueEquals(other.value, value);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(object),
    _generatedValueHash(value),
  ]);
}

final class RenderFrameResult {
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
  bool operator ==(Object other) =>
      other is RenderFrameResult &&
      _generatedValueEquals(other.disposition, disposition) &&
      _generatedValueEquals(other.token, token) &&
      _generatedValueEquals(other.mapUpdateGeneration, mapUpdateGeneration) &&
      _generatedValueEquals(other.extentGeneration, extentGeneration) &&
      _generatedValueEquals(other.frameGeneration, frameGeneration) &&
      _generatedValueEquals(other.needsRepaint, needsRepaint);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(disposition),
    _generatedValueHash(token),
    _generatedValueHash(mapUpdateGeneration),
    _generatedValueHash(extentGeneration),
    _generatedValueHash(frameGeneration),
    _generatedValueHash(needsRepaint),
  ]);
}

final class VulkanOwnedTextureFrame {
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
  bool operator ==(Object other) =>
      other is VulkanOwnedTextureFrame &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor) &&
      _generatedValueEquals(other.frameId, frameId) &&
      _generatedValueEquals(other.image, image) &&
      _generatedValueEquals(other.imageView, imageView) &&
      _generatedValueEquals(other.device, device) &&
      _generatedValueEquals(other.format, format) &&
      _generatedValueEquals(other.layout, layout);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(generation),
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
    _generatedValueHash(frameId),
    _generatedValueHash(image),
    _generatedValueHash(imageView),
    _generatedValueHash(device),
    _generatedValueHash(format),
    _generatedValueHash(layout),
  ]);
}

final class WebgpuOwnedTextureFrame {
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
  bool operator ==(Object other) =>
      other is WebgpuOwnedTextureFrame &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor) &&
      _generatedValueEquals(other.frameId, frameId) &&
      _generatedValueEquals(other.texture, texture) &&
      _generatedValueEquals(other.textureView, textureView) &&
      _generatedValueEquals(other.device, device) &&
      _generatedValueEquals(other.format, format);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(generation),
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
    _generatedValueHash(frameId),
    _generatedValueHash(texture),
    _generatedValueHash(textureView),
    _generatedValueHash(device),
    _generatedValueHash(format),
  ]);
}

final class UnitBezier {
  const UnitBezier(this.x1, this.y1, this.x2, this.y2);
  final double x1;
  final double y1;
  final double x2;
  final double y2;

  @override
  bool operator ==(Object other) =>
      other is UnitBezier &&
      _generatedValueEquals(other.x1, x1) &&
      _generatedValueEquals(other.y1, y1) &&
      _generatedValueEquals(other.x2, x2) &&
      _generatedValueEquals(other.y2, y2);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(x1),
    _generatedValueHash(y1),
    _generatedValueHash(x2),
    _generatedValueHash(y2),
  ]);
}

final class AnimationOptions {
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
  bool operator ==(Object other) =>
      other is AnimationOptions &&
      _generatedValueEquals(other.durationMs, durationMs) &&
      _generatedValueEquals(other.velocity, velocity) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.easing, easing) &&
      _generatedValueEquals(other.transitionId, transitionId);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(durationMs),
    _generatedValueHash(velocity),
    _generatedValueHash(minZoom),
    _generatedValueHash(easing),
    _generatedValueHash(transitionId),
  ]);
}

final class LatLng {
  const LatLng(this.latitude, this.longitude);
  final double latitude;
  final double longitude;

  @override
  bool operator ==(Object other) =>
      other is LatLng &&
      _generatedValueEquals(other.latitude, latitude) &&
      _generatedValueEquals(other.longitude, longitude);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(latitude),
    _generatedValueHash(longitude),
  ]);
}

final class LatLngBounds {
  const LatLngBounds({
    this.southwest = const LatLng(0, 0),
    this.northeast = const LatLng(0, 0),
  });
  final LatLng southwest;
  final LatLng northeast;

  @override
  bool operator ==(Object other) =>
      other is LatLngBounds &&
      _generatedValueEquals(other.southwest, southwest) &&
      _generatedValueEquals(other.northeast, northeast);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(southwest),
    _generatedValueHash(northeast),
  ]);
}

final class BoundOptions {
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
  bool operator ==(Object other) =>
      other is BoundOptions &&
      _generatedValueEquals(other.bounds, bounds) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.minPitch, minPitch) &&
      _generatedValueEquals(other.maxPitch, maxPitch) &&
      other.unbounded == unbounded;
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(bounds),
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(minPitch),
    _generatedValueHash(maxPitch),
    unbounded,
  ]);
}

final class ScreenPoint {
  const ScreenPoint(this.x, this.y);
  final double x;
  final double y;

  @override
  bool operator ==(Object other) =>
      other is ScreenPoint &&
      _generatedValueEquals(other.x, x) &&
      _generatedValueEquals(other.y, y);
  @override
  int get hashCode =>
      Object.hashAll([_generatedValueHash(x), _generatedValueHash(y)]);
}

final class CameraDelta {
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
  bool operator ==(Object other) =>
      other is CameraDelta &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.offset, offset) &&
      _generatedValueEquals(other.amount, amount) &&
      _generatedValueEquals(other.anchor, anchor) &&
      _generatedValueEquals(other.animation, animation);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(offset),
    _generatedValueHash(amount),
    _generatedValueHash(anchor),
    _generatedValueHash(animation),
  ]);
}

final class EdgeInsets {
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
  bool operator ==(Object other) =>
      other is EdgeInsets &&
      _generatedValueEquals(other.top, top) &&
      _generatedValueEquals(other.left, left) &&
      _generatedValueEquals(other.bottom, bottom) &&
      _generatedValueEquals(other.right, right);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(top),
    _generatedValueHash(left),
    _generatedValueHash(bottom),
    _generatedValueHash(right),
  ]);
}

final class CameraFitOptions {
  const CameraFitOptions({this.padding, this.bearing, this.pitch});
  final EdgeInsets? padding;
  final double? bearing;
  final double? pitch;

  @override
  bool operator ==(Object other) =>
      other is CameraFitOptions &&
      _generatedValueEquals(other.padding, padding) &&
      _generatedValueEquals(other.bearing, bearing) &&
      _generatedValueEquals(other.pitch, pitch);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(padding),
    _generatedValueHash(bearing),
    _generatedValueHash(pitch),
  ]);
}

final class CameraOptions {
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
  bool operator ==(Object other) =>
      other is CameraOptions &&
      _generatedValueEquals(other.center, center) &&
      _generatedValueEquals(other.centerAltitude, centerAltitude) &&
      _generatedValueEquals(other.padding, padding) &&
      _generatedValueEquals(other.anchor, anchor) &&
      _generatedValueEquals(other.zoom, zoom) &&
      _generatedValueEquals(other.bearing, bearing) &&
      _generatedValueEquals(other.pitch, pitch) &&
      _generatedValueEquals(other.roll, roll) &&
      _generatedValueEquals(other.fieldOfView, fieldOfView);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(center),
    _generatedValueHash(centerAltitude),
    _generatedValueHash(padding),
    _generatedValueHash(anchor),
    _generatedValueHash(zoom),
    _generatedValueHash(bearing),
    _generatedValueHash(pitch),
    _generatedValueHash(roll),
    _generatedValueHash(fieldOfView),
  ]);
}

final class CameraUpdate {
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
  bool operator ==(Object other) =>
      other is CameraUpdate &&
      _generatedValueEquals(other.mode, mode) &&
      _generatedValueEquals(other.camera, camera) &&
      _generatedValueEquals(other.animation, animation) &&
      _generatedValueEquals(other.gesturePhase, gesturePhase);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(mode),
    _generatedValueHash(camera),
    _generatedValueHash(animation),
    _generatedValueHash(gesturePhase),
  ]);
}

final class CanonicalTileId {
  const CanonicalTileId({this.z = 0, this.x = 0, this.y = 0});
  final int z;
  final int x;
  final int y;

  @override
  bool operator ==(Object other) =>
      other is CanonicalTileId &&
      _generatedValueEquals(other.z, z) &&
      _generatedValueEquals(other.x, x) &&
      _generatedValueEquals(other.y, y);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(z),
    _generatedValueHash(x),
    _generatedValueHash(y),
  ]);
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

final class RenderingStats {
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
  bool operator ==(Object other) =>
      other is RenderingStats &&
      _generatedValueEquals(other.encodingTime, encodingTime) &&
      _generatedValueEquals(other.renderingTime, renderingTime) &&
      _generatedValueEquals(other.frameCount, frameCount) &&
      _generatedValueEquals(other.drawCallCount, drawCallCount) &&
      _generatedValueEquals(other.totalDrawCallCount, totalDrawCallCount);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(encodingTime),
    _generatedValueHash(renderingTime),
    _generatedValueHash(frameCount),
    _generatedValueHash(drawCallCount),
    _generatedValueHash(totalDrawCallCount),
  ]);
}

final class RuntimeEventRenderFrame {
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
  bool operator ==(Object other) =>
      other is RuntimeEventRenderFrame &&
      _generatedValueEquals(other.mode, mode) &&
      _generatedValueEquals(other.needsRepaint, needsRepaint) &&
      _generatedValueEquals(other.placementChanged, placementChanged) &&
      _generatedValueEquals(other.stats, stats);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(mode),
    _generatedValueHash(needsRepaint),
    _generatedValueHash(placementChanged),
    _generatedValueHash(stats),
  ]);
}

final class RuntimeEventRenderMap {
  const RuntimeEventRenderMap({this.mode = const RenderMode.fromRawValue(0)});
  final RenderMode mode;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventRenderMap && _generatedValueEquals(other.mode, mode);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(mode)]);
}

final class TileId {
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
  bool operator ==(Object other) =>
      other is TileId &&
      _generatedValueEquals(other.overscaledZ, overscaledZ) &&
      _generatedValueEquals(other.wrap, wrap) &&
      _generatedValueEquals(other.canonicalZ, canonicalZ) &&
      _generatedValueEquals(other.canonicalX, canonicalX) &&
      _generatedValueEquals(other.canonicalY, canonicalY);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(overscaledZ),
    _generatedValueHash(wrap),
    _generatedValueHash(canonicalZ),
    _generatedValueHash(canonicalX),
    _generatedValueHash(canonicalY),
  ]);
}

final class RuntimeEventTileAction {
  const RuntimeEventTileAction({
    this.operation = const TileOperation.fromRawValue(0),
    this.tileId = const TileId(),
  });
  final TileOperation operation;
  final TileId tileId;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventTileAction &&
      _generatedValueEquals(other.operation, operation) &&
      _generatedValueEquals(other.tileId, tileId);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(operation),
    _generatedValueHash(tileId),
  ]);
}

final class OfflineRegionStatus {
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
  bool operator ==(Object other) =>
      other is OfflineRegionStatus &&
      _generatedValueEquals(other.downloadState, downloadState) &&
      _generatedValueEquals(
        other.completedResourceCount,
        completedResourceCount,
      ) &&
      _generatedValueEquals(
        other.completedResourceSize,
        completedResourceSize,
      ) &&
      _generatedValueEquals(other.completedTileCount, completedTileCount) &&
      _generatedValueEquals(other.requiredTileCount, requiredTileCount) &&
      _generatedValueEquals(other.completedTileSize, completedTileSize) &&
      _generatedValueEquals(
        other.requiredResourceCount,
        requiredResourceCount,
      ) &&
      _generatedValueEquals(
        other.requiredResourceCountIsPrecise,
        requiredResourceCountIsPrecise,
      ) &&
      _generatedValueEquals(other.complete, complete);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(downloadState),
    _generatedValueHash(completedResourceCount),
    _generatedValueHash(completedResourceSize),
    _generatedValueHash(completedTileCount),
    _generatedValueHash(requiredTileCount),
    _generatedValueHash(completedTileSize),
    _generatedValueHash(requiredResourceCount),
    _generatedValueHash(requiredResourceCountIsPrecise),
    _generatedValueHash(complete),
  ]);
}

final class RuntimeEventOfflineRegionStatus {
  const RuntimeEventOfflineRegionStatus({
    this.regionId = 0,
    required this.status,
  });
  final int regionId;
  final OfflineRegionStatus status;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventOfflineRegionStatus &&
      _generatedValueEquals(other.regionId, regionId) &&
      _generatedValueEquals(other.status, status);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(regionId),
    _generatedValueHash(status),
  ]);
}

final class RuntimeEventOfflineRegionResponseError {
  const RuntimeEventOfflineRegionResponseError({
    this.regionId = 0,
    this.reason = const ResourceErrorReason.fromRawValue(0),
  });
  final int regionId;
  final ResourceErrorReason reason;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventOfflineRegionResponseError &&
      _generatedValueEquals(other.regionId, regionId) &&
      _generatedValueEquals(other.reason, reason);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(regionId),
    _generatedValueHash(reason),
  ]);
}

final class RuntimeEventOfflineRegionTileCountLimit {
  const RuntimeEventOfflineRegionTileCountLimit({
    this.regionId = 0,
    required this.limit,
  });
  final int regionId;
  final BigInt limit;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventOfflineRegionTileCountLimit &&
      _generatedValueEquals(other.regionId, regionId) &&
      _generatedValueEquals(other.limit, limit);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(regionId),
    _generatedValueHash(limit),
  ]);
}

final class RuntimeEventCameraTransitionFinished {
  const RuntimeEventCameraTransitionFinished({required this.transitionId});
  final BigInt transitionId;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventCameraTransitionFinished &&
      _generatedValueEquals(other.transitionId, transitionId);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(transitionId)]);
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

final class RuntimeEventBatchView {
  RuntimeEventBatchView({required List<RuntimeEvent> events})
    : events = List.unmodifiable(events);
  final List<RuntimeEvent> events;

  @override
  bool operator ==(Object other) =>
      other is RuntimeEventBatchView &&
      _generatedValueEquals(other.events, events);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(events)]);
}

final class FrameDemand {
  const FrameDemand({
    this.flags = const FrameDemandFlag.fromRawValue(0),
    required this.token,
    required this.coalescingBoundary,
    required this.timeoutNs,
  });
  final FrameDemandFlag flags;
  final BigInt token;
  final BigInt coalescingBoundary;
  final BigInt timeoutNs;

  @override
  bool operator ==(Object other) =>
      other is FrameDemand &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.token, token) &&
      _generatedValueEquals(other.coalescingBoundary, coalescingBoundary) &&
      _generatedValueEquals(other.timeoutNs, timeoutNs);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(flags),
    _generatedValueHash(token),
    _generatedValueHash(coalescingBoundary),
    _generatedValueHash(timeoutNs),
  ]);
}

final class Vec3 {
  const Vec3(this.x, this.y, this.z);
  final double x;
  final double y;
  final double z;

  @override
  bool operator ==(Object other) =>
      other is Vec3 &&
      _generatedValueEquals(other.x, x) &&
      _generatedValueEquals(other.y, y) &&
      _generatedValueEquals(other.z, z);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(x),
    _generatedValueHash(y),
    _generatedValueHash(z),
  ]);
}

final class Quaternion {
  const Quaternion(this.x, this.y, this.z, this.w);
  final double x;
  final double y;
  final double z;
  final double w;

  @override
  bool operator ==(Object other) =>
      other is Quaternion &&
      _generatedValueEquals(other.x, x) &&
      _generatedValueEquals(other.y, y) &&
      _generatedValueEquals(other.z, z) &&
      _generatedValueEquals(other.w, w);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(x),
    _generatedValueHash(y),
    _generatedValueHash(z),
    _generatedValueHash(w),
  ]);
}

final class FreeCameraOptions {
  const FreeCameraOptions({this.position, this.orientation});
  final Vec3? position;
  final Quaternion? orientation;

  @override
  bool operator ==(Object other) =>
      other is FreeCameraOptions &&
      _generatedValueEquals(other.position, position) &&
      _generatedValueEquals(other.orientation, orientation);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(position),
    _generatedValueHash(orientation),
  ]);
}

final class GeojsonSourceOptions {
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
  bool operator ==(Object other) =>
      other is GeojsonSourceOptions &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.tolerance, tolerance) &&
      _generatedValueEquals(other.clusterMaxZoom, clusterMaxZoom) &&
      _generatedValueEquals(other.clusterProperties, clusterProperties) &&
      _generatedValueEquals(other.tileSize, tileSize) &&
      _generatedValueEquals(other.buffer, buffer) &&
      _generatedValueEquals(other.clusterRadius, clusterRadius) &&
      _generatedValueEquals(other.clusterMinPoints, clusterMinPoints) &&
      _generatedValueEquals(other.lineMetrics, lineMetrics) &&
      _generatedValueEquals(other.cluster, cluster) &&
      _generatedValueEquals(other.synchronousTiling, synchronousTiling);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(tolerance),
    _generatedValueHash(clusterMaxZoom),
    _generatedValueHash(clusterProperties),
    _generatedValueHash(tileSize),
    _generatedValueHash(buffer),
    _generatedValueHash(clusterRadius),
    _generatedValueHash(clusterMinPoints),
    _generatedValueHash(lineMetrics),
    _generatedValueHash(cluster),
    _generatedValueHash(synchronousTiling),
  ]);
}

final class ProjectedMeters {
  const ProjectedMeters({this.northing = 0, this.easting = 0});
  final double northing;
  final double easting;

  @override
  bool operator ==(Object other) =>
      other is ProjectedMeters &&
      _generatedValueEquals(other.northing, northing) &&
      _generatedValueEquals(other.easting, easting);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(northing),
    _generatedValueHash(easting),
  ]);
}

final class PremultipliedRgba8Image {
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
  bool operator ==(Object other) =>
      other is PremultipliedRgba8Image &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.stride, stride) &&
      _generatedValueEquals(other.pixels, pixels);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(stride),
    _generatedValueHash(pixels),
  ]);
}

final class StyleTileSourceOptions {
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
  bool operator ==(Object other) =>
      other is StyleTileSourceOptions &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.attribution, attribution) &&
      _generatedValueEquals(other.scheme, scheme) &&
      _generatedValueEquals(other.bounds, bounds) &&
      _generatedValueEquals(other.tileSize, tileSize) &&
      _generatedValueEquals(other.vectorEncoding, vectorEncoding) &&
      _generatedValueEquals(other.rasterEncoding, rasterEncoding);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(attribution),
    _generatedValueHash(scheme),
    _generatedValueHash(bounds),
    _generatedValueHash(tileSize),
    _generatedValueHash(vectorEncoding),
    _generatedValueHash(rasterEncoding),
  ]);
}

final class CameraQueryResult {
  const CameraQueryResult({
    required this.generation,
    this.camera = const CameraOptions(),
  });
  final BigInt generation;
  final CameraOptions camera;

  @override
  bool operator ==(Object other) =>
      other is CameraQueryResult &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.camera, camera);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(generation),
    _generatedValueHash(camera),
  ]);
}

final class ImageStretch {
  const ImageStretch(this.from, this.to);
  final double from;
  final double to;

  @override
  bool operator ==(Object other) =>
      other is ImageStretch &&
      _generatedValueEquals(other.from, from) &&
      _generatedValueEquals(other.to, to);
  @override
  int get hashCode =>
      Object.hashAll([_generatedValueHash(from), _generatedValueHash(to)]);
}

final class StyleImageStretchesResult {
  StyleImageStretchesResult({
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
  }) : stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;

  @override
  bool operator ==(Object other) =>
      other is StyleImageStretchesResult &&
      _generatedValueEquals(other.stretchX, stretchX) &&
      _generatedValueEquals(other.stretchY, stretchY);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(stretchX),
    _generatedValueHash(stretchY),
  ]);
}

final class LogicalExtent {
  const LogicalExtent({this.width = 0, this.height = 0, this.scaleFactor = 0});
  final int width;
  final int height;
  final double scaleFactor;

  @override
  bool operator ==(Object other) =>
      other is LogicalExtent &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
  ]);
}

final class MapOptions {
  const MapOptions({
    this.initialExtent = const LogicalExtent(),
    this.mapMode = const MapMode.fromRawValue(0),
    this.fastPforEnabled = false,
    this.eventMask = const RuntimeEventMask.fromRawValue(0),
  });
  final LogicalExtent initialExtent;
  final MapMode mapMode;
  final bool fastPforEnabled;
  final RuntimeEventMask eventMask;

  @override
  bool operator ==(Object other) =>
      other is MapOptions &&
      _generatedValueEquals(other.initialExtent, initialExtent) &&
      _generatedValueEquals(other.mapMode, mapMode) &&
      _generatedValueEquals(other.fastPforEnabled, fastPforEnabled) &&
      _generatedValueEquals(other.eventMask, eventMask);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(initialExtent),
    _generatedValueHash(mapMode),
    _generatedValueHash(fastPforEnabled),
    _generatedValueHash(eventMask),
  ]);
}

final class FeatureStateSelector {
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
  bool operator ==(Object other) =>
      other is FeatureStateSelector &&
      _generatedValueEquals(other.sourceId, sourceId) &&
      _generatedValueEquals(other.sourceLayerId, sourceLayerId) &&
      _generatedValueEquals(other.featureId, featureId) &&
      _generatedValueEquals(other.stateKey, stateKey);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(sourceId),
    _generatedValueHash(sourceLayerId),
    _generatedValueHash(featureId),
    _generatedValueHash(stateKey),
  ]);
}

final class ImageContent {
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
  bool operator ==(Object other) =>
      other is ImageContent &&
      _generatedValueEquals(other.left, left) &&
      _generatedValueEquals(other.top, top) &&
      _generatedValueEquals(other.right, right) &&
      _generatedValueEquals(other.bottom, bottom);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(left),
    _generatedValueHash(top),
    _generatedValueHash(right),
    _generatedValueHash(bottom),
  ]);
}

final class StyleImageInfo {
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
    this.pixelRatio = 0,
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
  bool operator ==(Object other) =>
      other is StyleImageInfo &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.stride, stride) &&
      _generatedValueEquals(other.byteLength, byteLength) &&
      _generatedValueEquals(other.stretchXCount, stretchXCount) &&
      _generatedValueEquals(other.stretchYCount, stretchYCount) &&
      _generatedValueEquals(other.content, content) &&
      _generatedValueEquals(other.textFitWidth, textFitWidth) &&
      _generatedValueEquals(other.textFitHeight, textFitHeight) &&
      _generatedValueEquals(other.pixelRatio, pixelRatio) &&
      _generatedValueEquals(other.sdf, sdf);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(stride),
    _generatedValueHash(byteLength),
    _generatedValueHash(stretchXCount),
    _generatedValueHash(stretchYCount),
    _generatedValueHash(content),
    _generatedValueHash(textFitWidth),
    _generatedValueHash(textFitHeight),
    _generatedValueHash(pixelRatio),
    _generatedValueHash(sdf),
  ]);
}

final class StyleImageResult {
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
  bool operator ==(Object other) =>
      other is StyleImageResult &&
      _generatedValueEquals(other.info, info) &&
      _generatedValueEquals(other.pixels, pixels) &&
      _generatedValueEquals(other.stretchX, stretchX) &&
      _generatedValueEquals(other.stretchY, stretchY);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(info),
    _generatedValueHash(pixels),
    _generatedValueHash(stretchX),
    _generatedValueHash(stretchY),
  ]);
}

final class StyleLayerInfo {
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
  bool operator ==(Object other) =>
      other is StyleLayerInfo &&
      _generatedValueEquals(other.type, type) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.visibility, visibility);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(type),
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(visibility),
  ]);
}

final class StyleLayerResult {
  const StyleLayerResult({required this.info, this.sourceId, this.sourceLayer});
  final StyleLayerInfo info;
  final String? sourceId;
  final String? sourceLayer;

  @override
  bool operator ==(Object other) =>
      other is StyleLayerResult &&
      _generatedValueEquals(other.info, info) &&
      _generatedValueEquals(other.sourceId, sourceId) &&
      _generatedValueEquals(other.sourceLayer, sourceLayer);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(info),
    _generatedValueHash(sourceId),
    _generatedValueHash(sourceLayer),
  ]);
}

final class StyleSourceTileInfo {
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
  bool operator ==(Object other) =>
      other is StyleSourceTileInfo &&
      _generatedValueEquals(other.tileCount, tileCount) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.scheme, scheme);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(tileCount),
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(scheme),
  ]);
}

final class StyleSourceInfo {
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
  bool operator ==(Object other) =>
      other is StyleSourceInfo &&
      _generatedValueEquals(other.type, type) &&
      _generatedValueEquals(other.idSize, idSize) &&
      _generatedValueEquals(other.isVolatile, isVolatile) &&
      _generatedValueEquals(other.attributionSize, attributionSize) &&
      _generatedValueEquals(other.urlSize, urlSize) &&
      _generatedValueEquals(other.tilejson, tilejson) &&
      _generatedValueEquals(other.bounds, bounds) &&
      _generatedValueEquals(other.tileSize, tileSize) &&
      _generatedValueEquals(other.vectorEncoding, vectorEncoding) &&
      _generatedValueEquals(other.rasterEncoding, rasterEncoding);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(type),
    _generatedValueHash(idSize),
    _generatedValueHash(isVolatile),
    _generatedValueHash(attributionSize),
    _generatedValueHash(urlSize),
    _generatedValueHash(tilejson),
    _generatedValueHash(bounds),
    _generatedValueHash(tileSize),
    _generatedValueHash(vectorEncoding),
    _generatedValueHash(rasterEncoding),
  ]);
}

final class StyleSourceResult {
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
  bool operator ==(Object other) =>
      other is StyleSourceResult &&
      _generatedValueEquals(other.info, info) &&
      _generatedValueEquals(other.attribution, attribution) &&
      _generatedValueEquals(other.url, url) &&
      _generatedValueEquals(other.tileUrls, tileUrls);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(info),
    _generatedValueHash(attribution),
    _generatedValueHash(url),
    _generatedValueHash(tileUrls),
  ]);
}

final class StyleSourceTileUrlsResult {
  StyleSourceTileUrlsResult({required List<String> tileUrls})
    : tileUrls = List.unmodifiable(tileUrls);
  final List<String> tileUrls;

  @override
  bool operator ==(Object other) =>
      other is StyleSourceTileUrlsResult &&
      _generatedValueEquals(other.tileUrls, tileUrls);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(tileUrls)]);
}

final class StyleTransitionOptions {
  const StyleTransitionOptions({
    this.durationMs,
    this.delayMs,
    this.enablePlacementTransitions,
  });
  final double? durationMs;
  final double? delayMs;
  final bool? enablePlacementTransitions;

  @override
  bool operator ==(Object other) =>
      other is StyleTransitionOptions &&
      _generatedValueEquals(other.durationMs, durationMs) &&
      _generatedValueEquals(other.delayMs, delayMs) &&
      _generatedValueEquals(
        other.enablePlacementTransitions,
        enablePlacementTransitions,
      );
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(durationMs),
    _generatedValueHash(delayMs),
    _generatedValueHash(enablePlacementTransitions),
  ]);
}

final class StyleLayerEntry {
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
  bool operator ==(Object other) =>
      other is StyleLayerEntry &&
      _generatedValueEquals(other.id, id) &&
      _generatedValueEquals(other.type, type) &&
      _generatedValueEquals(other.sourceId, sourceId) &&
      _generatedValueEquals(other.sourceLayer, sourceLayer);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(id),
    _generatedValueHash(type),
    _generatedValueHash(sourceId),
    _generatedValueHash(sourceLayer),
  ]);
}

final class ProjectionMode {
  const ProjectionMode({this.axonometric, this.xSkew, this.ySkew});
  final bool? axonometric;
  final double? xSkew;
  final double? ySkew;

  @override
  bool operator ==(Object other) =>
      other is ProjectionMode &&
      _generatedValueEquals(other.axonometric, axonometric) &&
      _generatedValueEquals(other.xSkew, xSkew) &&
      _generatedValueEquals(other.ySkew, ySkew);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(axonometric),
    _generatedValueHash(xSkew),
    _generatedValueHash(ySkew),
  ]);
}

final class StyleImageOptions {
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
  bool operator ==(Object other) =>
      other is StyleImageOptions &&
      _generatedValueEquals(other.stretchX, stretchX) &&
      _generatedValueEquals(other.stretchY, stretchY) &&
      _generatedValueEquals(other.content, content) &&
      _generatedValueEquals(other.textFitWidth, textFitWidth) &&
      _generatedValueEquals(other.textFitHeight, textFitHeight) &&
      _generatedValueEquals(other.pixelRatio, pixelRatio) &&
      _generatedValueEquals(other.sdf, sdf);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(stretchX),
    _generatedValueHash(stretchY),
    _generatedValueHash(content),
    _generatedValueHash(textFitWidth),
    _generatedValueHash(textFitHeight),
    _generatedValueHash(pixelRatio),
    _generatedValueHash(sdf),
  ]);
}

final class MapTileOptions {
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
  bool operator ==(Object other) =>
      other is MapTileOptions &&
      _generatedValueEquals(other.prefetchZoomDelta, prefetchZoomDelta) &&
      _generatedValueEquals(other.lodMinRadius, lodMinRadius) &&
      _generatedValueEquals(other.lodScale, lodScale) &&
      _generatedValueEquals(other.lodPitchThreshold, lodPitchThreshold) &&
      _generatedValueEquals(other.lodZoomShift, lodZoomShift) &&
      _generatedValueEquals(other.lodMode, lodMode);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(prefetchZoomDelta),
    _generatedValueHash(lodMinRadius),
    _generatedValueHash(lodScale),
    _generatedValueHash(lodPitchThreshold),
    _generatedValueHash(lodZoomShift),
    _generatedValueHash(lodMode),
  ]);
}

final class MapViewportOptions {
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
  bool operator ==(Object other) =>
      other is MapViewportOptions &&
      _generatedValueEquals(other.northOrientation, northOrientation) &&
      _generatedValueEquals(other.constrainMode, constrainMode) &&
      _generatedValueEquals(other.viewportMode, viewportMode) &&
      _generatedValueEquals(other.frustumOffset, frustumOffset);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(northOrientation),
    _generatedValueHash(constrainMode),
    _generatedValueHash(viewportMode),
    _generatedValueHash(frustumOffset),
  ]);
}

final class MapSnapshot {
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
  bool operator ==(Object other) =>
      other is MapSnapshot &&
      _generatedValueEquals(other.debugOptions, debugOptions) &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.camera, camera) &&
      _generatedValueEquals(other.logicalExtent, logicalExtent) &&
      _generatedValueEquals(other.projectionMode, projectionMode) &&
      _generatedValueEquals(other.viewport, viewport) &&
      _generatedValueEquals(other.fullyLoaded, fullyLoaded) &&
      _generatedValueEquals(
        other.renderingStatsViewEnabled,
        renderingStatsViewEnabled,
      ) &&
      _generatedValueEquals(other.repaintDemand, repaintDemand) &&
      _generatedValueEquals(other.gestureInProgress, gestureInProgress) &&
      _generatedValueEquals(other.eventMask, eventMask) &&
      _generatedValueEquals(
        other.latestRenderUpdateGeneration,
        latestRenderUpdateGeneration,
      ) &&
      _generatedValueEquals(other.tile, tile) &&
      _generatedValueEquals(other.bounds, bounds) &&
      _generatedValueEquals(other.freeCamera, freeCamera);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(debugOptions),
    _generatedValueHash(generation),
    _generatedValueHash(camera),
    _generatedValueHash(logicalExtent),
    _generatedValueHash(projectionMode),
    _generatedValueHash(viewport),
    _generatedValueHash(fullyLoaded),
    _generatedValueHash(renderingStatsViewEnabled),
    _generatedValueHash(repaintDemand),
    _generatedValueHash(gestureInProgress),
    _generatedValueHash(eventMask),
    _generatedValueHash(latestRenderUpdateGeneration),
    _generatedValueHash(tile),
    _generatedValueHash(bounds),
    _generatedValueHash(freeCamera),
  ]);
}

final class RenderTargetExtent {
  const RenderTargetExtent({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  @override
  bool operator ==(Object other) =>
      other is RenderTargetExtent &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.scaleFactor, scaleFactor);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(scaleFactor),
  ]);
}

final class MetalBorrowedTextureDescriptor {
  const MetalBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 0,
    this.physicalHeight = 0,
    this.texture = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final int physicalWidth;
  final int physicalHeight;
  final NativePointer texture;

  @override
  bool operator ==(Object other) =>
      other is MetalBorrowedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.physicalWidth, physicalWidth) &&
      _generatedValueEquals(other.physicalHeight, physicalHeight) &&
      _generatedValueEquals(other.texture, texture);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(physicalWidth),
    _generatedValueHash(physicalHeight),
    _generatedValueHash(texture),
  ]);
}

typedef WakeCallback = void Function();

final class Wake {
  const Wake({this.callback});
  final WakeCallback? callback;
}

final class RenderSessionAttachOptions {
  const RenderSessionAttachOptions({
    required this.driver,
    this.requestedTextureRingDepth = 0,
    this.frameWake = const Wake(),
    this.driverWorkWake = const Wake(),
  });
  final RenderDriverKind driver;
  final int requestedTextureRingDepth;
  final Wake frameWake;
  final Wake driverWorkWake;

  @override
  bool operator ==(Object other) =>
      other is RenderSessionAttachOptions &&
      _generatedValueEquals(other.driver, driver) &&
      _generatedValueEquals(
        other.requestedTextureRingDepth,
        requestedTextureRingDepth,
      ) &&
      _generatedValueEquals(other.frameWake, frameWake) &&
      _generatedValueEquals(other.driverWorkWake, driverWorkWake);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(driver),
    _generatedValueHash(requestedTextureRingDepth),
    _generatedValueHash(frameWake),
    _generatedValueHash(driverWorkWake),
  ]);
}

final class MetalContextDescriptor {
  const MetalContextDescriptor({this.device = NativePointer.nullPointer});
  final NativePointer device;

  @override
  bool operator ==(Object other) =>
      other is MetalContextDescriptor &&
      _generatedValueEquals(other.device, device);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(device)]);
}

final class MetalOwnedTextureDescriptor {
  const MetalOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final MetalContextDescriptor context;

  @override
  bool operator ==(Object other) =>
      other is MetalOwnedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
  ]);
}

final class MetalSurfaceDescriptor {
  const MetalSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
    this.layer = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final MetalContextDescriptor context;
  final NativePointer layer;

  @override
  bool operator ==(Object other) =>
      other is MetalSurfaceDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.layer, layer);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
    _generatedValueHash(layer),
  ]);
}

final class WglContextDescriptor {
  const WglContextDescriptor({
    this.deviceContext = NativePointer.nullPointer,
    this.shareContext = NativePointer.nullPointer,
    this.getProcAddress = NativePointer.nullPointer,
  });
  final NativePointer deviceContext;
  final NativePointer shareContext;
  final NativePointer getProcAddress;

  @override
  bool operator ==(Object other) =>
      other is WglContextDescriptor &&
      _generatedValueEquals(other.deviceContext, deviceContext) &&
      _generatedValueEquals(other.shareContext, shareContext) &&
      _generatedValueEquals(other.getProcAddress, getProcAddress);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(deviceContext),
    _generatedValueHash(shareContext),
    _generatedValueHash(getProcAddress),
  ]);
}

final class EglContextDescriptor {
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
  bool operator ==(Object other) =>
      other is EglContextDescriptor &&
      _generatedValueEquals(other.display, display) &&
      _generatedValueEquals(other.config, config) &&
      _generatedValueEquals(other.shareContext, shareContext) &&
      _generatedValueEquals(other.clientApi, clientApi) &&
      _generatedValueEquals(other.getProcAddress, getProcAddress);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(display),
    _generatedValueHash(config),
    _generatedValueHash(shareContext),
    _generatedValueHash(clientApi),
    _generatedValueHash(getProcAddress),
  ]);
}

final class WebglContextDescriptor {
  const WebglContextDescriptor({
    this.kind = const WebglContextKind.fromRawValue(0),
    this.context = 0,
    required this.canvasSelector,
  });
  final WebglContextKind kind;
  final int context;
  final String canvasSelector;

  @override
  bool operator ==(Object other) =>
      other is WebglContextDescriptor &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.canvasSelector, canvasSelector);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(context),
    _generatedValueHash(canvasSelector),
  ]);
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

final class OpenglBorrowedTextureDescriptor {
  const OpenglBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 0,
    this.physicalHeight = 0,
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
  bool operator ==(Object other) =>
      other is OpenglBorrowedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.physicalWidth, physicalWidth) &&
      _generatedValueEquals(other.physicalHeight, physicalHeight) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.texture, texture) &&
      _generatedValueEquals(other.target, target);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(physicalWidth),
    _generatedValueHash(physicalHeight),
    _generatedValueHash(context),
    _generatedValueHash(texture),
    _generatedValueHash(target),
  ]);
}

final class OpenglOwnedTextureDescriptor {
  const OpenglOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
  });
  final RenderTargetExtent extent;
  final OpenglContextDescriptor context;

  @override
  bool operator ==(Object other) =>
      other is OpenglOwnedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
  ]);
}

final class OpenglSurfaceDescriptor {
  const OpenglSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
    this.surface = NativePointer.nullPointer,
  });
  final RenderTargetExtent extent;
  final OpenglContextDescriptor context;
  final NativePointer surface;

  @override
  bool operator ==(Object other) =>
      other is OpenglSurfaceDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.surface, surface);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
    _generatedValueHash(surface),
  ]);
}

final class RenderAbandonResult {
  const RenderAbandonResult({
    this.disposition = const RenderAbandonDisposition.fromRawValue(0),
    this.quarantinedResourceCount = 0,
  });
  final RenderAbandonDisposition disposition;
  final int quarantinedResourceCount;

  @override
  bool operator ==(Object other) =>
      other is RenderAbandonResult &&
      _generatedValueEquals(other.disposition, disposition) &&
      _generatedValueEquals(
        other.quarantinedResourceCount,
        quarantinedResourceCount,
      );
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(disposition),
    _generatedValueHash(quarantinedResourceCount),
  ]);
}

final class RenderSessionCapabilities {
  const RenderSessionCapabilities({
    required this.driver,
    this.textureRingDepth = 0,
    this.flags = const RenderSessionCapabilityFlag.fromRawValue(0),
  });
  final RenderDriverKind driver;
  final int textureRingDepth;
  final RenderSessionCapabilityFlag flags;

  @override
  bool operator ==(Object other) =>
      other is RenderSessionCapabilities &&
      _generatedValueEquals(other.driver, driver) &&
      _generatedValueEquals(other.textureRingDepth, textureRingDepth) &&
      _generatedValueEquals(other.flags, flags);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(driver),
    _generatedValueHash(textureRingDepth),
    _generatedValueHash(flags),
  ]);
}

final class RenderSessionSnapshot {
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
  bool operator ==(Object other) =>
      other is RenderSessionSnapshot &&
      _generatedValueEquals(other.state, state) &&
      _generatedValueEquals(other.driver, driver) &&
      _generatedValueEquals(other.latestResult, latestResult) &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.generation, generation) &&
      _generatedValueEquals(other.mapUpdateGeneration, mapUpdateGeneration) &&
      _generatedValueEquals(
        other.renderedUpdateGeneration,
        renderedUpdateGeneration,
      ) &&
      _generatedValueEquals(other.extentGeneration, extentGeneration) &&
      _generatedValueEquals(other.frameGeneration, frameGeneration) &&
      _generatedValueEquals(other.latestDemandToken, latestDemandToken) &&
      _generatedValueEquals(other.pendingDemandCount, pendingDemandCount) &&
      _generatedValueEquals(other.acquiredFrameCount, acquiredFrameCount) &&
      _generatedValueEquals(other.targetReady, targetReady) &&
      _generatedValueEquals(other.pendingChanges, pendingChanges);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(state),
    _generatedValueHash(driver),
    _generatedValueHash(latestResult),
    _generatedValueHash(extent),
    _generatedValueHash(generation),
    _generatedValueHash(mapUpdateGeneration),
    _generatedValueHash(renderedUpdateGeneration),
    _generatedValueHash(extentGeneration),
    _generatedValueHash(frameGeneration),
    _generatedValueHash(latestDemandToken),
    _generatedValueHash(pendingDemandCount),
    _generatedValueHash(acquiredFrameCount),
    _generatedValueHash(targetReady),
    _generatedValueHash(pendingChanges),
  ]);
}

final class ScreenBox {
  const ScreenBox({
    this.min = const ScreenPoint(0, 0),
    this.max = const ScreenPoint(0, 0),
  });
  final ScreenPoint min;
  final ScreenPoint max;

  @override
  bool operator ==(Object other) =>
      other is ScreenBox &&
      _generatedValueEquals(other.min, min) &&
      _generatedValueEquals(other.max, max);
  @override
  int get hashCode =>
      Object.hashAll([_generatedValueHash(min), _generatedValueHash(max)]);
}

final class ScreenLineString {
  ScreenLineString({required List<ScreenPoint> points})
    : points = List.unmodifiable(points);
  final List<ScreenPoint> points;

  @override
  bool operator ==(Object other) =>
      other is ScreenLineString && _generatedValueEquals(other.points, points);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(points)]);
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

final class RenderedFeatureQueryOptions {
  RenderedFeatureQueryOptions({List<String>? layerIds, Uint8List? filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds),
      filter = filter == null
          ? null
          : Uint8List.fromList(filter).asUnmodifiableView();
  final List<String>? layerIds;
  final Uint8List? filter;

  @override
  bool operator ==(Object other) =>
      other is RenderedFeatureQueryOptions &&
      _generatedValueEquals(other.layerIds, layerIds) &&
      _generatedValueEquals(other.filter, filter);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(layerIds),
    _generatedValueHash(filter),
  ]);
}

final class QueriedFeature {
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
  bool operator ==(Object other) =>
      other is QueriedFeature &&
      _generatedValueEquals(other.feature, feature) &&
      _generatedValueEquals(other.sourceId, sourceId) &&
      _generatedValueEquals(other.sourceLayerId, sourceLayerId) &&
      _generatedValueEquals(other.state, state);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(feature),
    _generatedValueHash(sourceId),
    _generatedValueHash(sourceLayerId),
    _generatedValueHash(state),
  ]);
}

final class SourceFeatureQueryOptions {
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
  bool operator ==(Object other) =>
      other is SourceFeatureQueryOptions &&
      _generatedValueEquals(other.sourceLayerIds, sourceLayerIds) &&
      _generatedValueEquals(other.filter, filter);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(sourceLayerIds),
    _generatedValueHash(filter),
  ]);
}

final class ResourceResponse {
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
  bool operator ==(Object other) =>
      other is ResourceResponse &&
      _generatedValueEquals(other.status, status) &&
      _generatedValueEquals(other.errorReason, errorReason) &&
      _generatedValueEquals(other.bytes, bytes) &&
      _generatedValueEquals(other.errorMessage, errorMessage) &&
      _generatedValueEquals(other.mustRevalidate, mustRevalidate) &&
      _generatedValueEquals(other.modifiedUnixMs, modifiedUnixMs) &&
      _generatedValueEquals(other.expiresUnixMs, expiresUnixMs) &&
      _generatedValueEquals(other.etag, etag) &&
      _generatedValueEquals(other.retryAfterUnixMs, retryAfterUnixMs);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(status),
    _generatedValueHash(errorReason),
    _generatedValueHash(bytes),
    _generatedValueHash(errorMessage),
    _generatedValueHash(mustRevalidate),
    _generatedValueHash(modifiedUnixMs),
    _generatedValueHash(expiresUnixMs),
    _generatedValueHash(etag),
    _generatedValueHash(retryAfterUnixMs),
  ]);
}

final class RuntimeOptions {
  const RuntimeOptions({
    this.flags = 0,
    this.assetPath,
    this.cachePath,
    this.eventMask = const RuntimeEventMask.fromRawValue(0),
    this.eventWake = const Wake(),
  });
  final int flags;
  final String? assetPath;
  final String? cachePath;
  final RuntimeEventMask eventMask;
  final Wake eventWake;

  @override
  bool operator ==(Object other) =>
      other is RuntimeOptions &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.assetPath, assetPath) &&
      _generatedValueEquals(other.cachePath, cachePath) &&
      _generatedValueEquals(other.eventMask, eventMask) &&
      _generatedValueEquals(other.eventWake, eventWake);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(flags),
    _generatedValueHash(assetPath),
    _generatedValueHash(cachePath),
    _generatedValueHash(eventMask),
    _generatedValueHash(eventWake),
  ]);
}

final class OfflineTilePyramidRegionDefinition {
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
  bool operator ==(Object other) =>
      other is OfflineTilePyramidRegionDefinition &&
      _generatedValueEquals(other.styleUrl, styleUrl) &&
      _generatedValueEquals(other.bounds, bounds) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.pixelRatio, pixelRatio) &&
      _generatedValueEquals(other.includeIdeographs, includeIdeographs);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(styleUrl),
    _generatedValueHash(bounds),
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(pixelRatio),
    _generatedValueHash(includeIdeographs),
  ]);
}

final class OfflineGeometryRegionDefinition {
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
  bool operator ==(Object other) =>
      other is OfflineGeometryRegionDefinition &&
      _generatedValueEquals(other.styleUrl, styleUrl) &&
      _generatedValueEquals(other.geometry, geometry) &&
      _generatedValueEquals(other.minZoom, minZoom) &&
      _generatedValueEquals(other.maxZoom, maxZoom) &&
      _generatedValueEquals(other.pixelRatio, pixelRatio) &&
      _generatedValueEquals(other.includeIdeographs, includeIdeographs);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(styleUrl),
    _generatedValueHash(geometry),
    _generatedValueHash(minZoom),
    _generatedValueHash(maxZoom),
    _generatedValueHash(pixelRatio),
    _generatedValueHash(includeIdeographs),
  ]);
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

final class OfflineRegionInfo {
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
  bool operator ==(Object other) =>
      other is OfflineRegionInfo &&
      _generatedValueEquals(other.id, id) &&
      _generatedValueEquals(other.definition, definition) &&
      _generatedValueEquals(other.metadata, metadata);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(id),
    _generatedValueHash(definition),
    _generatedValueHash(metadata),
  ]);
}

final class AdapterHttpHeader {
  const AdapterHttpHeader({this.name, this.value});
  final String? name;
  final String? value;

  @override
  bool operator ==(Object other) =>
      other is AdapterHttpHeader &&
      _generatedValueEquals(other.name, name) &&
      _generatedValueEquals(other.value, value);
  @override
  int get hashCode =>
      Object.hashAll([_generatedValueHash(name), _generatedValueHash(value)]);
}

final class AdapterHttpHeaderTransformRule {
  AdapterHttpHeaderTransformRule({
    this.kind = 0,
    this.flags = 0,
    this.url,
    required List<AdapterHttpHeader> headers,
  }) : headers = List.unmodifiable(headers);
  final int kind;
  final int flags;
  final String? url;
  final List<AdapterHttpHeader> headers;

  @override
  bool operator ==(Object other) =>
      other is AdapterHttpHeaderTransformRule &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.url, url) &&
      _generatedValueEquals(other.headers, headers);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(flags),
    _generatedValueHash(url),
    _generatedValueHash(headers),
  ]);
}

final class AdapterHttpHeaderTransformRules {
  AdapterHttpHeaderTransformRules({
    required List<AdapterHttpHeaderTransformRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterHttpHeaderTransformRule> rules;

  @override
  bool operator ==(Object other) =>
      other is AdapterHttpHeaderTransformRules &&
      _generatedValueEquals(other.rules, rules);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(rules)]);
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

final class AdapterQueuedResourceProviderRoute {
  const AdapterQueuedResourceProviderRoute({
    this.kind = 0,
    this.flags = 0,
    this.url,
  });
  final int kind;
  final int flags;
  final String? url;

  @override
  bool operator ==(Object other) =>
      other is AdapterQueuedResourceProviderRoute &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.url, url);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(flags),
    _generatedValueHash(url),
  ]);
}

final class AdapterResourceProviderRule {
  const AdapterResourceProviderRule({
    this.kind = 0,
    this.flags = 0,
    this.requestedUrl,
    required this.response,
  });
  final int kind;
  final int flags;
  final String? requestedUrl;
  final ResourceResponse response;

  @override
  bool operator ==(Object other) =>
      other is AdapterResourceProviderRule &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.requestedUrl, requestedUrl) &&
      _generatedValueEquals(other.response, response);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(flags),
    _generatedValueHash(requestedUrl),
    _generatedValueHash(response),
  ]);
}

final class AdapterResourceProviderRules {
  AdapterResourceProviderRules({
    required List<AdapterResourceProviderRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterResourceProviderRule> rules;

  @override
  bool operator ==(Object other) =>
      other is AdapterResourceProviderRules &&
      _generatedValueEquals(other.rules, rules);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(rules)]);
}

sealed class ResourceProvider {
  const ResourceProvider._();
  const factory ResourceProvider.empty() = ResourceProviderEmpty;
  const factory ResourceProvider.resourceProviderRules(
    AdapterResourceProviderRules value,
  ) = ResourceProviderResourceProviderRules;
}

final class ResourceProviderEmpty extends ResourceProvider {
  const ResourceProviderEmpty() : super._();
}

final class ResourceProviderResourceProviderRules extends ResourceProvider {
  const ResourceProviderResourceProviderRules(this.value) : super._();
  final AdapterResourceProviderRules value;
}

final class AdapterResourceRewriteRule {
  const AdapterResourceRewriteRule({
    this.kind = 0,
    this.flags = 0,
    this.url,
    this.replacementUrl,
  });
  final int kind;
  final int flags;
  final String? url;
  final String? replacementUrl;

  @override
  bool operator ==(Object other) =>
      other is AdapterResourceRewriteRule &&
      _generatedValueEquals(other.kind, kind) &&
      _generatedValueEquals(other.flags, flags) &&
      _generatedValueEquals(other.url, url) &&
      _generatedValueEquals(other.replacementUrl, replacementUrl);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(kind),
    _generatedValueHash(flags),
    _generatedValueHash(url),
    _generatedValueHash(replacementUrl),
  ]);
}

final class AdapterResourceRewriteRules {
  AdapterResourceRewriteRules({required List<AdapterResourceRewriteRule> rules})
    : rules = List.unmodifiable(rules);
  final List<AdapterResourceRewriteRule> rules;

  @override
  bool operator ==(Object other) =>
      other is AdapterResourceRewriteRules &&
      _generatedValueEquals(other.rules, rules);
  @override
  int get hashCode => Object.hashAll([_generatedValueHash(rules)]);
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

final class TextureImageInfo {
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
  bool operator ==(Object other) =>
      other is TextureImageInfo &&
      _generatedValueEquals(other.width, width) &&
      _generatedValueEquals(other.height, height) &&
      _generatedValueEquals(other.stride, stride) &&
      _generatedValueEquals(other.byteLength, byteLength);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(width),
    _generatedValueHash(height),
    _generatedValueHash(stride),
    _generatedValueHash(byteLength),
  ]);
}

final class TextureReadbackResult {
  TextureReadbackResult({Uint8List? data, this.info = const TextureImageInfo()})
    : data = Uint8List.fromList(data ?? const <int>[]).asUnmodifiableView();
  final Uint8List data;
  final TextureImageInfo info;

  @override
  bool operator ==(Object other) =>
      other is TextureReadbackResult &&
      _generatedValueEquals(other.data, data) &&
      _generatedValueEquals(other.info, info);
  @override
  int get hashCode =>
      Object.hashAll([_generatedValueHash(data), _generatedValueHash(info)]);
}

final class VulkanContextDescriptor {
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
  bool operator ==(Object other) =>
      other is VulkanContextDescriptor &&
      _generatedValueEquals(other.instance, instance) &&
      _generatedValueEquals(other.physicalDevice, physicalDevice) &&
      _generatedValueEquals(other.device, device) &&
      _generatedValueEquals(other.graphicsQueue, graphicsQueue) &&
      _generatedValueEquals(
        other.graphicsQueueFamilyIndex,
        graphicsQueueFamilyIndex,
      ) &&
      _generatedValueEquals(other.getInstanceProcAddr, getInstanceProcAddr) &&
      _generatedValueEquals(other.getDeviceProcAddr, getDeviceProcAddr);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(instance),
    _generatedValueHash(physicalDevice),
    _generatedValueHash(device),
    _generatedValueHash(graphicsQueue),
    _generatedValueHash(graphicsQueueFamilyIndex),
    _generatedValueHash(getInstanceProcAddr),
    _generatedValueHash(getDeviceProcAddr),
  ]);
}

final class VulkanBorrowedTextureDescriptor {
  const VulkanBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 0,
    this.physicalHeight = 0,
    this.context = const VulkanContextDescriptor(),
    required this.image,
    required this.imageView,
    this.format = 0,
    this.initialLayout = 0,
    this.finalLayout = 0,
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
  bool operator ==(Object other) =>
      other is VulkanBorrowedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.physicalWidth, physicalWidth) &&
      _generatedValueEquals(other.physicalHeight, physicalHeight) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.image, image) &&
      _generatedValueEquals(other.imageView, imageView) &&
      _generatedValueEquals(other.format, format) &&
      _generatedValueEquals(other.initialLayout, initialLayout) &&
      _generatedValueEquals(other.finalLayout, finalLayout);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(physicalWidth),
    _generatedValueHash(physicalHeight),
    _generatedValueHash(context),
    _generatedValueHash(image),
    _generatedValueHash(imageView),
    _generatedValueHash(format),
    _generatedValueHash(initialLayout),
    _generatedValueHash(finalLayout),
  ]);
}

final class VulkanOwnedTextureDescriptor {
  const VulkanOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final VulkanContextDescriptor context;

  @override
  bool operator ==(Object other) =>
      other is VulkanOwnedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
  ]);
}

final class VulkanSurfaceDescriptor {
  const VulkanSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
    required this.surface,
  });
  final RenderTargetExtent extent;
  final VulkanContextDescriptor context;
  final BigInt surface;

  @override
  bool operator ==(Object other) =>
      other is VulkanSurfaceDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.surface, surface);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
    _generatedValueHash(surface),
  ]);
}

final class WebgpuContextDescriptor {
  const WebgpuContextDescriptor({
    this.instance = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.queue = NativePointer.nullPointer,
  });
  final NativePointer instance;
  final NativePointer device;
  final NativePointer queue;

  @override
  bool operator ==(Object other) =>
      other is WebgpuContextDescriptor &&
      _generatedValueEquals(other.instance, instance) &&
      _generatedValueEquals(other.device, device) &&
      _generatedValueEquals(other.queue, queue);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(instance),
    _generatedValueHash(device),
    _generatedValueHash(queue),
  ]);
}

final class WebgpuBorrowedTextureDescriptor {
  const WebgpuBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 0,
    this.physicalHeight = 0,
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
  bool operator ==(Object other) =>
      other is WebgpuBorrowedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.physicalWidth, physicalWidth) &&
      _generatedValueEquals(other.physicalHeight, physicalHeight) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.texture, texture) &&
      _generatedValueEquals(other.textureView, textureView) &&
      _generatedValueEquals(other.format, format);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(physicalWidth),
    _generatedValueHash(physicalHeight),
    _generatedValueHash(context),
    _generatedValueHash(texture),
    _generatedValueHash(textureView),
    _generatedValueHash(format),
  ]);
}

final class WebgpuOwnedTextureDescriptor {
  const WebgpuOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const WebgpuContextDescriptor(),
  });
  final RenderTargetExtent extent;
  final WebgpuContextDescriptor context;

  @override
  bool operator ==(Object other) =>
      other is WebgpuOwnedTextureDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
  ]);
}

final class WebgpuSurfaceDescriptor {
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
  bool operator ==(Object other) =>
      other is WebgpuSurfaceDescriptor &&
      _generatedValueEquals(other.extent, extent) &&
      _generatedValueEquals(other.context, context) &&
      _generatedValueEquals(other.surface, surface) &&
      _generatedValueEquals(other.format, format);
  @override
  int get hashCode => Object.hashAll([
    _generatedValueHash(extent),
    _generatedValueHash(context),
    _generatedValueHash(surface),
    _generatedValueHash(format),
  ]);
}

bool _generatedValueEquals(Object? left, Object? right) {
  if (left is List && right is List) {
    if (left.length != right.length) {
      return false;
    }
    for (var index = 0; index < left.length; index++) {
      if (!_generatedValueEquals(left[index], right[index])) {
        return false;
      }
    }
    return true;
  }
  return left == right;
}

int _generatedValueHash(Object? value) => value is List
    ? Object.hashAll(value.map(_generatedValueHash))
    : value.hashCode;
