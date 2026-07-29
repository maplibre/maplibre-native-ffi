/// <reference lib="esnext.disposable" />
export declare const MaplibreStatus: Readonly<{
  invalidArgument: "invalid-argument";
  invalidState: "invalid-state";
  wrongThread: "wrong-thread";
  unsupported: "unsupported";
  nativeError: "native-error";
  abiVersionMismatch: "abi-version-mismatch";
  unknownStatus: "unknown-status";
}>;

export type MaplibreStatusKind =
  (typeof MaplibreStatus)[keyof typeof MaplibreStatus];

export declare class MaplibreError extends Error {
  readonly status: MaplibreStatusKind;
  readonly nativeStatusCode: number | null;
  readonly diagnostic: string;
  constructor(
    status: MaplibreStatusKind,
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export declare class InvalidArgumentError extends MaplibreError {
  constructor(
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export declare class InvalidStateError extends MaplibreError {
  constructor(
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export declare class WrongThreadError extends MaplibreError {
  constructor(
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export declare class UnsupportedFeatureError extends MaplibreError {
  constructor(
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export declare class NativeError extends MaplibreError {
  constructor(
    nativeStatusCode: number | null,
    diagnostic: string,
    options?: ErrorOptions,
  );
}

export interface RenderBackends {
  rawMask: number;
  metal: boolean;
  vulkan: boolean;
  opengl: boolean;
}

export interface OpenGLContextProviders {
  rawMask: number;
  wgl: boolean;
  egl: boolean;
}

export type NetworkStatusValue =
  | { kind: "online"; raw: 1 }
  | { kind: "offline"; raw: 2 }
  | { kind: "unknown"; raw: number };

export interface NativeLeakReport {
  handleType: string;
  address: bigint;
}

export interface RuntimeOptionsInput {
  assetPath?: string | null;
  cachePath?: string | null;
  maximumCacheSize?: bigint | null;
}

export declare class RuntimeOptions implements RuntimeOptionsInput {
  constructor(input?: RuntimeOptionsInput | null);
  readonly assetPath?: string | null;
  readonly cachePath?: string | null;
  readonly maximumCacheSize?: bigint | null;
  equals(other: unknown): boolean;
  copy(changes?: RuntimeOptionsInput | null): RuntimeOptions;
}

export interface RuntimeEvent {
  eventType: string;
  rawEventType: number;
  sourceType: string;
  rawSourceType: number;
  sourceMap: MapHandle | null;
  code: number;
  cameraChangeMode?: "immediate" | "animated" | "unknown" | null;
  rawCameraChangeMode?: number | null;
  message?: string | null;
  payloadKind: RuntimeEventPayload["kind"];
  payload: RuntimeEventPayload;
}

export type RuntimeEventPayload =
  | RuntimeEventPayloadNone
  | RuntimeEventPayloadRenderFrame
  | RuntimeEventPayloadRenderMap
  | RuntimeEventPayloadStyleImageMissing
  | RuntimeEventPayloadTileAction
  | RuntimeEventPayloadOfflineRegionStatus
  | RuntimeEventPayloadOfflineRegionResponseError
  | RuntimeEventPayloadOfflineRegionTileCountLimit
  | RuntimeEventPayloadOfflineOperationCompleted
  | RuntimeEventPayloadCameraTransitionFinished
  | RuntimeEventPayloadUnknown;

export interface RuntimeEventPayloadBase {
  kind: string;
  rawType: number;
}

export interface RuntimeEventPayloadNone extends RuntimeEventPayloadBase {
  kind: "none";
}

export interface RenderingStats {
  encodingTime: number;
  renderingTime: number;
  frameCount: bigint;
  drawCallCount: bigint;
  totalDrawCallCount: bigint;
}

export type RenderMode = "partial" | "full" | "unknown";

export interface RuntimeEventPayloadRenderFrame extends RuntimeEventPayloadBase {
  kind: "render-frame";
  renderFrame: {
    mode: RenderMode;
    rawMode: number;
    needsRepaint: boolean;
    placementChanged: boolean;
    stats: RenderingStats;
  };
}

export interface RuntimeEventPayloadRenderMap extends RuntimeEventPayloadBase {
  kind: "render-map";
  renderMap: {
    mode: RenderMode;
    rawMode: number;
  };
}

export interface RuntimeEventPayloadStyleImageMissing extends RuntimeEventPayloadBase {
  kind: "style-image-missing";
  styleImageMissing: { imageId: string };
}

export type TileOperation =
  | "requestedFromCache"
  | "requestedFromNetwork"
  | "loadFromNetwork"
  | "loadFromCache"
  | "startParse"
  | "endParse"
  | "error"
  | "cancelled"
  | "null"
  | "unknown";

export interface TileId {
  overscaledZ: number;
  wrap: number;
  canonicalZ: number;
  canonicalX: number;
  canonicalY: number;
}

export interface RuntimeEventPayloadTileAction extends RuntimeEventPayloadBase {
  kind: "tile-action";
  tileAction: {
    operation: TileOperation;
    rawOperation: number;
    tileId: TileId;
    sourceId: string;
  };
}

export interface RuntimeEventPayloadOfflineRegionStatus extends RuntimeEventPayloadBase {
  kind: "offline-region-status";
  offlineRegionStatus: {
    regionId: bigint;
    status: OfflineRegionStatus;
  };
}

export type ResourceErrorReason =
  | "none"
  | "notFound"
  | "server"
  | "connection"
  | "rateLimit"
  | "other"
  | "unknown";

export interface RuntimeEventPayloadOfflineRegionResponseError extends RuntimeEventPayloadBase {
  kind: "offline-region-response-error";
  offlineRegionResponseError: {
    regionId: bigint;
    reason: ResourceErrorReason;
    rawReason: number;
  };
}

export interface RuntimeEventPayloadOfflineRegionTileCountLimit extends RuntimeEventPayloadBase {
  kind: "offline-region-tile-count-limit";
  offlineRegionTileCountLimit: {
    regionId: bigint;
    limit: bigint;
  };
}

export type OfflineOperationKind =
  | "ambientCache"
  | "regionCreate"
  | "regionGet"
  | "regionsList"
  | "regionsMergeDatabase"
  | "regionUpdateMetadata"
  | "regionGetStatus"
  | "regionSetObserved"
  | "regionSetDownloadState"
  | "regionInvalidate"
  | "regionDelete"
  | "unknown";

export type OfflineOperationResultKind =
  | "none"
  | "region"
  | "optionalRegion"
  | "regionList"
  | "regionStatus"
  | "unknown";

export interface RuntimeEventPayloadOfflineOperationCompleted extends RuntimeEventPayloadBase {
  kind: "offline-operation-completed";
  offlineOperationCompleted: {
    operation: OfflineOperationHandle | null;
    operationKind: OfflineOperationKind;
    rawOperationKind: number;
    resultKind: OfflineOperationResultKind;
    rawResultKind: number;
    resultStatus: number;
    found: boolean;
  };
}

export interface RuntimeEventPayloadCameraTransitionFinished extends RuntimeEventPayloadBase {
  kind: "camera-transition-finished";
  cameraTransitionFinished: {
    transitionId: bigint;
  };
}

export interface RuntimeEventPayloadUnknown extends RuntimeEventPayloadBase {
  kind: "unknown";
  unknown: {
    rawType: number;
    bytes: Uint8Array;
  };
}

export type MapDebugOption =
  | "tileBorders"
  | "parseStatus"
  | "timestamps"
  | "collision"
  | "overdraw"
  | "stencilClip"
  | "depthBuffer";

export interface CameraOptionsInput {
  center?: LatLng | null;
  zoom?: number | null;
  bearing?: number | null;
  pitch?: number | null;
  centerAltitude?: number | null;
  padding?: EdgeInsets | null;
  anchor?: ScreenPoint | null;
  roll?: number | null;
  fieldOfView?: number | null;
}

export declare class CameraOptions implements CameraOptionsInput {
  constructor(input?: CameraOptionsInput | null);
  readonly center?: LatLng | null;
  readonly zoom?: number | null;
  readonly bearing?: number | null;
  readonly pitch?: number | null;
  readonly centerAltitude?: number | null;
  readonly padding?: EdgeInsets | null;
  readonly anchor?: ScreenPoint | null;
  readonly roll?: number | null;
  readonly fieldOfView?: number | null;
  equals(other: unknown): boolean;
  copy(changes?: CameraOptionsInput | null): CameraOptions;
}

export interface UnitBezier {
  x1: number;
  y1: number;
  x2: number;
  y2: number;
}

export interface AnimationOptionsInput {
  durationMs?: number | null;
  velocity?: number | null;
  minZoom?: number | null;
  easing?: UnitBezier | null;
  transitionId?: bigint | null;
}

export declare class AnimationOptions implements AnimationOptionsInput {
  constructor(input?: AnimationOptionsInput | null);
  readonly durationMs?: number | null;
  readonly velocity?: number | null;
  readonly minZoom?: number | null;
  readonly easing?: UnitBezier | null;
  readonly transitionId?: bigint | null;
  equals(other: unknown): boolean;
  copy(changes?: AnimationOptionsInput | null): AnimationOptions;
}

export interface Vec3 {
  x: number;
  y: number;
  z: number;
}

export interface Quaternion {
  x: number;
  y: number;
  z: number;
  w: number;
}

export interface FreeCameraOptionsInput {
  position?: Vec3 | null;
  orientation?: Quaternion | null;
}

export declare class FreeCameraOptions implements FreeCameraOptionsInput {
  constructor(input?: FreeCameraOptionsInput | null);
  readonly position?: Vec3 | null;
  readonly orientation?: Quaternion | null;
  equals(other: unknown): boolean;
  copy(changes?: FreeCameraOptionsInput | null): FreeCameraOptions;
}

export interface EdgeInsets {
  top: number;
  left: number;
  bottom: number;
  right: number;
}

export interface CameraFitOptionsInput {
  padding?: EdgeInsets | null;
  bearing?: number | null;
  pitch?: number | null;
}

export declare class CameraFitOptions implements CameraFitOptionsInput {
  constructor(input?: CameraFitOptionsInput | null);
  readonly padding?: EdgeInsets | null;
  readonly bearing?: number | null;
  readonly pitch?: number | null;
  equals(other: unknown): boolean;
  copy(changes?: CameraFitOptionsInput | null): CameraFitOptions;
}

type NorthOrientationInput =
  | { northOrientation?: null; northOrientationRaw?: null }
  | { northOrientation: "up"; northOrientationRaw?: 0 | null }
  | { northOrientation: "right"; northOrientationRaw?: 1 | null }
  | { northOrientation: "down"; northOrientationRaw?: 2 | null }
  | { northOrientation: "left"; northOrientationRaw?: 3 | null }
  | {
      northOrientation?: null;
      northOrientationRaw: number;
    }
  | { northOrientation: "unknown"; northOrientationRaw: number };

type ConstrainModeInput =
  | { constrainMode?: null; constrainModeRaw?: null }
  | { constrainMode: "none"; constrainModeRaw?: 0 | null }
  | { constrainMode: "heightOnly"; constrainModeRaw?: 1 | null }
  | { constrainMode: "widthAndHeight"; constrainModeRaw?: 2 | null }
  | { constrainMode: "screen"; constrainModeRaw?: 3 | null }
  | {
      constrainMode?: null;
      constrainModeRaw: number;
    }
  | { constrainMode: "unknown"; constrainModeRaw: number };

type ViewportModeInput =
  | { viewportMode?: null; viewportModeRaw?: null }
  | { viewportMode: "default"; viewportModeRaw?: 0 | null }
  | { viewportMode: "flippedY"; viewportModeRaw?: 1 | null }
  | {
      viewportMode?: null;
      viewportModeRaw: number;
    }
  | { viewportMode: "unknown"; viewportModeRaw: number };

export type MapViewportOptionsInput = NorthOrientationInput &
  ConstrainModeInput &
  ViewportModeInput & {
    frustumOffset?: EdgeInsets | null;
  };

export declare class MapViewportOptions {
  constructor(input?: MapViewportOptionsInput | null);
  readonly northOrientation?:
    | "up"
    | "right"
    | "down"
    | "left"
    | "unknown"
    | null;
  readonly northOrientationRaw?: number | null;
  readonly constrainMode?:
    | "none"
    | "heightOnly"
    | "widthAndHeight"
    | "screen"
    | "unknown"
    | null;
  readonly constrainModeRaw?: number | null;
  readonly viewportMode?: "default" | "flippedY" | "unknown" | null;
  readonly viewportModeRaw?: number | null;
  readonly frustumOffset?: EdgeInsets | null;
  equals(other: unknown): boolean;
  copy(changes?: MapViewportOptionsInput | null): MapViewportOptions;
}

type MapTileOptionsInputFields = {
  prefetchZoomDelta?: number | null;
  lodMinRadius?: number | null;
  lodScale?: number | null;
  lodPitchThreshold?: number | null;
  lodZoomShift?: number | null;
};

export type MapTileOptionsInput = MapTileOptionsInputFields &
  (
    | { lodMode?: null; lodModeRaw?: null }
    | { lodMode: "default"; lodModeRaw?: 0 | null }
    | { lodMode: "distance"; lodModeRaw?: 1 | null }
    | { lodMode?: null; lodModeRaw: number }
    | { lodMode: "unknown"; lodModeRaw: number }
  );

export declare class MapTileOptions {
  constructor(input?: MapTileOptionsInput | null);
  readonly prefetchZoomDelta?: number | null;
  readonly lodMinRadius?: number | null;
  readonly lodScale?: number | null;
  readonly lodPitchThreshold?: number | null;
  readonly lodZoomShift?: number | null;
  readonly lodMode?: "default" | "distance" | "unknown" | null;
  readonly lodModeRaw?: number | null;
  equals(other: unknown): boolean;
  copy(changes?: MapTileOptionsInput | null): MapTileOptions;
}

export interface BoundOptionsInput {
  bounds?: LatLngBounds | null;
  unbounded?: boolean | null;
  minZoom?: number | null;
  maxZoom?: number | null;
  minPitch?: number | null;
  maxPitch?: number | null;
}

export declare class BoundOptions implements BoundOptionsInput {
  constructor(input?: BoundOptionsInput | null);
  readonly bounds?: LatLngBounds | null;
  readonly unbounded?: boolean | null;
  readonly minZoom?: number | null;
  readonly maxZoom?: number | null;
  readonly minPitch?: number | null;
  readonly maxPitch?: number | null;
  equals(other: unknown): boolean;
  copy(changes?: BoundOptionsInput | null): BoundOptions;
}

export interface ProjectionModeInput {
  axonometric?: boolean | null;
  xSkew?: number | null;
  ySkew?: number | null;
}

export declare class ProjectionMode implements ProjectionModeInput {
  constructor(input?: ProjectionModeInput | null);
  readonly axonometric?: boolean | null;
  readonly xSkew?: number | null;
  readonly ySkew?: number | null;
  equals(other: unknown): boolean;
  copy(changes?: ProjectionModeInput | null): ProjectionMode;
}

export interface MapOptionsInput {
  width?: number | null;
  height?: number | null;
  scaleFactor?: number | null;
  mapMode?: "continuous" | "static" | "tile" | null;
  fastPforEnabled?: boolean | null;
}

export declare class MapOptions implements MapOptionsInput {
  constructor(input?: MapOptionsInput | null);
  readonly width?: number | null;
  readonly height?: number | null;
  readonly scaleFactor?: number | null;
  readonly mapMode?: "continuous" | "static" | "tile" | null;
  readonly fastPforEnabled?: boolean | null;
  equals(other: unknown): boolean;
  copy(changes?: MapOptionsInput | null): MapOptions;
}

export declare class NativePointer {
  static readonly null: NativePointer;
  static unsafeFromAddress(address: bigint): NativePointer;
  private constructor(address: bigint);
  readonly address: bigint;
  readonly isNull: boolean;
  equals(other: unknown): boolean;
  toString(): string;
}

export declare class NativeBuffer {
  static allocate(byteLength: number): NativeBuffer;
  static from(data: NativeBuffer | ArrayBuffer | ArrayBufferView): NativeBuffer;
  constructor(data: number | ArrayBuffer | ArrayBufferView);
  readonly byteLength: number;
  asArrayBuffer(): ArrayBuffer;
  asUint8Array(): Uint8Array;
  readonly [Symbol.toStringTag]: "NativeBuffer";
}

export type AmbientCacheOperation =
  | "resetDatabase"
  | "packDatabase"
  | "invalidate"
  | "clear";

export type ResourceKind =
  | "unknown"
  | "style"
  | "source"
  | "tile"
  | "glyphs"
  | "sprite-image"
  | "sprite-json"
  | "image";

export type ResourceKindValue =
  | { kind: "unknown"; rawKind: number }
  | { kind: "style"; rawKind: 1 }
  | { kind: "source"; rawKind: 2 }
  | { kind: "tile"; rawKind: 3 }
  | { kind: "glyphs"; rawKind: 4 }
  | { kind: "sprite-image"; rawKind: 5 }
  | { kind: "sprite-json"; rawKind: 6 }
  | { kind: "image"; rawKind: 7 };

export interface ResourceRoute {
  kind?: ResourceKind | ResourceKindValue | null;
  url?: string | null;
  urlPrefix?: string | null;
}

export type ResourceTransformRule =
  | (ResourceRoute & {
      replacementUrl: string;
      replacementUrlPrefix?: null;
    })
  | (Omit<ResourceRoute, "urlPrefix"> & {
      urlPrefix: string;
      replacementUrl?: null;
      replacementUrlPrefix: string;
    });

export interface ResourceByteRange {
  start: bigint;
  end: bigint;
}

export interface ResourceProviderRequest {
  url: string;
  kind: ResourceKind;
  rawKind: number;
  loadingMethod: "all" | "cacheOnly" | "networkOnly" | "unknown";
  rawLoadingMethod: number;
  priority: "regular" | "low" | "unknown";
  rawPriority: number;
  usage: "online" | "offline" | "unknown";
  rawUsage: number;
  storagePolicy: "permanent" | "volatile" | "unknown";
  rawStoragePolicy: number;
  range?: ResourceByteRange | null;
  priorModifiedUnixMs?: bigint | null;
  priorExpiresUnixMs?: bigint | null;
  priorEtag?: string | null;
  priorData: Uint8Array;
  handle: ResourceRequestHandle;
}

export interface ResourceResponseInput {
  status?: "ok" | "error" | "noContent" | "notModified" | null;
  errorReason?:
    | "none"
    | "notFound"
    | "server"
    | "connection"
    | "rateLimit"
    | "other"
    | number
    | null;
  bytes?: Uint8Array | null;
  errorMessage?: string | null;
  mustRevalidate?: boolean | null;
  modifiedUnixMs?: bigint | null;
  expiresUnixMs?: bigint | null;
  etag?: string | null;
  retryAfterUnixMs?: bigint | null;
}

export type ResourceProviderCallback = (
  request: ResourceProviderRequest,
) => void | PromiseLike<void>;

export declare class ResourceRequestHandle {
  private constructor(nativeHandle: unknown);
  readonly closed: boolean;
  complete(response?: ResourceResponseInput): void;
  cancelled(): boolean;
  close(): void;
  [Symbol.dispose](): void;
}

export declare class RuntimeHandle {
  constructor(options?: RuntimeOptionsInput | RuntimeOptions | null);
  readonly closed: boolean;
  createMap(options?: MapOptionsInput | MapOptions | null): MapHandle;
  close(): void;
  pump(timeoutMs?: number | null): void;
  acquireWakeSource(): WakeSourceHandle;
  setResourceTransformRules(rules: readonly ResourceTransformRule[]): void;
  clearResourceTransform(): void;
  setResourceProviderRoutes(
    routes: readonly ResourceRoute[],
    callback: ResourceProviderCallback,
  ): void;
  clearResourceProvider(): void;
  runAmbientCacheOperation(
    operation: AmbientCacheOperation,
  ): OfflineOperationHandle;
  offlineRegionsList(): OfflineOperationHandle;
  offlineRegionGet(regionId: bigint): OfflineOperationHandle;
  offlineRegionsMergeDatabase(path: string): OfflineOperationHandle;
  offlineRegionUpdateMetadata(
    regionId: bigint,
    metadata?: Uint8Array | null,
  ): OfflineOperationHandle;
  offlineRegionGetStatus(regionId: bigint): OfflineOperationHandle;
  offlineRegionSetObserved(
    regionId: bigint,
    observed: boolean,
  ): OfflineOperationHandle;
  offlineRegionSetDownloadState(
    regionId: bigint,
    state: OfflineRegionDownloadState | OfflineRegionDownloadStateValue,
  ): OfflineOperationHandle;
  offlineRegionInvalidate(regionId: bigint): OfflineOperationHandle;
  offlineRegionDelete(regionId: bigint): OfflineOperationHandle;
  offlineRegionCreate(
    definition: OfflineRegionDefinition,
    metadata?: Uint8Array | null,
  ): OfflineOperationHandle;
  offlineRegionCreateTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionInfo;
  offlineRegionGetTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionInfo | null;
  offlineRegionsListTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionInfo[];
  offlineRegionsMergeDatabaseTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionInfo[];
  offlineRegionUpdateMetadataTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionInfo;
  offlineRegionGetStatusTakeResult(
    operation: OfflineOperationRef,
  ): OfflineRegionStatus;
  pollEvent(): RuntimeEvent | null;
  [Symbol.dispose](): void;
}

export declare class WakeSourceHandle {
  private constructor(nativeHandle: unknown);
  static fromTransfer(transfer: WakeSourceTransfer): WakeSourceHandle;
  readonly closed: boolean;
  signal(): void;
  transfer(): WakeSourceTransfer;
  close(): void;
  [Symbol.dispose](): void;
}

export interface WakeSourceTransfer {
  readonly kind: "wakeSource";
  readonly token: string;
  cancel?(): void;
}

export interface OfflineTilePyramidRegionDefinition {
  kind: "tilePyramid";
  styleUrl: string;
  bounds: LatLngBounds;
  minZoom: number;
  maxZoom: number;
  pixelRatio: number;
  includeIdeographs?: boolean | null;
}

export interface OfflineGeometryRegionDefinition {
  kind: "geometry";
  styleUrl: string;
  geometry: JsonValue;
  minZoom: number;
  maxZoom: number;
  pixelRatio: number;
  includeIdeographs?: boolean | null;
}

export type OfflineRegionDefinition =
  | OfflineTilePyramidRegionDefinition
  | OfflineGeometryRegionDefinition;

export interface OfflineTilePyramidRegionDefinitionValue {
  kind: "tilePyramid";
  rawType: 1;
  styleUrl: string;
  bounds: LatLngBounds;
  minZoom: number;
  maxZoom: number;
  pixelRatio: number;
  includeIdeographs: boolean;
}

export interface OfflineGeometryRegionDefinitionValue {
  kind: "geometry";
  rawType: 2;
  styleUrl: string;
  geometry: JsonValue;
  minZoom: number;
  maxZoom: number;
  pixelRatio: number;
  includeIdeographs: boolean;
}

export interface OfflineUnknownRegionDefinitionValue {
  kind: "unknown";
  rawType: number;
}

export type OfflineRegionDefinitionValue =
  | OfflineTilePyramidRegionDefinitionValue
  | OfflineGeometryRegionDefinitionValue
  | OfflineUnknownRegionDefinitionValue;

export interface OfflineRegionInfo {
  id: bigint;
  definition: OfflineRegionDefinitionValue;
  metadata: Uint8Array;
}

export type OfflineRegionStatus = OfflineRegionDownloadStateValue & {
  completedResourceCount: bigint;
  completedResourceSize: bigint;
  completedTileCount: bigint;
  requiredTileCount: bigint;
  completedTileSize: bigint;
  requiredResourceCount: bigint;
  requiredResourceCountIsPrecise: boolean;
  complete: boolean;
};

export type OfflineOperationRef = OfflineOperationHandle;

export type OfflineRegionDownloadState = "inactive" | "active";

export type OfflineRegionDownloadStateValue =
  | {
      downloadState: "inactive";
      rawDownloadState: 0;
    }
  | {
      downloadState: "active";
      rawDownloadState: 1;
    }
  | {
      downloadState: "unknown";
      rawDownloadState: number;
    };

export declare class OfflineOperationHandle {
  private constructor(nativeHandle: unknown);
  readonly closed: boolean;
  close(): void;
  [Symbol.dispose](): void;
}

export interface RenderTargetExtent {
  width: number;
  height: number;
  scaleFactor: number;
}

export interface PhysicalSize {
  width: number;
  height: number;
}

export declare function renderTargetExtentPhysicalSize(
  extent: RenderTargetExtent,
): PhysicalSize;

export interface MetalContextDescriptor {
  device?: NativePointer | null;
}

export interface MetalOwnedTextureDescriptor {
  extent: RenderTargetExtent;
  context: MetalContextDescriptor & { device: NativePointer };
}

export interface MetalBorrowedTextureDescriptor {
  extent: RenderTargetExtent;
  physicalWidth: number;
  physicalHeight: number;
  texture: NativePointer;
}

export interface MetalSurfaceDescriptor {
  extent: RenderTargetExtent;
  context: MetalContextDescriptor;
  layer: NativePointer;
}

export interface VulkanContextDescriptor {
  instance: NativePointer;
  physicalDevice: NativePointer;
  device: NativePointer;
  graphicsQueue: NativePointer;
  graphicsQueueFamilyIndex: number;
  getInstanceProcAddr?: NativePointer | null;
  getDeviceProcAddr?: NativePointer | null;
}

export interface WglContextDescriptor {
  platform: "wgl";
  deviceContext: NativePointer;
  shareContext: NativePointer;
  getProcAddress?: NativePointer | null;
}

export interface EglContextDescriptor {
  platform: "egl";
  display: NativePointer;
  config: NativePointer;
  shareContext: NativePointer;
  getProcAddress?: NativePointer | null;
}

export type OpenGLContextDescriptor =
  | WglContextDescriptor
  | EglContextDescriptor;

export interface VulkanOwnedTextureDescriptor {
  extent: RenderTargetExtent;
  context: VulkanContextDescriptor;
}

export interface VulkanBorrowedTextureDescriptor {
  extent: RenderTargetExtent;
  physicalWidth: number;
  physicalHeight: number;
  context: VulkanContextDescriptor;
  image: NativePointer;
  imageView: NativePointer;
  format: number;
  initialLayout: number;
  finalLayout: number;
}

export interface VulkanSurfaceDescriptor {
  extent: RenderTargetExtent;
  context: VulkanContextDescriptor;
  surface: NativePointer;
}

export interface OpenGLOwnedTextureDescriptor {
  extent: RenderTargetExtent;
  context: OpenGLContextDescriptor;
}

export interface OpenGLBorrowedTextureDescriptor {
  extent: RenderTargetExtent;
  physicalWidth: number;
  physicalHeight: number;
  context: OpenGLContextDescriptor;
  texture: number;
  target: number;
}

export interface OpenGLSurfaceDescriptor {
  extent: RenderTargetExtent;
  context: OpenGLContextDescriptor;
  surface: NativePointer;
}

export interface TextureImageInfo {
  width: number;
  height: number;
  stride: number;
  byteLength: number;
}

export type TextureReadbackBuffer =
  | NativeBuffer
  | ArrayBuffer
  | ArrayBufferView;

export declare class MetalOwnedTextureFrame {
  private constructor(nativeFrame: unknown);
  readonly closed: boolean;
  readonly generation: bigint;
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  readonly frameId: bigint;
  readonly texture: NativePointer;
  readonly device: NativePointer;
  readonly pixelFormat: bigint;
  close(): void;
  [Symbol.dispose](): void;
}

export declare class VulkanOwnedTextureFrame {
  private constructor(nativeFrame: unknown);
  readonly closed: boolean;
  readonly generation: bigint;
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  readonly frameId: bigint;
  readonly image: NativePointer;
  readonly imageView: NativePointer;
  readonly device: NativePointer;
  readonly format: number;
  readonly layout: number;
  close(): void;
  [Symbol.dispose](): void;
}

export declare class OpenGLOwnedTextureFrame {
  private constructor(nativeFrame: unknown);
  readonly closed: boolean;
  readonly generation: bigint;
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  readonly frameId: bigint;
  readonly texture: number;
  readonly target: number;
  readonly internalFormat: number;
  readonly format: number;
  readonly type: number;
  close(): void;
  [Symbol.dispose](): void;
}

export type FeatureStateSelector =
  | {
      sourceId: string;
      sourceLayerId?: string | null;
      featureId: string;
      stateKey?: string | null;
    }
  | {
      sourceId: string;
      sourceLayerId?: string | null;
      featureId?: null;
      stateKey?: null;
    };

export interface ScreenBox {
  min: ScreenPoint;
  max: ScreenPoint;
}

export type RenderedQueryGeometry =
  | { kind: "point"; point: ScreenPoint }
  | { kind: "box"; box: ScreenBox }
  | { kind: "lineString"; points: ScreenPoint[] };

export interface RenderedFeatureQueryOptionsInput {
  layerIds?: readonly string[] | null;
  filter?: JsonValue | null;
}

export declare class RenderedFeatureQueryOptions implements RenderedFeatureQueryOptionsInput {
  constructor(input?: RenderedFeatureQueryOptionsInput | null);
  readonly layerIds?: readonly string[] | null;
  readonly filter?: JsonValue | null;
  equals(other: unknown): boolean;
  copy(
    changes?: RenderedFeatureQueryOptionsInput | null,
  ): RenderedFeatureQueryOptions;
}

export interface SourceFeatureQueryOptionsInput {
  sourceLayerIds?: readonly string[] | null;
  filter?: JsonValue | null;
}

export declare class SourceFeatureQueryOptions implements SourceFeatureQueryOptionsInput {
  constructor(input?: SourceFeatureQueryOptionsInput | null);
  readonly sourceLayerIds?: readonly string[] | null;
  readonly filter?: JsonValue | null;
  equals(other: unknown): boolean;
  copy(
    changes?: SourceFeatureQueryOptionsInput | null,
  ): SourceFeatureQueryOptions;
}

export interface QueriedFeature {
  feature: JsonValue;
  sourceId?: string | null;
  sourceLayerId?: string | null;
  state?: JsonValue | null;
}

export type FeatureExtensionResult =
  | { kind: "value"; value: JsonValue }
  | { kind: "featureCollection"; features: JsonValue[] }
  | { kind: "unknown"; rawType: number };

export interface MapAttachReferenceTransfer {
  readonly kind: "mapAttachReference";
  readonly token: string;
  cancel?(): void;
}

export declare class MapAttachReference {
  private constructor(nativeHandle: unknown);
  static fromTransfer(transfer: MapAttachReferenceTransfer): MapAttachReference;
  readonly closed: boolean;
  transfer(): MapAttachReferenceTransfer;
  attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle;
  attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle;
  attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle;
}

export declare class RenderSessionHandle {
  private constructor(
    nativeHandle: unknown,
    map: MapHandle | MapAttachReference,
  );
  readonly map: MapHandle | MapAttachReference;
  readonly closed: boolean;
  close(): void;
  resize(width: number, height: number, scaleFactor: number): void;
  renderUpdate(): boolean;
  detach(): void;
  reduceMemoryUse(): void;
  clearData(): void;
  dumpDebugLogs(): void;
  setFeatureState(selector: FeatureStateSelector, state: JsonValue): void;
  getFeatureState(selector: FeatureStateSelector): JsonValue;
  removeFeatureState(selector: FeatureStateSelector): void;
  queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options?:
      | RenderedFeatureQueryOptionsInput
      | RenderedFeatureQueryOptions
      | null,
  ): QueriedFeature[];
  querySourceFeatures(
    sourceId: string,
    options?: SourceFeatureQueryOptionsInput | SourceFeatureQueryOptions | null,
  ): QueriedFeature[];
  queryFeatureExtension(
    sourceId: string,
    feature: JsonValue,
    extension: string,
    extensionField: string,
    args?: JsonValue | null,
  ): FeatureExtensionResult;
  acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrame;
  acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrame;
  acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrame;
  readPremultipliedRgba8Into(data: TextureReadbackBuffer): TextureImageInfo;
  [Symbol.dispose](): void;
}

export declare class MapProjectionHandle {
  private constructor(map: MapHandle);
  readonly closed: boolean;
  close(): void;
  getCamera(): CameraOptions;
  setCamera(camera: CameraOptionsInput | CameraOptions): void;
  setVisibleCoordinates(coordinates: LatLng[], padding: EdgeInsets): void;
  setVisibleGeometry(geometry: JsonValue, padding: EdgeInsets): void;
  pixelForLatLng(coordinate: LatLng): ScreenPoint;
  latLngForPixel(point: ScreenPoint): LatLng;
  [Symbol.dispose](): void;
}

export declare class MapHandle {
  private constructor(
    runtime: RuntimeHandle,
    options?: MapOptionsInput | MapOptions | null,
  );
  readonly closed: boolean;
  renderingStatsViewEnabled: boolean;
  close(): void;
  createProjection(): MapProjectionHandle;
  getSize(): MapSize;
  attachReference(): MapAttachReference;
  attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle;
  attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle;
  attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
  ): RenderSessionHandle;
  attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
  ): RenderSessionHandle;
  attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle;
  requestRepaint(): void;
  requestStillImage(): void;
  isFullyLoaded(): boolean;
  dumpDebugLogs(): void;
  getDebugOptions(): MapDebugOption[];
  getDebugOptionsRawMask(): number;
  setDebugOptions(options: Iterable<MapDebugOption>): void;
  setDebugOptionsRawMask(mask: number): void;
  getViewportOptions(): MapViewportOptions;
  setViewportOptions(
    options: MapViewportOptionsInput | MapViewportOptions,
  ): void;
  getTileOptions(): MapTileOptions;
  setTileOptions(options: MapTileOptionsInput | MapTileOptions): void;
  getBounds(): BoundOptions;
  setBounds(options: BoundOptionsInput | BoundOptions): void;
  getFreeCameraOptions(): FreeCameraOptions;
  setFreeCameraOptions(
    options: FreeCameraOptionsInput | FreeCameraOptions,
  ): void;
  getProjectionMode(): ProjectionMode;
  setProjectionMode(mode: ProjectionModeInput | ProjectionMode): void;
  moveBy(deltaX: number, deltaY: number): void;
  scaleBy(scale: number, anchor?: ScreenPoint | null): void;
  rotateBy(first: ScreenPoint, second: ScreenPoint): void;
  pitchBy(pitch: number): void;
  moveByAnimated(
    deltaX: number,
    deltaY: number,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  scaleByAnimated(
    scale: number,
    anchor?: ScreenPoint | null,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  pitchByAnimated(
    pitch: number,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  cancelTransitions(): void;
  getCamera(): CameraOptions;
  jumpTo(camera: CameraOptionsInput | CameraOptions): void;
  easeTo(
    camera: CameraOptionsInput | CameraOptions,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  flyTo(
    camera: CameraOptionsInput | CameraOptions,
    animation?: AnimationOptionsInput | AnimationOptions | null,
  ): void;
  cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions?: CameraFitOptionsInput | CameraFitOptions | null,
  ): CameraOptions;
  cameraForLatLngs(
    coordinates: LatLng[],
    fitOptions?: CameraFitOptionsInput | CameraFitOptions | null,
  ): CameraOptions;
  cameraForGeometry(
    geometry: JsonValue,
    fitOptions?: CameraFitOptionsInput | CameraFitOptions | null,
  ): CameraOptions;
  latLngBoundsForCamera(
    camera: CameraOptionsInput | CameraOptions,
  ): LatLngBounds;
  latLngBoundsForCameraUnwrapped(
    camera: CameraOptionsInput | CameraOptions,
  ): LatLngBounds;
  pixelForLatLng(coordinate: LatLng): ScreenPoint;
  latLngForPixel(point: ScreenPoint): LatLng;
  pixelsForLatLngs(coordinates: LatLng[]): ScreenPoint[];
  latLngsForPixels(points: ScreenPoint[]): LatLng[];
  addStyleSourceJson(sourceId: string, source: JsonValue): void;
  styleSourceExists(sourceId: string): boolean;
  removeStyleSource(sourceId: string): boolean;
  listStyleSourceIds(): string[];
  getStyleSourceType(sourceId: string): StyleSourceTypeValue | null;
  getStyleSourceInfo(sourceId: string): StyleSourceInfo | null;
  addGeoJsonSourceUrl(
    sourceId: string,
    url: string,
    options?: GeoJsonSourceOptionsInput | GeoJsonSourceOptions | null,
  ): void;
  addGeoJsonSourceData(
    sourceId: string,
    data: JsonValue,
    options?: GeoJsonSourceOptionsInput | GeoJsonSourceOptions | null,
  ): void;
  setGeoJsonSourceUrl(sourceId: string, url: string): void;
  setGeoJsonSourceData(sourceId: string, data: JsonValue): void;
  addVectorSourceUrl(
    sourceId: string,
    url: string,
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addRasterSourceUrl(
    sourceId: string,
    url: string,
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addRasterDemSourceUrl(
    sourceId: string,
    url: string,
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addVectorSourceTiles(
    sourceId: string,
    tiles: readonly string[],
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addRasterSourceTiles(
    sourceId: string,
    tiles: readonly string[],
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addRasterDemSourceTiles(
    sourceId: string,
    tiles: readonly string[],
    options?: TileSourceOptionsInput | TileSourceOptions | null,
  ): void;
  addCustomGeometrySource(
    sourceId: string,
    options: CustomGeometrySourceOptions,
  ): void;
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: JsonValue,
  ): void;
  invalidateCustomGeometrySourceTile(
    sourceId: string,
    tileId: CanonicalTileId,
  ): void;
  invalidateCustomGeometrySourceRegion(
    sourceId: string,
    bounds: LatLngBounds,
  ): void;
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8ImageInput,
    options?: StyleImageOptionsInput | StyleImageOptions | null,
  ): void;
  styleImageExists(imageId: string): boolean;
  removeStyleImage(imageId: string): boolean;
  getStyleImageInfo(imageId: string): StyleImageInfo | null;
  copyStyleImagePremultipliedRgba8(imageId: string): StyleImage | null;
  addImageSourceUrl(
    sourceId: string,
    coordinates: ImageSourceCoordinates,
    url: string,
  ): void;
  addImageSourceImage(
    sourceId: string,
    coordinates: ImageSourceCoordinates,
    image: PremultipliedRgba8ImageInput,
  ): void;
  setImageSourceUrl(sourceId: string, url: string): void;
  setImageSourceImage(
    sourceId: string,
    image: PremultipliedRgba8ImageInput,
  ): void;
  setImageSourceCoordinates(
    sourceId: string,
    coordinates: ImageSourceCoordinates,
  ): void;
  getImageSourceCoordinates(sourceId: string): ImageSourceCoordinates | null;
  addHillshadeLayer(
    layerId: string,
    sourceId: string,
    beforeLayerId?: string | null,
  ): void;
  addColorReliefLayer(
    layerId: string,
    sourceId: string,
    beforeLayerId?: string | null,
  ): void;
  addLocationIndicatorLayer(
    layerId: string,
    beforeLayerId?: string | null,
  ): void;
  setLocationIndicatorLocation(
    layerId: string,
    coordinate: LatLng,
    altitude?: number,
  ): void;
  setLocationIndicatorBearing(layerId: string, bearing: number): void;
  setLocationIndicatorAccuracyRadius(layerId: string, radius: number): void;
  setLocationIndicatorImageName(
    layerId: string,
    imageKind: LocationIndicatorImageKind,
    imageId: string,
  ): void;
  addStyleLayerJson(layer: JsonValue, beforeLayerId?: string | null): void;
  styleLayerExists(layerId: string): boolean;
  removeStyleLayer(layerId: string): boolean;
  listStyleLayerIds(): string[];
  getStyleLayerType(layerId: string): string | null;
  getStyleLayerJson(layerId: string): JsonValue | null;
  moveStyleLayer(layerId: string, beforeLayerId?: string | null): void;
  setLayerProperty(
    layerId: string,
    propertyName: string,
    value: JsonValue,
  ): void;
  getLayerProperty(layerId: string, propertyName: string): JsonValue | null;
  setLayerFilter(layerId: string, filter: JsonValue | null): void;
  getLayerFilter(layerId: string): JsonValue | null;
  setStyleLight(light: JsonValue): void;
  setStyleLightProperty(propertyName: string, value: JsonValue): void;
  getStyleLightProperty(propertyName: string): JsonValue | null;
  setStyleJson(json: string): void;
  setStyleUrl(url: string): void;
  [Symbol.dispose](): void;
}

export interface LatLng {
  latitude: number;
  longitude: number;
}

export type ImageSourceCoordinates = readonly [LatLng, LatLng, LatLng, LatLng];

export interface LatLngBounds {
  southwest: LatLng;
  northeast: LatLng;
}

export interface ProjectedMeters {
  northing: number;
  easting: number;
}

export interface ScreenPoint {
  x: number;
  y: number;
}

/**
 * JavaScript-native JSON value used by structured JSON and GeoJSON APIs.
 *
 * Object member order, duplicate names, and integer precision follow
 * JavaScript JSON semantics. Use `MapHandle.setStyleJson(json: string)` for a
 * raw style document that must pass through without wrapper parsing.
 */
export type JsonValue =
  | null
  | boolean
  | number
  | string
  | JsonValue[]
  | { [key: string]: JsonValue };

export type StyleSourceType =
  | "unknown"
  | "vector"
  | "raster"
  | "raster-dem"
  | "geojson"
  | "image"
  | "video"
  | "annotations"
  | "custom-vector";

export interface StyleSourceTypeValue {
  kind: StyleSourceType;
  rawType: number;
}

export interface StyleSourceInfo {
  sourceType: StyleSourceType;
  rawType: number;
  idSize: number;
  isVolatile: boolean;
  hasAttribution: boolean;
  attributionSize: number;
  attribution?: string | null;
}

export interface TileSourceOptionsInput {
  minZoom?: number | null;
  maxZoom?: number | null;
  attribution?: string | null;
  scheme?: "xyz" | "tms" | null;
  bounds?: LatLngBounds | null;
  tileSize?: number | null;
  vectorEncoding?: "mvt" | "mlt" | null;
  rasterDemEncoding?: "mapbox" | "terrarium" | null;
}

export declare class TileSourceOptions implements TileSourceOptionsInput {
  constructor(input?: TileSourceOptionsInput | null);
  readonly minZoom?: number | null;
  readonly maxZoom?: number | null;
  readonly attribution?: string | null;
  readonly scheme?: "xyz" | "tms" | null;
  readonly bounds?: LatLngBounds | null;
  readonly tileSize?: number | null;
  readonly vectorEncoding?: "mvt" | "mlt" | null;
  readonly rasterDemEncoding?: "mapbox" | "terrarium" | null;
  equals(other: unknown): boolean;
  copy(changes?: TileSourceOptionsInput | null): TileSourceOptions;
}

export interface CanonicalTileId {
  z: number;
  x: number;
  y: number;
}

export type CustomGeometrySourceCallback = (tileId: CanonicalTileId) => void;

export interface CustomGeometrySourceOptions {
  fetchTile: CustomGeometrySourceCallback;
  cancelTile?: CustomGeometrySourceCallback | null;
  minZoom?: number | null;
  maxZoom?: number | null;
  tolerance?: number | null;
  tileSize?: number | null;
  buffer?: number | null;
  clip?: boolean | null;
  wrap?: boolean | null;
}

export interface PremultipliedRgba8ImageInput {
  width: number;
  height: number;
  stride?: number | null;
  pixels: Uint8Array;
}

export interface StyleImageOptionsInput {
  pixelRatio?: number | null;
  sdf?: boolean | null;
}

export interface MapSize {
  width: number;
  height: number;
  pixelRatio: number;
}

export interface GeoJsonSourceOptionsInput {
  minZoom?: number | null;
  maxZoom?: number | null;
  tolerance?: number | null;
  clusterMaxZoom?: number | null;
  clusterProperties?: JsonValue | null;
  tileSize?: number | null;
  buffer?: number | null;
  clusterRadius?: number | null;
  clusterMinPoints?: number | null;
  lineMetrics?: boolean | null;
  cluster?: boolean | null;
}

export declare class GeoJsonSourceOptions implements GeoJsonSourceOptionsInput {
  constructor(input?: GeoJsonSourceOptionsInput | null);
  readonly minZoom?: number | null;
  readonly maxZoom?: number | null;
  readonly tolerance?: number | null;
  readonly clusterMaxZoom?: number | null;
  readonly clusterProperties?: JsonValue | null;
  readonly tileSize?: number | null;
  readonly buffer?: number | null;
  readonly clusterRadius?: number | null;
  readonly clusterMinPoints?: number | null;
  readonly lineMetrics?: boolean | null;
  readonly cluster?: boolean | null;
  equals(other: unknown): boolean;
  copy(changes?: GeoJsonSourceOptionsInput | null): GeoJsonSourceOptions;
}

export declare class StyleImageOptions implements StyleImageOptionsInput {
  constructor(input?: StyleImageOptionsInput | null);
  readonly pixelRatio?: number | null;
  readonly sdf?: boolean | null;
  equals(other: unknown): boolean;
  copy(changes?: StyleImageOptionsInput | null): StyleImageOptions;
}

export interface StyleImageInfo {
  width: number;
  height: number;
  stride: number;
  byteLength: number;
  pixelRatio: number;
  sdf: boolean;
}

export interface StyleImage extends StyleImageInfo {
  pixels: Uint8Array;
}

export type LocationIndicatorImageKind = "top" | "bearing" | "shadow";

export declare function cVersion(): number;
export declare function supportedRenderBackends(): RenderBackends;
export declare function supportedOpenGLContextProviders(): OpenGLContextProviders;
export declare function threadLastErrorMessage(): string;
export declare function takeNativeLeakReports(): NativeLeakReport[];
export declare function networkStatus(): NetworkStatusValue;
export type LogSeverity = "info" | "warning" | "error";

export interface LogRecord {
  severity: LogSeverity | "unknown";
  rawSeverity: number;
  event: string;
  rawEvent: number;
  code: bigint;
  message: string;
}

export type LogCallback = (record: LogRecord) => void;

export declare function setNetworkStatus(
  status: "online" | "offline" | NetworkStatusValue,
): void;
export declare function setLogCallback(callback: LogCallback): void;
export declare function clearLogCallback(): void;
export declare function setAsyncLogSeverities(
  severities: Iterable<LogSeverity>,
): void;
export declare function setAsyncLogSeverityMask(mask: number): void;
export declare function restoreDefaultAsyncLogSeverities(): void;
export declare function projectedMetersForLatLng(
  coordinate: LatLng,
): ProjectedMeters;
export declare function latLngForProjectedMeters(
  meters: ProjectedMeters,
): LatLng;
