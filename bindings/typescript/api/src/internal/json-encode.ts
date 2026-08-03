/**
 * Materializing a structured JSON value for the C API.
 *
 * The C API takes a descriptor graph whose every interior pointer is borrowed
 * for the duration of the call, so the whole graph is written into call-scoped
 * storage: the values, the arrays, the members, and the UTF-8 bytes of every
 * string and key.
 */

import { MaplibreError } from "../errors.ts";
import type { JsonMember, JsonValue } from "../json.ts";
import { MLN_JSON_VALUE_TYPE } from "../raw/enums.ts";
import type { Scope } from "./memory.ts";
import type { Native } from "./native.ts";
import { asInt64, asUint64 } from "./numbers.ts";
import type { Ptr } from "./transport.ts";

const encoder = new TextEncoder();

/** Writes one value and returns its address. */
export function writeJsonValue(
  native: Native,
  scope: Scope,
  value: JsonValue,
): Ptr {
  const layout = native.layout("mln_json_value");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  writeJsonValueInto(native, scope, storage, value);
  return storage;
}

/** Writes one value into storage the caller already has. */
export function writeJsonValueInto(
  native: Native,
  scope: Scope,
  storage: Ptr,
  value: JsonValue,
): void {
  const layout = native.layout("mln_json_value");
  const fields = layout.fields;
  const view = native.memory.view(storage, layout.size);
  view.setUint32(fields.size!.offset, layout.size, true);
  const data = (storage + BigInt(fields.data!.offset)) as Ptr;

  switch (value.kind) {
    case "null":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_NULL,
        true,
      );
      return;
    case "bool":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_BOOL,
        true,
      );
      native.memory.view(data, 1).setUint8(0, value.value ? 1 : 0);
      return;
    case "uint":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_UINT,
        true,
      );
      native.memory
        .view(data, 8)
        .setBigUint64(
          0,
          asUint64(value.value, "a JSON unsigned integer"),
          true,
        );
      return;
    case "int":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_INT,
        true,
      );
      native.memory
        .view(data, 8)
        .setBigInt64(0, asInt64(value.value, "a JSON signed integer"), true);
      return;
    case "double":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_DOUBLE,
        true,
      );
      native.memory.view(data, 8).setFloat64(0, value.value, true);
      return;
    case "string":
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_STRING,
        true,
      );
      writeStringView(native, scope, data, value.value);
      return;
    case "array": {
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_ARRAY,
        true,
      );
      const array = native.layout("mln_json_array");
      const elements = scope.allocateZeroed(
        Math.max(layout.size * value.values.length, 1),
        layout.align,
      );
      value.values.forEach((element, index) => {
        writeJsonValueInto(
          native,
          scope,
          (elements + BigInt(index * layout.size)) as Ptr,
          element,
        );
      });
      native.memory.writePointer(
        (data + BigInt(array.fields.values!.offset)) as Ptr,
        elements,
      );
      writeSize(
        native,
        (data + BigInt(array.fields.value_count!.offset)) as Ptr,
        value.values.length,
      );
      return;
    }
    case "object": {
      view.setUint32(
        fields.type!.offset,
        MLN_JSON_VALUE_TYPE.MLN_JSON_VALUE_TYPE_OBJECT,
        true,
      );
      const object = native.layout("mln_json_object");
      const member = native.layout("mln_json_member");
      const members = scope.allocateZeroed(
        Math.max(member.size * value.members.length, 1),
        member.align,
      );
      value.members.forEach((entry: JsonMember, index) => {
        const base = (members + BigInt(index * member.size)) as Ptr;
        writeStringView(
          native,
          scope,
          (base + BigInt(member.fields.key!.offset)) as Ptr,
          entry.name,
        );
        native.memory.writePointer(
          (base + BigInt(member.fields.value!.offset)) as Ptr,
          writeJsonValue(native, scope, entry.value),
        );
      });
      native.memory.writePointer(
        (data + BigInt(object.fields.members!.offset)) as Ptr,
        members,
      );
      writeSize(
        native,
        (data + BigInt(object.fields.member_count!.offset)) as Ptr,
        value.members.length,
      );
      return;
    }
  }
}

/**
 * Writes an `mln_string_view`.
 *
 * A string view carries bytes and a length, so an embedded NUL is allowed here
 * where a null-terminated input would reject it.
 */
export function writeStringView(
  native: Native,
  scope: Scope,
  storage: Ptr,
  value: string,
): void {
  const layout = native.layout("mln_string_view");
  const bytes = encoder.encode(value);
  if (bytes.length > 0) {
    const data = scope.allocate(bytes.length, 1);
    native.memory.bytes(data, bytes.length).set(bytes);
    native.memory.writePointer(
      (storage + BigInt(layout.fields.data!.offset)) as Ptr,
      data,
    );
  } else {
    native.memory.writePointer(
      (storage + BigInt(layout.fields.data!.offset)) as Ptr,
      0n as Ptr,
    );
  }
  writeSize(
    native,
    (storage + BigInt(layout.fields.size!.offset)) as Ptr,
    bytes.length,
  );
}

/** Allocates and fills an `mln_string_view`, returning its address. */
export function stringView(native: Native, scope: Scope, value: string): Ptr {
  const layout = native.layout("mln_string_view");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  writeStringView(native, scope, storage, value);
  return storage;
}

function writeSize(native: Native, address: Ptr, value: number): void {
  if (!Number.isInteger(value) || value < 0) {
    throw new MaplibreError(
      "invalidInput",
      `a length must be a count, not ${value}`,
    );
  }
  const view = native.memory.view(address, native.transport.pointerSize);
  if (native.transport.pointerSize === 8) {
    view.setBigUint64(0, BigInt(value), true);
    return;
  }
  view.setUint32(0, value, true);
}
