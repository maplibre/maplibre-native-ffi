/**
 * Offline operations.
 *
 * Offline work runs against a database, so it takes as long as it takes. Every
 * operation is therefore a command: the call reports that the operation was
 * accepted and hands back an id, the completion arrives as a runtime event, and
 * the result is taken afterwards.
 *
 * Taking a result transfers ownership once. A take that fails leaves the
 * operation there to retry.
 */

import { NamedValue } from "./events.ts";
import {
  MLN_AMBIENT_CACHE_OPERATION,
  MLN_OFFLINE_REGION_DEFINITION_TYPE,
} from "./raw/enums.ts";

/** Identifies an operation across its start, its event, and its result. */
export type OfflineOperationId = bigint;

/** Which shape an offline region covers. */
export class OfflineRegionDefinitionType extends NamedValue {
  static readonly tilePyramid = new OfflineRegionDefinitionType(
    MLN_OFFLINE_REGION_DEFINITION_TYPE.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    "tilePyramid",
  );
  static readonly geometry = new OfflineRegionDefinitionType(
    MLN_OFFLINE_REGION_DEFINITION_TYPE.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
    "geometry",
  );

  static fromRawValue(rawValue: number): OfflineRegionDefinitionType {
    return (
      [
        OfflineRegionDefinitionType.tilePyramid,
        OfflineRegionDefinitionType.geometry,
      ].find((value) => value.rawValue === rawValue) ??
      new OfflineRegionDefinitionType(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** What the ambient cache operation should do. */
export class AmbientCacheOperation extends NamedValue {
  static readonly resetDatabase = new AmbientCacheOperation(
    MLN_AMBIENT_CACHE_OPERATION.MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE,
    "resetDatabase",
  );
  static readonly packDatabase = new AmbientCacheOperation(
    MLN_AMBIENT_CACHE_OPERATION.MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE,
    "packDatabase",
  );
  static readonly invalidate = new AmbientCacheOperation(
    MLN_AMBIENT_CACHE_OPERATION.MLN_AMBIENT_CACHE_OPERATION_INVALIDATE,
    "invalidate",
  );
  static readonly clear = new AmbientCacheOperation(
    MLN_AMBIENT_CACHE_OPERATION.MLN_AMBIENT_CACHE_OPERATION_CLEAR,
    "clear",
  );

  static fromRawValue(rawValue: number): AmbientCacheOperation {
    return (
      [
        AmbientCacheOperation.resetDatabase,
        AmbientCacheOperation.packDatabase,
        AmbientCacheOperation.invalidate,
        AmbientCacheOperation.clear,
      ].find((value) => value.rawValue === rawValue) ??
      new AmbientCacheOperation(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** One offline region, copied out of the list the C API handed back. */
export interface OfflineRegion {
  readonly id: bigint;
  readonly definitionType: OfflineRegionDefinitionType;
  /** The host-supplied bytes this region was created with. */
  readonly metadata: Uint8Array;
}
