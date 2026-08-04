/**
 * Materializing a GeoJSON descriptor graph for the C API.
 *
 * Every interior pointer is borrowed for the duration of the call, so the whole
 * graph — geometries, coordinate runs, ring arrays, features, and property
 * members — is written into call-scoped storage.
 */

import type { LatLng } from "../geo.ts";
import type {
  CoordinateSpan,
  Feature,
  GeoJson,
  Geometry,
  PolygonRings,
} from "../geojson.ts";
import type { JsonMember } from "../json.ts";
import {
  MLN_FEATURE_IDENTIFIER_TYPE,
  MLN_GEOJSON_TYPE,
  MLN_GEOMETRY_TYPE,
} from "../raw/enums.ts";
import { writeJsonValue, writeStringView } from "./json-encode.ts";
import type { Scope } from "./memory.ts";
import type { Native } from "./native.ts";
import { asInt64, asUint64 } from "./numbers.ts";
import type { Ptr } from "./transport.ts";

/** Writes one GeoJSON value and returns its address. */
export function writeGeoJson(
  native: Native,
  scope: Scope,
  value: GeoJson,
): Ptr {
  const layout = native.layout("mln_geojson");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;

  switch (value.kind) {
    case "geometry":
      view.setUint32(
        layout.fields.type!.offset,
        MLN_GEOJSON_TYPE.MLN_GEOJSON_TYPE_GEOMETRY,
        true,
      );
      native.memory.writePointer(
        data,
        writeGeometry(native, scope, value.geometry),
      );
      return storage;
    case "feature":
      view.setUint32(
        layout.fields.type!.offset,
        MLN_GEOJSON_TYPE.MLN_GEOJSON_TYPE_FEATURE,
        true,
      );
      native.memory.writePointer(
        data,
        writeFeature(native, scope, value.feature),
      );
      return storage;
    case "featureCollection": {
      view.setUint32(
        layout.fields.type!.offset,
        MLN_GEOJSON_TYPE.MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
        true,
      );
      const collection = native.layout("mln_feature_collection");
      const feature = native.layout("mln_feature");
      const array = scope.allocateZeroed(
        Math.max(feature.size * value.features.length, 1),
        feature.align,
      );
      value.features.forEach((entry, index) => {
        writeFeatureInto(
          native,
          scope,
          (array + BigInt(index * feature.size)) as Ptr,
          entry,
        );
      });
      native.memory.writePointer(
        (data + BigInt(collection.fields.features!.offset)) as Ptr,
        array,
      );
      writeCount(
        native,
        (data + BigInt(collection.fields.feature_count!.offset)) as Ptr,
        value.features.length,
      );
      return storage;
    }
  }
}

function writeFeature(native: Native, scope: Scope, feature: Feature): Ptr {
  const layout = native.layout("mln_feature");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  writeFeatureInto(native, scope, storage, feature);
  return storage;
}

function writeFeatureInto(
  native: Native,
  scope: Scope,
  storage: Ptr,
  feature: Feature,
): void {
  const layout = native.layout("mln_feature");
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;
  view.setUint32(fields.size!.offset, layout.size, true);
  native.memory.writePointer(
    (storage + BigInt(fields.geometry!.offset)) as Ptr,
    writeGeometry(native, scope, feature.geometry),
  );

  const properties = feature.properties ?? [];
  if (properties.length > 0) {
    native.memory.writePointer(
      (storage + BigInt(fields.properties!.offset)) as Ptr,
      writeMembers(native, scope, properties),
    );
  }
  writeCount(
    native,
    (storage + BigInt(fields.property_count!.offset)) as Ptr,
    properties.length,
  );

  const identifier = feature.identifier ?? { kind: "none" as const };
  const data = (storage + BigInt(fields.identifier!.offset)) as Ptr;
  switch (identifier.kind) {
    case "none":
      view.setUint32(
        fields.identifier_type!.offset,
        MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_NULL,
        true,
      );
      return;
    case "uint":
      view.setUint32(
        fields.identifier_type!.offset,
        MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_UINT,
        true,
      );
      native.memory
        .view(data, 8)
        .setBigUint64(
          0,
          asUint64(identifier.value, "a feature identifier"),
          true,
        );
      return;
    case "int":
      view.setUint32(
        fields.identifier_type!.offset,
        MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_INT,
        true,
      );
      native.memory
        .view(data, 8)
        .setBigInt64(
          0,
          asInt64(identifier.value, "a feature identifier"),
          true,
        );
      return;
    case "double":
      view.setUint32(
        fields.identifier_type!.offset,
        MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE,
        true,
      );
      native.memory.view(data, 8).setFloat64(0, identifier.value, true);
      return;
    case "string":
      view.setUint32(
        fields.identifier_type!.offset,
        MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_STRING,
        true,
      );
      writeStringView(native, scope, data, identifier.value);
      return;
  }
}

function writeMembers(
  native: Native,
  scope: Scope,
  members: readonly JsonMember[],
): Ptr {
  const layout = native.layout("mln_json_member");
  const array = scope.allocateZeroed(
    Math.max(layout.size * members.length, 1),
    layout.align,
  );
  members.forEach((member, index) => {
    const base = (array + BigInt(index * layout.size)) as Ptr;
    writeStringView(
      native,
      scope,
      (base + BigInt(layout.fields.key!.offset)) as Ptr,
      member.name,
    );
    native.memory.writePointer(
      (base + BigInt(layout.fields.value!.offset)) as Ptr,
      writeJsonValue(native, scope, member.value),
    );
  });
  return array;
}

