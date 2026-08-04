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

const assertions: Expect = {
  equal(actual, expected) {
    expect(actual).toEqual(expected);
  },
  ok(actual) {
    expect(actual).toBe(true);
  },
  throws(body) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown).toBeInstanceOf(MaplibreError);
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
