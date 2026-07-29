import { type JsonValue } from "@maplibre/native-ffi-node";
import { InvalidArgumentError } from "@maplibre/native-ffi-node/error";
import {
  projectedMetersForLatLng,
  type LatLng,
} from "@maplibre/native-ffi-node/geo";
import { setLogCallback, type LogRecord } from "@maplibre/native-ffi-node/log";
import {
  AnimationOptions,
  BoundOptions,
  CameraFitOptions,
  CameraOptions,
  FreeCameraOptions,
  MapHandle,
  MapOptions,
  MapTileOptions,
  MapViewportOptions,
  ProjectionMode,
  StyleImageOptions,
  TileSourceOptions,
  type CameraOptionsInput,
  type CustomGeometrySourceOptions,
  type ImageSourceCoordinates,
  type LocationIndicatorImageKind,
  type MapTileOptionsInput,
  type MapViewportOptionsInput,
  type PremultipliedRgba8ImageInput,
  type StyleImageInfo,
  type StyleSourceTypeValue,
} from "@maplibre/native-ffi-node/map";
import { OfflineOperationHandle } from "@maplibre/native-ffi-node/offline";
import type {
  OfflineRegionDefinitionValue,
  OfflineRegionDownloadStateValue,
} from "@maplibre/native-ffi-node/offline";
import {
  NativeBuffer,
  NativePointer,
  RenderedFeatureQueryOptions,
  RenderSessionHandle,
  SourceFeatureQueryOptions,
  type FeatureStateSelector,
  type MetalBorrowedTextureDescriptor,
  type MetalOwnedTextureDescriptor,
  type QueriedFeature,
  type RenderedQueryGeometry,
  type TextureReadbackBuffer,
} from "@maplibre/native-ffi-node/render";
import {
  ResourceRequestHandle,
  type ResourceKindValue,
  type ResourceProviderCallback,
  type ResourceResponseInput,
  type ResourceRoute,
  type ResourceTransformRule,
} from "@maplibre/native-ffi-node/resource";
import { RuntimeOptions } from "@maplibre/native-ffi-node/runtime";
import {
  RuntimeHandle,
  networkStatus,
  takeNativeLeakReports,
  type NativeLeakReport,
} from "@maplibre/native-ffi-node/runtime";

const camera: CameraOptionsInput = {
  center: { latitude: 1, longitude: 2 },
};
const coordinate: LatLng = { latitude: 1, longitude: 2 };
projectedMetersForLatLng(coordinate);
setLogCallback((record: LogRecord) => {
  void record.message;
});
networkStatus();
const leakReports: NativeLeakReport[] = takeNativeLeakReports();
void leakReports;
void InvalidArgumentError;
void MapHandle;
void OfflineOperationHandle;
void RenderSessionHandle;
void ResourceRequestHandle;
void RuntimeHandle;
void NativeBuffer;
const descriptor: MetalBorrowedTextureDescriptor = {
  extent: { width: 1, height: 1, scaleFactor: 1 },
  physicalWidth: 1,
  physicalHeight: 1,
  texture: NativePointer.null,
};
const geometry: RenderedQueryGeometry = {
  kind: "point",
  point: { x: 0, y: 0 },
};
const response: ResourceResponseInput = {
  status: "error",
  errorReason: 999_001,
  modifiedUnixMs: 1n,
};
const route: ResourceRoute = { urlPrefix: "custom://", kind: "source" };
const unknownResourceKind: ResourceKindValue = {
  kind: "unknown",
  rawKind: 1000,
};
const futureResourceRoute: ResourceRoute = {
  urlPrefix: "future://",
  kind: unknownResourceKind,
};
const providerCallback: ResourceProviderCallback = (request) => {
  request.handle.complete(response);
};
// @ts-expect-error provider callbacks complete through request.handle, not return values.
const invalidProviderCallback: ResourceProviderCallback = () => response;
RuntimeHandle.prototype.setResourceProviderRoutes.call(
  {} as RuntimeHandle,
  [route],
  providerCallback,
);
const viewportOptions: MapViewportOptionsInput = { northOrientation: "up" };
const tileOptions: MapTileOptionsInput = { lodMode: "distance" };
const matchingViewportOptions: MapViewportOptionsInput = {
  northOrientation: "right",
  northOrientationRaw: 1,
};
const matchingTileOptions: MapTileOptionsInput = {
  lodMode: "distance",
  lodModeRaw: 1,
};
// @ts-expect-error known viewport names require the corresponding raw value.
const mismatchedViewportOptions: MapViewportOptionsInput = {
  northOrientation: "right",
  northOrientationRaw: 2,
};
// @ts-expect-error known tile LOD names require the corresponding raw value.
const mismatchedTileOptions: MapTileOptionsInput = {
  lodMode: "distance",
  lodModeRaw: 0,
};
const unknownViewportOptions: MapViewportOptionsInput = {
  viewportMode: "unknown",
  viewportModeRaw: 1000,
};
const unknownTileOptions: MapTileOptionsInput = {
  lodMode: "unknown",
  lodModeRaw: 1000,
};
// @ts-expect-error unknown viewport names require their raw values.
const incompleteUnknownViewportOptions: MapViewportOptionsInput = {
  viewportMode: "unknown",
};
// @ts-expect-error unknown tile LOD names require their raw values.
const incompleteUnknownTileOptions: MapTileOptionsInput = {
  lodMode: "unknown",
};
const runtimeEvent = RuntimeHandle.prototype.pollEvent.call(
  {} as RuntimeHandle,
);
if (runtimeEvent?.payload.kind === "offline-operation-completed") {
  const operation: OfflineOperationHandle | null =
    runtimeEvent.payload.offlineOperationCompleted.operation;
  void operation;
}
const transformRule: ResourceTransformRule = {
  urlPrefix: "http://example.test/",
  replacementUrlPrefix: "https://example.test/",
};
const readbackBuffer: TextureReadbackBuffer = NativeBuffer.allocate(4);
declare const runtime: RuntimeHandle;
declare const map: MapHandle;
const imageSourceCoordinates: ImageSourceCoordinates = [
  { latitude: 1, longitude: 2 },
  { latitude: 3, longitude: 4 },
  { latitude: 5, longitude: 6 },
  { latitude: 7, longitude: 8 },
];
map.setImageSourceCoordinates("image", imageSourceCoordinates);
// @ts-expect-error image sources require exactly four ordered coordinates.
map.setImageSourceCoordinates("image", imageSourceCoordinates.slice(0, 3));
declare const offlineDefinition: OfflineRegionDefinitionValue;
if (offlineDefinition.kind === "unknown") {
  void offlineDefinition.rawType;
} else {
  void offlineDefinition.styleUrl;
}
const styleSourceType: StyleSourceTypeValue | null =
  map.getStyleSourceType("source");
