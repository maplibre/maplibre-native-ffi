/**
 * The conformance suite, registered in Deno's test framework.
 *
 * Deno implements Node-API, so it loads the same compiled addon Node does. It
 * needs a local node_modules directory to resolve the runtime payload package,
 * and the FFI permission to load a native addon at all.
 */

import {
  type Capability,
  CONFORMANCE,
  type Expect,
  groupsFor,
} from "../src/conformance/index.ts";
import { Maplibre, MaplibreError } from "../src/index.ts";
import type { NativePointer, OpenGlContext } from "../src/render.ts";
import { openHeadlessOpenGl } from "./headless-opengl.ts";
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
 * The one graphics context this process renders through.
 *
 * A native host has none until it asks a driver for one, and only an OpenGL
 * build can use the EGL context this asks for, so it asks only when the loaded
 * library carries that backend. A host with no driver reports none at all, and
 * the render cases are then left to a host that has one.
 */
const graphics = maplibre.renderBackends.opengl
  ? await openHeadlessOpenGl()
  : undefined;

function renderContext(): OpenGlContext {
  if (graphics === undefined) {
    throw new Error("this runtime has no graphics context to render through");
  }
  return graphics.context;
}

/**
 * A texture this process owns, which a caller-owned target draws into.
 *
 * Made in the context the sessions attach to, so a session that shares with it
 * can see the name, and kept by the host so a case can check the session left
 * it alone.
 */
function hostTexture(
  width: number,
  height: number,
): { texture: number; target: number } {
  if (graphics === undefined) {
    throw new Error(
      "this runtime has no graphics context to make a texture in",
    );
  }
  return graphics.texture(width, height);
}

/**
 * The pbuffer the fixture made, when there is one.
 *
 * A surface session needs somewhere to present, and a headless host has this
 * rather than a window.
 */
function hostSurface(): NativePointer | undefined {
  return graphics?.surface;
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
  // Marked, so a bigint cannot compare equal to the string of the same digits.
  // Every address in this binding crosses as a bigint, so an unmarked one would
  // let a case pass against a value of the wrong type.
  return typeof value === "bigint" ? `${value}n` : value;
}

/** What this runner loads, so a case restricted to another one is skipped. */
const TRANSPORT = "node-api";

/**
 * What this runner can offer beyond the transport.
 *
 * The render-session cases are registered only when a context was made above,
 * which is a property of the host and of the backend the loaded build carries
 * rather than of the transport.
 */
const CAPABILITIES: Capability[] = [
  "packageResolution",
  "httpHeaderTransforms",
  "httpOrigin",
];
if (graphics !== undefined) {
  CAPABILITIES.push("renderContext");
}

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
        hostSurface,
        loadPackage,
        httpOrigin: startHttpOrigin,
      });
    });
  }
}
