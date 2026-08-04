/**
 * GeoJSON values, as MapLibre holds them.
 *
 * These mirror the C API's descriptor graph rather than the GeoJSON text form:
 * a geometry is a tagged variant, a feature's properties are ordered members
 * that may repeat a name, and a feature's identifier keeps the alternative it
 * arrived as. Text is a separate path, for the entry points that take a
 * document.
 */

import type { LatLng } from "./geo.ts";
import type { JsonMember } from "./json.ts";

/** One ring or line: an ordered run of coordinates. */
export type CoordinateSpan = readonly LatLng[];

/** A polygon: an outer ring followed by its holes. */
export type PolygonRings = readonly CoordinateSpan[];

/** One geometry, tagged with the variant MapLibre holds. */
export type Geometry =
  | { readonly kind: "empty" }
  | { readonly kind: "point"; readonly coordinate: LatLng }
  | { readonly kind: "lineString"; readonly coordinates: CoordinateSpan }
  | { readonly kind: "polygon"; readonly rings: PolygonRings }
  | { readonly kind: "multiPoint"; readonly coordinates: CoordinateSpan }
  | {
      readonly kind: "multiLineString";
      readonly lines: readonly CoordinateSpan[];
    }
  | {
      readonly kind: "multiPolygon";
      readonly polygons: readonly PolygonRings[];
    }
  | { readonly kind: "collection"; readonly geometries: readonly Geometry[] };

/**
 * A feature's identifier.
 *
 * MapLibre keeps the alternative an identifier arrived as, so a number that was
 * an unsigned integer stays one.
 */
export type FeatureIdentifier =
  | { readonly kind: "none" }
  | { readonly kind: "uint"; readonly value: bigint }
  | { readonly kind: "int"; readonly value: bigint }
  | { readonly kind: "double"; readonly value: number }
  | { readonly kind: "string"; readonly value: string };

/** One feature: a geometry, its properties, and its identifier. */
export interface Feature {
  readonly geometry: Geometry;
  /** Ordered members, which may repeat a name, like any MapLibre JSON object. */
  readonly properties?: readonly JsonMember[];
  readonly identifier?: FeatureIdentifier;
}

/** What a GeoJSON value carries. */
export type GeoJson =
  | { readonly kind: "geometry"; readonly geometry: Geometry }
  | { readonly kind: "feature"; readonly feature: Feature }
  | {
      readonly kind: "featureCollection";
      readonly features: readonly Feature[];
    };

export const emptyGeometry: Geometry = { kind: "empty" };

export function pointGeometry(coordinate: LatLng): Geometry {
  return { kind: "point", coordinate };
}

export function lineStringGeometry(coordinates: CoordinateSpan): Geometry {
  return { kind: "lineString", coordinates };
}

export function polygonGeometry(rings: PolygonRings): Geometry {
  return { kind: "polygon", rings };
}

export function geoJsonGeometry(geometry: Geometry): GeoJson {
  return { kind: "geometry", geometry };
}

export function geoJsonFeature(feature: Feature): GeoJson {
  return { kind: "feature", feature };
}

export function geoJsonFeatureCollection(
  features: readonly Feature[],
): GeoJson {
  return { kind: "featureCollection", features };
}