void styleSourceType?.rawType;
// @ts-expect-error offline take-result APIs require typed handles, not raw ids.
runtime.offlineRegionsListTakeResult(1n);
map.addVectorSourceTiles("source", ["https://example.test/{z}/{x}/{y}"]);
// @ts-expect-error tile templates must use the array shape accepted at runtime.
map.addVectorSourceTiles("source", new Set<string>());
// @ts-expect-error custom geometry sources require a fetchTile callback.
map.addCustomGeometrySource("custom");
// @ts-expect-error prefix replacements require a matched URL prefix.
const invalidTransformRuleMissingPrefix: ResourceTransformRule = {
  replacementUrlPrefix: "https://example.test/",
};
// @ts-expect-error transform rules set exactly one replacement form.
const invalidTransformRuleBothReplacements: ResourceTransformRule = {
  urlPrefix: "http://example.test/",
  replacementUrl: "https://example.test/a",
  replacementUrlPrefix: "https://example.test/",
};
const image: PremultipliedRgba8ImageInput = {
  width: 1,
  height: 1,
  pixels: new Uint8Array(4),
};
const optionValues = [
  new RuntimeOptions({ maximumCacheSize: 1n }),
  new MapOptions({ width: 1 }),
  new CameraOptions({ zoom: 1 }),
  new AnimationOptions({ durationMs: 1 }),
  new CameraFitOptions({ bearing: 1 }),
  new FreeCameraOptions({ position: { x: 1, y: 2, z: 3 } }),
  new BoundOptions({ minZoom: 1 }),
  new MapViewportOptions({ viewportMode: "default" }),
  new MapTileOptions({ lodMode: "default" }),
  new ProjectionMode({ axonometric: true }),
  new TileSourceOptions({ tileSize: 256 }),
  new RenderedFeatureQueryOptions({ layerIds: ["layer"] }),
  new SourceFeatureQueryOptions({ sourceLayerIds: ["source-layer"] }),
  new StyleImageOptions({ pixelRatio: 2 }),
];
void optionValues.map((value) => value.copy().equals(value));
const imageInfo: StyleImageInfo = {
  width: 1,
  height: 1,
  stride: 4,
  byteLength: 4,
  pixelRatio: 1,
  sdf: false,
};
const customGeometryOptions: CustomGeometrySourceOptions = {
  fetchTile: (tileId) => void tileId.z,
};
const metalOwnedTexture: MetalOwnedTextureDescriptor = {
  extent: { width: 1, height: 1, scaleFactor: 1 },
  context: { device: NativePointer.null },
};
const metalOwnedTextureMissingDevice: MetalOwnedTextureDescriptor = {
  extent: { width: 1, height: 1, scaleFactor: 1 },
  // @ts-expect-error owned Metal textures require the device used to create them.
  context: {},
};
const locationIndicatorKind: LocationIndicatorImageKind = "top";
const featureSelector: FeatureStateSelector = { sourceId: "source" };
const keyedFeatureSelector: FeatureStateSelector = {
  sourceId: "source",
  featureId: "feature",
  stateKey: "selected",
};
// @ts-expect-error stateKey requires a featureId.
const invalidKeyedFeatureSelector: FeatureStateSelector = {
  sourceId: "source",
  stateKey: "selected",
};
const unknownOfflineDownloadState: OfflineRegionDownloadStateValue = {
  downloadState: "unknown",
  rawDownloadState: 1000,
};
declare const offlineStatus: import("@maplibre/native-ffi-node/offline").OfflineRegionStatus;
runtime.offlineRegionSetDownloadState(1n, offlineStatus);
const queriedFeature: QueriedFeature = { feature: { type: "Feature" } };
const json: JsonValue = { ok: true };
void camera;
void descriptor;
void geometry;
void response;
void route;
void unknownResourceKind;
void futureResourceRoute;
void providerCallback;
void viewportOptions;
void tileOptions;
void matchingViewportOptions;
void matchingTileOptions;
void mismatchedViewportOptions;
void mismatchedTileOptions;
void unknownViewportOptions;
void unknownTileOptions;
void incompleteUnknownViewportOptions;
void incompleteUnknownTileOptions;
void invalidProviderCallback;
void transformRule;
void readbackBuffer;
void invalidTransformRuleMissingPrefix;
void invalidTransformRuleBothReplacements;
void image;
void imageInfo;
void customGeometryOptions;
void metalOwnedTexture;
void metalOwnedTextureMissingDevice;
void locationIndicatorKind;
void featureSelector;
void keyedFeatureSelector;
void invalidKeyedFeatureSelector;
void unknownOfflineDownloadState;
void queriedFeature;
void json;
