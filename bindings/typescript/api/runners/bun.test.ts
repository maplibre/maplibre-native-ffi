/**
 * The conformance suite, registered in Bun's test framework.
 *
 * Bun implements Node-API, so it loads the same compiled addon Node does. The
 * cases come from the shared suite, so a failure here is a difference between
 * the runtimes rather than between two suites.
 */

import {
  CONFORMANCE,
  type Expect,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";
import { describe, expect, it } from "bun:test";

const maplibre = await Maplibre.load();

/** Offline work needs a database; Bun implements Node's filesystem API. */
async function cacheDirectory(): Promise<string> {
  const { mkdtemp } = await import("node:fs/promises");
  const { tmpdir } = await import("node:os");
  const { join } = await import("node:path");
  return mkdtemp(join(tmpdir(), "maplibre-conformance-"));
}

const assertions: Expect = {
  equal(actual, expected, what) {
    expect(actual, what).toEqual(expected);
  },
  notEqual(actual, unexpected, what) {
    expect(actual, what).not.toEqual(unexpected);
  },
  ok(actual, what) {
    expect(actual, what).toBe(true);
  },
  closeTo(actual, expected, digits, what) {
    // Bun and Deno spell an approximate comparison differently, so the suite
    // gets the same rule from arithmetic instead.
    const tolerance = 10 ** -digits / 2;
    expect(Math.abs(actual - expected) <= tolerance, what).toBe(true);
  },
  defined(actual, what) {
    expect(actual === undefined || actual === null, what).toBe(false);
    return actual as NonNullable<typeof actual>;
  },
  absent(actual, what) {
    expect(actual === undefined, what).toBe(true);
  },
  contains(haystack, needle, what) {
    expect(haystack.includes(needle), what).toBe(true);
  },
  throws(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown instanceof MaplibreError, what).toBe(true);
    return thrown as MaplibreError;
  },
  fail(what): never {
    throw new Error(what);
  },
};

for (const group of CONFORMANCE) {
  describe(group.name, () => {
    for (const entry of group.cases) {
      it(entry.name, async () => {
        await entry.run({ maplibre, expect: assertions, cacheDirectory });
      });
    }
  });
}
