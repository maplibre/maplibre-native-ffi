/**
 * Feature queries.
 *
 * A query reads what a session has rendered, so it belongs to a live render
 * session rather than to the map: the answer depends on which tiles are loaded
 * and what the last frame placed. Results come back as copied values, because
 * the native result handle is released before the query returns.
 */

import type { ScreenPoint } from "./geo.ts";
import type { Feature } from "./geojson.ts";
import type { JsonValue } from "./json.ts";

/** Where on screen a rendered-feature query looks. */
export type RenderedQueryGeometry =
  | { readonly kind: "point"; readonly point: ScreenPoint }
  | {
      readonly kind: "box";
      readonly min: ScreenPoint;
      readonly max: ScreenPoint;
    }
  | { readonly kind: "lineString"; readonly points: readonly ScreenPoint[] };

/** Narrows a rendered-feature query. */
export interface RenderedFeatureQueryOptions {
  /** Style layers to look in. Every layer when absent. */
  readonly layerIds?: readonly string[];
  /** A style filter expression the features must match. */
  readonly filter?: JsonValue;
}

/** Narrows a source-feature query. */
export interface SourceFeatureQueryOptions {
  /** Source layers to look in, for a source that has them. */
  readonly sourceLayerIds?: readonly string[];
  readonly filter?: JsonValue;
}

/** One feature a query found, copied out of the native result. */
export interface QueriedFeature {
  readonly feature: Feature;
  /** The source the feature came from, when the result names one. */
  readonly sourceId: string | undefined;
  /** The source layer, for a source that has them. */
  readonly sourceLayerId: string | undefined;
  /** The feature state this map holds for it, when there is any. */
  readonly state: JsonValue | undefined;
}

export function pointQuery(point: ScreenPoint): RenderedQueryGeometry {
  return { kind: "point", point };
}

export function boxQuery(
  min: ScreenPoint,
  max: ScreenPoint,
): RenderedQueryGeometry {
  return { kind: "box", min, max };
}

export function lineStringQuery(
  points: readonly ScreenPoint[],
): RenderedQueryGeometry {
  return { kind: "lineString", points };
}
