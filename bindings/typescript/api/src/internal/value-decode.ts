/**
 * Reading the value trees the C API fills.
 *
 * A query result borrows its whole graph — geometries, coordinate runs,
 * properties, strings — from a native handle released as soon as the query
 * returns, so every leaf is copied here rather than pointed at.
 */

import type { LatLng } from "../geo.ts";
import type {
  CoordinateSpan,
  Feature,
  FeatureIdentifier,
  Geometry,
} from "../geojson.ts";
import type { JsonMember, JsonValue } from "../json.ts";
import {
  MLN_FEATURE_IDENTIFIER_TYPE,
  MLN_GEOMETRY_TYPE,
  MLN_JSON_VALUE_TYPE,
} from "../raw/enums.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

/** Reads a pointer field at the transport's width. */
function readPointer(native: Native, address: Ptr): Ptr {
  return native.memory.readPointer(address);
}

/** Reads a `size_t` count field. */
function readCount(native: Native, address: Ptr): number {
  return native.readSize(address);
}

/** Copies a borrowed `mln_string_view`. */
export function readStringView(native: Native, storage: Ptr): string {
  const layout = native.layout("mln_string_view");
  const data = readPointer(
    native,
    (storage + BigInt(layout.fields.data!.offset)) as Ptr,
  );
  const size = readCount(
    native,
    (storage + BigInt(layout.fields.size!.offset)) as Ptr,
  );
  return native.foreignString(data, size);
}

/** Reads one `mln_json_value`. */
export function readJsonValue(native: Native, storage: Ptr): JsonValue {
  const layout = native.layout("mln_json_value");
  const view = native.memory.view(storage, layout.size);
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;

  switch (view.getUint32(layout.fields.type!.offset, true)) {
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_NULL:
      return { kind: "null" };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_BOOL:
      return { kind: "bool", value: native.memory.bytes(data, 1)[0] !== 0 };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_UINT:
      return {
        kind: "uint",
        value: native.memory.view(data, 8).getBigUint64(0, true),
      };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_INT:
      return {
        kind: "int",
        value: native.memory.view(data, 8).getBigInt64(0, true),
      };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_DOUBLE:
      return {
        kind: "double",
        value: native.memory.view(data, 8).getFloat64(0, true),
      };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_STRING:
      return { kind: "string", value: readStringView(native, data) };
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_ARRAY: {
      const array = native.layout("mln_json_array");
      const values = readPointer(
        native,
        (data + BigInt(array.fields.values!.offset)) as Ptr,
      );
      const count = readCount(
        native,
        (data + BigInt(array.fields.value_count!.offset)) as Ptr,
      );
      const element = native.layout("mln_json_value");
      const items: JsonValue[] = [];
      for (let index = 0; index < count; index += 1) {
        items.push(
          readJsonValue(native, (values + BigInt(index * element.size)) as Ptr),
        );
      }
      return { kind: "array", values: items };
    }
    case MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_OBJECT: {
      const object = native.layout("mln_json_object");
      const members = readPointer(
        native,
        (data + BigInt(object.fields.members!.offset)) as Ptr,
      );
      const count = readCount(
        native,
        (data + BigInt(object.fields.member_count!.offset)) as Ptr,
      );
      return { kind: "object", members: readMembers(native, members, count) };
    }
    default:
      // A value alternative this build does not know keeps nothing it could
      // misread; an unknown tag is reported as null rather than guessed.
      return { kind: "null" };
  }
}

function readMembers(
  native: Native,
  storage: Ptr,
  count: number,
): JsonMember[] {
  const layout = native.layout("mln_json_member");
  const members: JsonMember[] = [];
  for (let index = 0; index < count; index += 1) {
    const base = (storage + BigInt(index * layout.size)) as Ptr;
    members.push({
      name: readStringView(
        native,
        (base + BigInt(layout.fields.key!.offset)) as Ptr,
      ),
      value: readJsonValue(
        native,
        readPointer(
          native,
          (base + BigInt(layout.fields.value!.offset)) as Ptr,
        ),
      ),
    });
  }
  return members;
}

/** Reads one `mln_feature`. */
export function readFeature(native: Native, storage: Ptr): Feature {
  const layout = native.layout("mln_feature");
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;

  const geometry = readGeometry(
    native,
    readPointer(native, (storage + BigInt(fields.geometry!.offset)) as Ptr),
  );
  const properties = readMembers(
    native,
    readPointer(native, (storage + BigInt(fields.properties!.offset)) as Ptr),
    readCount(native, (storage + BigInt(fields.property_count!.offset)) as Ptr),
  );
  const data = (storage + BigInt(fields.identifier!.offset)) as Ptr;

  let identifier: FeatureIdentifier = { kind: "none" };
  switch (view.getUint32(fields.identifier_type!.offset, true)) {
    case MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_UINT:
      identifier = {
        kind: "uint",
        value: native.memory.view(data, 8).getBigUint64(0, true),
      };
      break;
    case MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_INT:
      identifier = {
        kind: "int",
        value: native.memory.view(data, 8).getBigInt64(0, true),
      };
      break;
    case MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE:
      identifier = {
        kind: "double",
        value: native.memory.view(data, 8).getFloat64(0, true),
      };
      break;
    case MLN_FEATURE_IDENTIFIER_TYPE.MLN_FEATURE_IDENTIFIER_TYPE_STRING:
      identifier = { kind: "string", value: readStringView(native, data) };
      break;
    default:
      break;
  }

  return { geometry, properties, identifier };
}

