/**
 * Geometry values.
 *
 * These are copied language values with content equality and an independent
 * copy, so a caller can diff successive snapshots and derive a modified
 * descriptor without reaching for reference identity.
 */

/** A geographic coordinate. */
export interface LatLng {
  readonly latitude: number;
  readonly longitude: number;
}

/** A screen-space point in logical map pixels. */
export interface ScreenPoint {
  readonly x: number;
  readonly y: number;
}

/** A screen-space inset in logical map pixels. */
export interface EdgeInsets {
  readonly top: number;
  readonly left: number;
  readonly bottom: number;
  readonly right: number;
}

/** A position in Web Mercator meters. */
export interface ProjectedMeters {
  readonly northing: number;
  readonly easting: number;
}

export function latLngEquals(left: LatLng, right: LatLng): boolean {
  return left.latitude === right.latitude && left.longitude === right.longitude;
}

export function screenPointEquals(
  left: ScreenPoint,
  right: ScreenPoint,
): boolean {
  return left.x === right.x && left.y === right.y;
}

export function edgeInsetsEquals(left: EdgeInsets, right: EdgeInsets): boolean {
  return (
    left.top === right.top &&
    left.left === right.left &&
    left.bottom === right.bottom &&
    left.right === right.right
  );
}

export function projectedMetersEquals(
  left: ProjectedMeters,
  right: ProjectedMeters,
): boolean {
  return left.northing === right.northing && left.easting === right.easting;
}
