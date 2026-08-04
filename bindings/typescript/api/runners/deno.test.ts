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
  ok(actual, what) {
    if (!actual) {
      throw new Error(`${what}: expected true`);
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
