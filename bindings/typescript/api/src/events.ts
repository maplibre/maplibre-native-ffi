/**
 * Runtime events, copied into JavaScript values.
 *
 * The C API queues one event per host-visible outcome and hands out borrowed
 * storage that the next poll invalidates, so every field a poll returns here is
 * already a copy. Values this build does not know keep their raw form rather
 * than collapsing onto a known case.
 */

import {
  MLN_CAMERA_CHANGE_MODE,
  MLN_RENDER_MODE,
  MLN_RUNTIME_EVENT_PAYLOAD_TYPE,
  MLN_RUNTIME_EVENT_SOURCE_TYPE,
  MLN_RUNTIME_EVENT_TYPE,
  MLN_TILE_OPERATION,
} from "./raw/enums.ts";

/** A C enum value, which keeps its raw form when this build does not name it. */
export class NamedValue {
  protected constructor(
    readonly rawValue: number,
    readonly name: string,
  ) {}

  equals(other: NamedValue): boolean {
    return (
      this.constructor === other.constructor && this.rawValue === other.rawValue
    );
  }

  toString(): string {
    return this.name;
  }
}

function lookup<T extends NamedValue>(
  known: readonly T[],
  rawValue: number,
  make: (rawValue: number, name: string) => T,
): T {
  return (
    known.find((value) => value.rawValue === rawValue) ??
    make(rawValue, `unknown(${rawValue})`)
  );
}

/** What happened. */
export class RuntimeEventType extends NamedValue {
  static readonly mapCameraWillChange = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
    "mapCameraWillChange",
  );
  static readonly mapCameraIsChanging = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
    "mapCameraIsChanging",
  );
  static readonly mapCameraDidChange = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
    "mapCameraDidChange",
  );
  static readonly mapStyleLoaded = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
    "mapStyleLoaded",
  );
  static readonly mapLoadingStarted = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
    "mapLoadingStarted",
  );
  static readonly mapLoadingFinished = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
    "mapLoadingFinished",
  );
  static readonly mapLoadingFailed = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
    "mapLoadingFailed",
  );
  static readonly mapIdle = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_IDLE,
    "mapIdle",
  );
  static readonly mapRenderUpdateAvailable = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE,
    "mapRenderUpdateAvailable",
  );
  static readonly mapRenderError = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
    "mapRenderError",
  );
  static readonly mapStillImageFinished = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED,
    "mapStillImageFinished",
  );
  static readonly mapStillImageFailed = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED,
    "mapStillImageFailed",
  );
  static readonly mapRenderFrameStarted = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED,
    "mapRenderFrameStarted",
  );
  static readonly mapRenderFrameFinished = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
    "mapRenderFrameFinished",
  );
  static readonly mapRenderMapStarted = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED,
    "mapRenderMapStarted",
  );
  static readonly mapRenderMapFinished = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
    "mapRenderMapFinished",
  );
  static readonly mapStyleImageMissing = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
    "mapStyleImageMissing",
  );
  static readonly mapTileAction = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
    "mapTileAction",
  );
  static readonly offlineRegionStatusChanged = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
    "offlineRegionStatusChanged",
  );
  static readonly offlineRegionResponseError = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
    "offlineRegionResponseError",
  );
  static readonly offlineRegionTileCountLimitExceeded = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
    "offlineRegionTileCountLimitExceeded",
  );
  static readonly offlineOperationCompleted = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED,
    "offlineOperationCompleted",
  );
  static readonly mapCameraTransitionFinished = new RuntimeEventType(
    MLN_RUNTIME_EVENT_TYPE.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED,
    "mapCameraTransitionFinished",
  );

  static readonly #known: readonly RuntimeEventType[] = [
    RuntimeEventType.mapCameraWillChange,
    RuntimeEventType.mapCameraIsChanging,
    RuntimeEventType.mapCameraDidChange,
    RuntimeEventType.mapStyleLoaded,
    RuntimeEventType.mapLoadingStarted,
    RuntimeEventType.mapLoadingFinished,
    RuntimeEventType.mapLoadingFailed,
    RuntimeEventType.mapIdle,
    RuntimeEventType.mapRenderUpdateAvailable,
    RuntimeEventType.mapRenderError,
    RuntimeEventType.mapStillImageFinished,
    RuntimeEventType.mapStillImageFailed,
    RuntimeEventType.mapRenderFrameStarted,
    RuntimeEventType.mapRenderFrameFinished,
    RuntimeEventType.mapRenderMapStarted,
    RuntimeEventType.mapRenderMapFinished,
    RuntimeEventType.mapStyleImageMissing,
    RuntimeEventType.mapTileAction,
    RuntimeEventType.offlineRegionStatusChanged,
    RuntimeEventType.offlineRegionResponseError,
    RuntimeEventType.offlineRegionTileCountLimitExceeded,
    RuntimeEventType.offlineOperationCompleted,
    RuntimeEventType.mapCameraTransitionFinished,
  ];

  static fromRawValue(rawValue: number): RuntimeEventType {
    return lookup(
      RuntimeEventType.#known,
      rawValue,
      (raw, name) => new RuntimeEventType(raw, name),
    );
  }
}

/** What produced an event. */
export class RuntimeEventSourceType extends NamedValue {
  static readonly runtime = new RuntimeEventSourceType(
    MLN_RUNTIME_EVENT_SOURCE_TYPE.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    "runtime",
  );
  static readonly map = new RuntimeEventSourceType(
    MLN_RUNTIME_EVENT_SOURCE_TYPE.MLN_RUNTIME_EVENT_SOURCE_MAP,
    "map",
  );

