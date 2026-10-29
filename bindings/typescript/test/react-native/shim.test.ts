import assert from "node:assert/strict";

import { test } from "../support/harness.ts";

const globals = globalThis as typeof globalThis & {
  __mlnShimTest: {
    signed(value: bigint): bigint;
    unsigned(value: bigint): bigint;
    pending(value: object): never;
    call(callback: () => unknown): unknown;
    bytes(value: unknown): number[];
    createBytes(): Uint8Array;
    external(value: unknown): boolean;
    token: object;
  };
  __mlnForeignState: object;
};
const api = globals.__mlnShimTest;

test("JSI BigInt conversions preserve bounds", () => {
  for (const value of [0n, 1n, -1n, -(2n ** 63n), 2n ** 63n - 1n]) {
    assert.equal(api.signed(value), value);
  }
  for (const value of [0n, 1n, 2n ** 63n, 2n ** 64n - 1n]) {
    assert.equal(api.unsigned(value), value);
  }
  assert.throws(() => api.signed(2n ** 63n), RangeError);
  assert.throws(() => api.unsigned(-1n), RangeError);
});

test("JSI exceptions survive C calls and nested callbacks", () => {
  const sentinel = {};
  assert.throws(
    () =>
      api.pending({
        get throwing() {
          throw sentinel;
        },
      }),
    (error) => error === sentinel,
  );
  assert.throws(
    () =>
      api.call(() => {
        throw sentinel;
      }),
    (error) => error === sentinel,
  );
  assert.ok(api.call(() => api.unsigned(2n ** 64n - 1n)) === 2n ** 64n - 1n);
});

test("JSI typed arrays use intrinsic storage and reject impostors", () => {
  const sentinel = {};
  const bytes = new Uint8Array([10, 20, 30, 40]).subarray(1, 3);
  for (const name of ["buffer", "byteOffset", "byteLength", "length"]) {
    Object.defineProperty(bytes, name, {
      get() {
        throw sentinel;
      },
    });
  }
  Object.defineProperty(bytes, Symbol.toStringTag, { value: "Float64Array" });
  assert.deepEqual(api.bytes(bytes), [1, 2, 1, 20]);
  assert.deepEqual(api.bytes(new Uint8Array(0)), [1, 0, 0, -1]);
  assert.throws(
    () =>
      api.bytes({
        buffer: new ArrayBuffer(100),
        length: 100,
        byteOffset: 0,
        byteLength: 100,
        [Symbol.toStringTag]: "Uint8Array",
      }),
    TypeError,
  );
  assert.throws(() => api.bytes(new Proxy(new Uint8Array(1), {})), TypeError);
  const created = api.createBytes();
  assert.ok(created instanceof Uint8Array && created.byteOffset === 2);
  assert.ok(created.length === 3 && created[0] === 2 && created[2] === 4);
});

test("JSI externals reject foreign native state", () => {
  assert.ok(api.external(api.token));
  assert.throws(() => api.external({}), TypeError);
  assert.throws(() => api.external(globals.__mlnForeignState), TypeError);
});
