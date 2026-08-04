/**
 * The transport-neutral conformance suite, registered in vitest.
 *
 * The cases themselves live in `src/conformance.ts` so Bun, Deno, and a browser
 * run the same tree. A case that passes here and fails there is a difference
 * between the runtimes rather than between two suites.
 */

import {
  CONFORMANCE,
  type Expect,
  Maplibre,
  MaplibreError,
} from "../src/index.ts";
import { describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

const assertions: Expect = {
  equal(actual, expected, what) {
    expect(actual, what).toEqual(expected);
  },
  ok(actual, what) {
    expect(actual, what).toBe(true);
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
};

for (const group of CONFORMANCE) {
  describe(group.name, () => {
    for (const entry of group.cases) {
      it(entry.name, async () => {
        await entry.run({ maplibre, expect: assertions });
      });
    }
  });
}