  static fromRawValue(rawValue: number): RuntimeEventSourceType {
    return lookup(
      [RuntimeEventSourceType.runtime, RuntimeEventSourceType.map],
      rawValue,
      (raw, name) => new RuntimeEventSourceType(raw, name),
    );
  }
}

/** How a camera reached a new value. */
export class CameraChangeMode extends NamedValue {
  static readonly immediate = new CameraChangeMode(
    MLN_CAMERA_CHANGE_MODE.MLN_CAMERA_CHANGE_MODE_IMMEDIATE,
    "immediate",
  );
  static readonly animated = new CameraChangeMode(
    MLN_CAMERA_CHANGE_MODE.MLN_CAMERA_CHANGE_MODE_ANIMATED,
    "animated",
  );

  static fromRawValue(rawValue: number): CameraChangeMode {
    return lookup(
      [CameraChangeMode.immediate, CameraChangeMode.animated],
      rawValue,
      (raw, name) => new CameraChangeMode(raw, name),
    );
  }
}

/** Which rendering pass a render event describes. */
export class RenderMode extends NamedValue {
  static readonly partial = new RenderMode(
    MLN_RENDER_MODE.MLN_RENDER_MODE_PARTIAL,
    "partial",
  );
  static readonly full = new RenderMode(
    MLN_RENDER_MODE.MLN_RENDER_MODE_FULL,
    "full",
  );

  static fromRawValue(rawValue: number): RenderMode {
    return lookup(
      [RenderMode.partial, RenderMode.full],
      rawValue,
      (raw, name) => new RenderMode(raw, name),
    );
  }
}

/** What happened to a tile. */
export class TileOperation extends NamedValue {
  static readonly requestedFromCache = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_REQUESTED_FROM_CACHE,
    "requestedFromCache",
  );
  static readonly requestedFromNetwork = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK,
    "requestedFromNetwork",
  );
  static readonly loadFromCache = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_LOAD_FROM_CACHE,
    "loadFromCache",
  );
  static readonly loadFromNetwork = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
    "loadFromNetwork",
  );
  static readonly startParse = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_START_PARSE,
    "startParse",
  );
  static readonly endParse = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_END_PARSE,
    "endParse",
  );
  static readonly error = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_ERROR,
    "error",
  );
  static readonly cancelled = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_CANCELLED,
    "cancelled",
  );
  static readonly nullOp = new TileOperation(
    MLN_TILE_OPERATION.MLN_TILE_OPERATION_NULL,
    "nullOp",
  );

  static fromRawValue(rawValue: number): TileOperation {
    return lookup(
      [
        TileOperation.requestedFromCache,
        TileOperation.requestedFromNetwork,
        TileOperation.loadFromCache,
        TileOperation.loadFromNetwork,
        TileOperation.startParse,
        TileOperation.endParse,
        TileOperation.error,
        TileOperation.cancelled,
        TileOperation.nullOp,
      ],
      rawValue,
      (raw, name) => new TileOperation(raw, name),
    );
  }
}

/** The identity of a map an event came from. */
export class MapIdentity {
  readonly #id: bigint;

  /** @internal */
  constructor(id: bigint) {
    this.#id = id;
  }

  equals(other: MapIdentity): boolean {
    return this.#id === other.#id;
  }

  /** A key for hash-based collections, which JavaScript keys by value. */
  get key(): string {
    return `map:${this.#id}`;
  }

  toString(): string {
    return this.key;
  }
}

/** Rendering statistics a frame event reports. */
export interface RenderingStats {
  readonly encodingTime: number;
  readonly renderingTime: number;
  readonly frameCount: bigint;
  readonly drawCallCount: bigint;
  readonly totalDrawCallCount: bigint;
}

/** A tile's position in the tile pyramid. */
export interface TileId {
  readonly overscaledZ: number;
  readonly wrap: number;
  readonly canonicalZ: number;
  readonly canonicalX: number;
  readonly canonicalY: number;
}

export type RuntimeEventPayload =
  | { readonly kind: "none" }
  | {
      readonly kind: "renderFrame";
      readonly mode: RenderMode;
      readonly needsRepaint: boolean;
      readonly placementChanged: boolean;
      readonly stats: RenderingStats;
    }
  | { readonly kind: "renderMap"; readonly mode: RenderMode }
  | { readonly kind: "styleImageMissing"; readonly imageId: string }
  | {
      readonly kind: "tileAction";
      readonly operation: TileOperation;
      readonly tileId: TileId;
      readonly sourceId: string;
    }
  | { readonly kind: "cameraTransitionFinished"; readonly transitionId: bigint }
  /**
   * A payload domain this build does not know, or one whose native storage was
   * smaller than the layout this build expects. The bytes are copied as they
   * arrived.
   */
  | {
      readonly kind: "unknown";
      readonly rawType: number;
      readonly bytes: Uint8Array;
    };

/** One queued runtime event, copied out of native storage. */
export interface RuntimeEvent {
  readonly type: RuntimeEventType;
  readonly sourceType: RuntimeEventSourceType;
  /** The map an event came from, when its source is a map. */
  readonly source: MapIdentity | undefined;
  readonly code: number;
  readonly message: string;
  readonly payload: RuntimeEventPayload;
}

/** The payload kinds this build decodes, by their C payload type. */
export const PAYLOAD_TYPES = MLN_RUNTIME_EVENT_PAYLOAD_TYPE;
