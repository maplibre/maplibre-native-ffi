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
  groupsFor,
} from "../src/index.ts";
import { describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

/** Loads the built package the way a consumer of this runtime would. */
async function loadPackage(
  format: "esm" | "cjs",
): Promise<typeof import("../src/index.ts")> {
  const { fileURLToPath } = await import("node:url");
  const distribution = fileURLToPath(new URL("../dist/", import.meta.url));
  if (format === "esm") {
    return (await import(`${distribution}index.mjs`)) as never;
  }
  const { createRequire } = await import("node:module");
  return createRequire(import.meta.url)(`${distribution}index.cjs`) as never;
}

/** Offline work needs a database, and Node spells its temp directory this way. */
async function cacheDirectory(): Promise<string> {
  const { mkdtemp } = await import("node:fs/promises");
  const { tmpdir } = await import("node:os");
  const { join } = await import("node:path");
  return mkdtemp(join(tmpdir(), "maplibre-conformance-"));
}

/**
 * No graphics context exists here.
 *
 * A case that needs one names the capability and this runner leaves it out, so
 * reaching this is a registration mistake rather than something to work around.
 */
function renderContext(): never {
  throw new Error("this runtime has no graphics context to render through");
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
  throwsAny(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    expect(thrown, what).toBeInstanceOf(Error);
    return thrown as Error;
  },
  fail(what) {
    expect.unreachable(what);
  },
};

/** What this runner loads, so a case restricted to another one is skipped. */
const TRANSPORT = "node-api";

/**
 * What this runner can offer beyond the transport.
 *
 * No graphics context exists here, so the render-session cases are left to a
 * host that has one.
 */
const CAPABILITIES = ["packageResolution"] as const;

for (const group of groupsFor(CONFORMANCE, {
  transport: TRANSPORT,
  capabilities: CAPABILITIES,
})) {
  describe(group.name, () => {
    for (const entry of group.cases) {
      it(entry.name, async () => {
        await entry.run({
          maplibre,
          expect: assertions,
          cacheDirectory,
          renderContext,
          loadPackage,
        });
      });
    }
  });
}
