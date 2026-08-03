/**
 * Structured JSON, as MapLibre itself holds it.
 *
 * MapLibre distinguishes an unsigned integer, a signed integer, and a double,
 * and it reads some values only from one of those alternatives — a cluster id
 * that arrives as a double reads as absent. An object also keeps its member
 * order and may repeat a name. A plain JavaScript object can express none of
 * that, so a value here is a tagged alternative and an object is an ordered list
 * of members.
 *
 * Style documents cross as text instead, because native holds a document as the
 * bytes it parsed. This model is for the value APIs.
 */

/** One JSON value, tagged with the alternative MapLibre holds. */
export type JsonValue =
  | { readonly kind: "null" }
  | { readonly kind: "bool"; readonly value: boolean }
  /** An unsigned 64-bit integer, over the full domain. */
  | { readonly kind: "uint"; readonly value: bigint }
  /** A signed 64-bit integer, over the full domain. */
  | { readonly kind: "int"; readonly value: bigint }
  | { readonly kind: "double"; readonly value: number }
  | { readonly kind: "string"; readonly value: string }
  | { readonly kind: "array"; readonly values: readonly JsonValue[] }
  | { readonly kind: "object"; readonly members: readonly JsonMember[] };

/** One member of a JSON object. Names may repeat, and order is preserved. */
export interface JsonMember {
  readonly name: string;
  readonly value: JsonValue;
}

export const jsonNull: JsonValue = { kind: "null" };

export function jsonBool(value: boolean): JsonValue {
  return { kind: "bool", value };
}

export function jsonUint(value: bigint): JsonValue {
  return { kind: "uint", value };
}

export function jsonInt(value: bigint): JsonValue {
  return { kind: "int", value };
}

export function jsonDouble(value: number): JsonValue {
  return { kind: "double", value };
}

export function jsonString(value: string): JsonValue {
  return { kind: "string", value };
}

export function jsonArray(values: readonly JsonValue[]): JsonValue {
  return { kind: "array", values };
}

export function jsonObject(members: readonly JsonMember[]): JsonValue {
  return { kind: "object", members };
}

/**
 * Builds a value from ordinary JavaScript data.
 *
 * Numbers become doubles and `bigint`s become signed or unsigned integers, so a
 * caller that needs a specific alternative names it rather than hoping. Objects
 * keep the order their keys enumerate in.
 */
export function jsonFrom(value: unknown): JsonValue {
  if (value === null || value === undefined) {
    return jsonNull;
  }
  switch (typeof value) {
    case "boolean":
      return jsonBool(value);
    case "number":
      return jsonDouble(value);
    case "bigint":
      return value < 0n ? jsonInt(value) : jsonUint(value);
    case "string":
      return jsonString(value);
    default:
      break;
  }
  if (Array.isArray(value)) {
    return jsonArray(value.map(jsonFrom));
  }
  return jsonObject(
    Object.entries(value as Record<string, unknown>).map(([name, member]) => ({
      name,
      value: jsonFrom(member),
    })),
  );
}

/** Compares two values by content, including member order and alternative. */
export function jsonEquals(left: JsonValue, right: JsonValue): boolean {
  if (left.kind !== right.kind) {
    return false;
  }
  switch (left.kind) {
    case "null":
      return true;
    case "bool":
    case "uint":
    case "int":
    case "double":
    case "string":
      return left.value === (right as typeof left).value;
    case "array": {
      const other = right as typeof left;
      return (
        left.values.length === other.values.length &&
        left.values.every((value, index) =>
          jsonEquals(value, other.values[index]!),
        )
      );
    }
    case "object": {
      const other = right as typeof left;
      return (
        left.members.length === other.members.length &&
        left.members.every(
          (member, index) =>
            member.name === other.members[index]!.name &&
            jsonEquals(member.value, other.members[index]!.value),
        )
      );
    }
  }
}
