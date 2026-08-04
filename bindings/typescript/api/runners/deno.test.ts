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
  groupsFor,
} from "../src/index.ts";
import { startHttpOrigin } from "./http-origin.ts";

const maplibre = await Maplibre.load();

/** Loads the built package the way a Deno consumer would. */
async function loadPackage(
  format: "esm" | "cjs",
): Promise<typeof import("../src/index.ts")> {
  const distribution = new URL("../dist/", import.meta.url);
  const specifier = new URL(
    format === "esm" ? "index.mjs" : "index.cjs",
    distribution,
  );
  return (await import(specifier.href)) as never;
}

/** Offline work needs a database, which Deno creates through its own API. */
async function cacheDirectory(): Promise<string> {
  return Deno.makeTempDir({ prefix: "maplibre-conformance-" });
}

/**
 * No graphics context exists here.
 *
 * A case that needs one names the capability and this runner leaves it out, so
 * reaching this is a registration mistake rather than something to work around.
 */
function hostTexture(): never {
  throw new Error("this runtime has no graphics context to make a texture in");
}

function renderContext(): never {
  throw new Error("this runtime has no graphics context to render through");
}

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
  throwsAny(body, what) {
    let thrown: unknown;
    try {
      body();
    } catch (error) {
      thrown = error;
    }
    if (!(thrown instanceof Error)) {
      throw new Error(`${what}: expected a failure`);
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

/** What this runner loads, so a case restricted to another one is skipped. */
const TRANSPORT = "node-api";

/**
 * What this runner can offer beyond the transport.
 *
 * No graphics context exists here, so the render-session cases are left to a
 * host that has one.
 */
const CAPABILITIES = [
  "packageResolution",
  "httpHeaderTransforms",
  "httpOrigin",
] as const;

for (const group of groupsFor(CONFORMANCE, {
  transport: TRANSPORT,
  capabilities: CAPABILITIES,
})) {
  for (const entry of group.cases) {
    Deno.test(`${group.name} > ${entry.name}`, async () => {
      await entry.run({
        maplibre,
        expect: assertions,
        cacheDirectory,
        renderContext,
        hostTexture,
        loadPackage,
        httpOrigin: startHttpOrigin,
      });
    });
  }
}