/** Reads one `mln_geometry`, whichever variant it carries. */
export function readGeometry(native: Native, storage: Ptr): Geometry {
  if (storage === 0n) {
    return { kind: "empty" };
  }
  const layout = native.layout("mln_geometry");
  const view = native.memory.view(storage, layout.size);
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;

  switch (view.getUint32(layout.fields.type!.offset, true)) {
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_POINT:
      return { kind: "point", coordinate: readLatLng(native, data) };
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_LINE_STRING:
      return { kind: "lineString", coordinates: readSpan(native, data) };
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_POINT:
      return { kind: "multiPoint", coordinates: readSpan(native, data) };
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_POLYGON:
      return {
        kind: "polygon",
        rings: readSpans(
          native,
          data,
          "mln_polygon_geometry",
          "rings",
          "ring_count",
        ),
      };
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING:
      return {
        kind: "multiLineString",
        lines: readSpans(
          native,
          data,
          "mln_multi_line_geometry",
          "lines",
          "line_count",
        ),
      };
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_MULTI_POLYGON: {
      const outer = native.layout("mln_multi_polygon_geometry");
      const polygon = native.layout("mln_polygon_geometry");
      const array = readPointer(
        native,
        (data + BigInt(outer.fields.polygons!.offset)) as Ptr,
      );
      const count = readCount(
        native,
        (data + BigInt(outer.fields.polygon_count!.offset)) as Ptr,
      );
      const polygons: CoordinateSpan[][] = [];
      for (let index = 0; index < count; index += 1) {
        polygons.push([
          ...readSpans(
            native,
            (array + BigInt(index * polygon.size)) as Ptr,
            "mln_polygon_geometry",
            "rings",
            "ring_count",
          ),
        ]);
      }
      return { kind: "multiPolygon", polygons };
    }
    case MLN_GEOMETRY_TYPE.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION: {
      const collection = native.layout("mln_geometry_collection");
      const array = readPointer(
        native,
        (data + BigInt(collection.fields.geometries!.offset)) as Ptr,
      );
      const count = readCount(
        native,
        (data + BigInt(collection.fields.geometry_count!.offset)) as Ptr,
      );
      const geometries: Geometry[] = [];
      for (let index = 0; index < count; index += 1) {
        geometries.push(
          readGeometry(native, (array + BigInt(index * layout.size)) as Ptr),
        );
      }
      return { kind: "collection", geometries };
    }
    default:
      return { kind: "empty" };
  }
}

function readLatLng(native: Native, storage: Ptr): LatLng {
  const layout = native.layout("mln_lat_lng");
  const view = native.memory.view(storage, layout.size);
  return {
    latitude: view.getFloat64(layout.fields.latitude!.offset, true),
    longitude: view.getFloat64(layout.fields.longitude!.offset, true),
  };
}

function readSpan(native: Native, storage: Ptr): CoordinateSpan {
  const layout = native.layout("mln_coordinate_span");
  const point = native.layout("mln_lat_lng");
  const array = readPointer(
    native,
    (storage + BigInt(layout.fields.coordinates!.offset)) as Ptr,
  );
  const count = readCount(
    native,
    (storage + BigInt(layout.fields.coordinate_count!.offset)) as Ptr,
  );
  const coordinates: LatLng[] = [];
  for (let index = 0; index < count; index += 1) {
    coordinates.push(
      readLatLng(native, (array + BigInt(index * point.size)) as Ptr),
    );
  }
  return coordinates;
}

function readSpans(
  native: Native,
  storage: Ptr,
  record: string,
  arrayField: string,
  countField: string,
): readonly CoordinateSpan[] {
  const layout = native.layout(record);
  const span = native.layout("mln_coordinate_span");
  const array = readPointer(
    native,
    (storage + BigInt(layout.fields[arrayField]!.offset)) as Ptr,
  );
  const count = readCount(
    native,
    (storage + BigInt(layout.fields[countField]!.offset)) as Ptr,
  );
  const spans: CoordinateSpan[] = [];
  for (let index = 0; index < count; index += 1) {
    spans.push(readSpan(native, (array + BigInt(index * span.size)) as Ptr));
  }
  return spans;
}
