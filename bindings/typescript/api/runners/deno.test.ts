/**
 * The conformance suite, registered in Deno's test framework.
 *
 * Deno implements Node-API, so it loads the same compiled addon Node does. It
 * needs a local node_modules directory to resolve the runtime payload package,
 * and the FFI permission to load a native addon at all.
 */

import {
  CONFORMANCE,
  type Expect,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";

const maplibre = await Maplibre.load();

const assertions: Expect = {
  equal(actual, expected, what) {
    const left = JSON.stringify(actual, replacer);
    const right = JSON.stringify(expected, replacer);
    if (left !== right) {
      throw new Error(`${what}: expected ${right}, got ${left}`);
    }
  },
  notEqual(actual, unexpected, what) {
    if (
      JSON.stringify(actual, replacer) === JSON.stringify(unexpected, replacer)
    ) {
      throw new Error(
        `${what}: expected something other than ${String(actual)}`,
      );
    }
  },
  ok(actual, what) {
    if (!actual) {
      throw new Error(`${what}: expected true`);
    }
  },
  closeTo(actual, expected, digits, what) {
    const tolerance = 10 ** -digits / 2;
    if (!(Math.abs(actual - expected) <= tolerance)) {
      throw new Error(`${what}: expected ${expected}, got ${actual}`);
    }
  },
  defined(actual, what) {
    if (actual === undefined || actual === null) {
      throw new Error(`${what}: expected a value`);
    }
    return actual;
  },
  absent(actual, what) {
    if (actual !== undefined) {
      throw new Error(`${what}: expected nothing, got ${String(actual)}`);
    }
  },
  contains(haystack, needle, what) {
    if (!haystack.includes(needle)) {
      throw new Error(`${what}: ${JSON.stringify(haystack)} lacks ${needle}`);
    }
  },
  throws(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    if (!(thrown instanceof MaplibreError)) {
      throw new Error(
        `${what}: expected a MaplibreError, got ${String(thrown)}`,
      );
    }
    return thrown;
  },
  fail(what): never {
    throw new Error(what);
  },
};

/** `bigint` has no JSON form, and these comparisons are for small values. */
function replacer(_key: string, value: unknown): unknown {
  return typeof value === "bigint" ? value.toString() : value;
}

for (const group of CONFORMANCE) {
  for (const entry of group.cases) {
    Deno.test(`${group.name} > ${entry.name}`, async () => {
      await entry.run({ maplibre, expect: assertions });
    });
  }
}
