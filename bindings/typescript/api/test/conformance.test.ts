/**
 * The conformance suite, registered in vitest.
 *
 * The cases live in `src/conformance/` so Bun, Deno, and the WebAssembly
 * transport run the same tree. A case that passes here and fails there is a
 * difference between the runtimes rather than between two suites.
 */

import {
  CONFORMANCE,
  type Expect,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";
import { describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

/** Offline work needs a database, and Node spells its temp directory this way. */
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
    expect(actual, what).toBeCloseTo(expected, digits);
  },
  defined(actual, what) {
    expect(actual, what).not.toBeUndefined();
    expect(actual, what).not.toBeNull();
    return actual as NonNullable<typeof actual>;
  },
  absent(actual, what) {
    expect(actual, what).toBeUndefined();
  },
  contains(haystack, needle, what) {
    expect(haystack, what).toContain(needle);
  },
  throws(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown, what).toBeInstanceOf(MaplibreError);
    return thrown as MaplibreError;
  },
  fail(what) {
    expect.unreachable(what);
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
