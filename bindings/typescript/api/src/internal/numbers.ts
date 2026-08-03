/**
 * Checking that a public value survives the trip to C.
 *
 * `DataView` writes and `BigInt.asUintN` narrow modulo the target width, so a
 * width of `2**32 + 1` would reach C as one and a negative duration would reach
 * it as an enormous positive one. The binding owns that: a value it cannot
 * represent is rejected here, and every value it can represent is passed through
 * for the C API to validate.
 */

import { MaplibreError } from "../errors.ts";

const UINT32_MAX = 0xffff_ffff;
const UINT64_MAX = (1n << 64n) - 1n;
const INT64_MIN = -(1n << 63n);
const INT64_MAX = (1n << 63n) - 1n;

/** Checks a value the C API takes as `uint32_t`. */
export function asUint32(value: number, what: string): number {
  if (!Number.isInteger(value) || value < 0 || value > UINT32_MAX) {
    throw new MaplibreError(
      "invalidInput",
      `${what} must be an integer between 0 and ${UINT32_MAX}, not ${value}`,
    );
  }
  return value;
}

/** Checks a value the C API takes as `int32_t`. */
export function asInt32(value: number, what: string): number {
  if (!Number.isInteger(value) || value < -0x8000_0000 || value > 0x7fff_ffff) {
    throw new MaplibreError(
      "invalidInput",
      `${what} must be a 32-bit signed integer, not ${value}`,
    );
  }
  return value;
}

/** Checks a value the C API takes as `uint64_t`. */
export function asUint64(value: bigint, what: string): bigint {
  if (value < 0n || value > UINT64_MAX) {
    throw new MaplibreError(
      "invalidInput",
      `${what} must be an unsigned 64-bit integer, not ${value}`,
    );
  }
  return value;
}

/** Checks a value the C API takes as `int64_t`. */
export function asInt64(value: bigint, what: string): bigint {
  if (value < INT64_MIN || value > INT64_MAX) {
    throw new MaplibreError(
      "invalidInput",
      `${what} must be a signed 64-bit integer, not ${value}`,
    );
  }
  return value;
}

/**
 * Checks a raw enum value.
 *
 * An unknown value is passed through for the C API to reject, because the
 * binding does not duplicate native enum validation. A value no `uint32_t` can
 * hold is a different thing: it would arrive as some other enum member.
 */
export function asRawEnum(value: number, what: string): number {
  return asUint32(value, what);
}
