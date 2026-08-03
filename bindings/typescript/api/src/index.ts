/**
 * Low-level TypeScript binding for the MapLibre Native C API.
 *
 * This package owns every public name. A runtime payload package supplies one
 * compiled artifact and the metadata describing it, and defines no MapLibre API
 * of its own.
 */

export {
  type AnimationOptions,
  type CameraOptions,
  cameraOptionsEquals,
  copyCameraOptions,
  type UnitBezier,
} from "./camera.ts";
export {
  type ErrorKind,
  MaplibreError,
  type MaplibreErrorOptions,
} from "./errors.ts";
export {
  CameraChangeMode,
  MapIdentity,
  NamedValue,
  RenderMode,
  type RenderingStats,
  type RuntimeEvent,
  type RuntimeEventPayload,
  RuntimeEventSourceType,
  RuntimeEventType,
  type TileId,
  TileOperation,
} from "./events.ts";
export {
  type EdgeInsets,
  edgeInsetsEquals,
  type LatLng,
  latLngEquals,
  type ProjectedMeters,
  projectedMetersEquals,
  type ScreenPoint,
  screenPointEquals,
} from "./geo.ts";
export { AbiMismatchError } from "./internal/node-transport.ts";
export { Map, MapMode, type MapOptions, type MapSize } from "./map.ts";
export {
  type LogCallbackOptions,
  LogEvent,
  type LogRecord,
  LogSeverity,
} from "./logging.ts";
export {
  type LoadOptions,
  Maplibre,
  NetworkStatus,
  type RenderBackends,
} from "./maplibre.ts";
export {
  jsonArray,
  jsonBool,
  jsonDouble,
  jsonEquals,
  jsonFrom,
  jsonInt,
  type JsonMember,
  jsonNull,
  jsonObject,
  jsonString,
  jsonUint,
  type JsonValue,
} from "./json.ts";
export { MapProjection } from "./projection.ts";
export {
  ResourceErrorReason,
  ResourceRequest,
  type ResourceRequestInfo,
  type ResourceResponse,
  ResourceResponseStatus,
} from "./resource-request.ts";
export {
  ANY_RESOURCE_KIND,
  ResourceKind,
  type ResourceRewriteRule,
  type ResourceRoute,
} from "./resources.ts";
export {
  type PumpTimeout,
  Runtime,
  type RuntimeOptions,
  WakeSource,
  WakeSourceTransfer,
} from "./runtime.ts";