function writeGeometry(native: Native, scope: Scope, geometry: Geometry): Ptr {
  const layout = native.layout("mln_geometry");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  writeGeometryInto(native, scope, storage, geometry);
  return storage;
}

function writeGeometryInto(
  native: Native,
  scope: Scope,
  storage: Ptr,
  geometry: Geometry,
): void {
  const layout = native.layout("mln_geometry");
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;
  const tag = (value: number): void => {
    view.setUint32(layout.fields.type!.offset, value, true);
  };

  switch (geometry.kind) {
    case "empty":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_EMPTY);
      return;
    case "point":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_POINT);
      writeLatLng(native, data, geometry.coordinate);
      return;
    case "lineString":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_LINE_STRING);
      writeSpan(native, scope, data, geometry.coordinates);
      return;
    case "multiPoint":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_POINT);
      writeSpan(native, scope, data, geometry.coordinates);
      return;
    case "polygon":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_POLYGON);
      writeRings(
        native,
        scope,
        data,
        geometry.rings,
        "mln_polygon_geometry",
        "rings",
        "ring_count",
      );
      return;
    case "multiLineString":
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING);
      writeRings(
        native,
        scope,
        data,
        geometry.lines,
        "mln_multi_line_geometry",
        "lines",
        "line_count",
      );
      return;
    case "multiPolygon": {
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_POLYGON);
      const outer = native.layout("mln_multi_polygon_geometry");
      const polygon = native.layout("mln_polygon_geometry");
      const array = scope.allocateZeroed(
        Math.max(polygon.size * geometry.polygons.length, 1),
        polygon.align,
      );
      geometry.polygons.forEach((rings, index) => {
        writeRings(
          native,
          scope,
          (array + BigInt(index * polygon.size)) as Ptr,
          rings,
          "mln_polygon_geometry",
          "rings",
          "ring_count",
        );
      });
      native.memory.writePointer(
        (data + BigInt(outer.fields.polygons!.offset)) as Ptr,
        array,
      );
      writeCount(
        native,
        (data + BigInt(outer.fields.polygon_count!.offset)) as Ptr,
        geometry.polygons.length,
      );
      return;
    }
    case "collection": {
      tag(MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION);
      const collection = native.layout("mln_geometry_collection");
      const array = scope.allocateZeroed(
        Math.max(layout.size * geometry.geometries.length, 1),
        layout.align,
      );
      geometry.geometries.forEach((entry, index) => {
        writeGeometryInto(
          native,
          scope,
          (array + BigInt(index * layout.size)) as Ptr,
          entry,
        );
      });
      native.memory.writePointer(
        (data + BigInt(collection.fields.geometries!.offset)) as Ptr,
        array,
      );
      writeCount(
        native,
        (data + BigInt(collection.fields.geometry_count!.offset)) as Ptr,
        geometry.geometries.length,
      );
      return;
    }
  }
}

/** Writes an array of coordinate spans, which three geometries are made of. */
function writeRings(
  native: Native,
  scope: Scope,
  storage: Ptr,
  spans: readonly CoordinateSpan[],
  record: string,
  arrayField: string,
  countField: string,
): void {
  const layout = native.layout(record);
  const span = native.layout("mln_coordinate_span");
  const array = scope.allocateZeroed(
    Math.max(span.size * spans.length, 1),
    span.align,
  );
  spans.forEach((coordinates, index) => {
    writeSpan(
      native,
      scope,
      (array + BigInt(index * span.size)) as Ptr,
      coordinates,
    );
  });
  native.memory.writePointer(
    (storage + BigInt(layout.fields[arrayField]!.offset)) as Ptr,
    array,
  );
  writeCount(
    native,
    (storage + BigInt(layout.fields[countField]!.offset)) as Ptr,
    spans.length,
  );
}

function writeSpan(
  native: Native,
  scope: Scope,
  storage: Ptr,
  coordinates: CoordinateSpan,
): void {
  const layout = native.layout("mln_coordinate_span");
  const point = native.layout("mln_lat_lng");
  const array = scope.allocateZeroed(
    Math.max(point.size * coordinates.length, 1),
    point.align,
  );
  coordinates.forEach((coordinate, index) => {
    writeLatLng(
      native,
      (array + BigInt(index * point.size)) as Ptr,
      coordinate,
    );
  });
  native.memory.writePointer(
    (storage + BigInt(layout.fields.coordinates!.offset)) as Ptr,
    array,
  );
  writeCount(
    native,
    (storage + BigInt(layout.fields.coordinate_count!.offset)) as Ptr,
    coordinates.length,
  );
}

function writeLatLng(native: Native, storage: Ptr, coordinate: LatLng): void {
  const layout = native.layout("mln_lat_lng");
  const view = native.memory.view(storage, layout.size);
  view.setFloat64(layout.fields.latitude!.offset, coordinate.latitude, true);
  view.setFloat64(layout.fields.longitude!.offset, coordinate.longitude, true);
}

function writeCount(native: Native, address: Ptr, value: number): void {
  const view = native.memory.view(address, native.transport.pointerSize);
  if (native.transport.pointerSize === 8) {
    view.setBigUint64(0, BigInt(value), true);
    return;
  }
  view.setUint32(0, value, true);
}

/** The polygon rings a caller passed, for the multi-polygon writer above. */
export type { PolygonRings };
