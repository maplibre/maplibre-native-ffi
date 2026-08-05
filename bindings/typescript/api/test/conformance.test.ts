/**
 * The conformance suite, registered in vitest.
 *
 * The cases live in `src/conformance/` so Bun, Deno, and the WebAssembly
 * transport run the same tree. A case that passes here and fails there is a
 * difference between the runtimes rather than between two suites.
 */

import { openHeadlessOpenGl } from "../runners/headless-opengl.ts";
import { startHttpOrigin } from "../runners/http-origin.ts";
import {
  type Capability,
  CONFORMANCE,
  type Expect,
  groupsFor,
} from "../src/conformance/index.ts";
import { Maplibre, MaplibreError } from "../src/index.ts";
import type { NativePointer, OpenGlContext } from "../src/render.ts";
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
  describe(group.name, () => {
    for (const entry of group.cases) {
      it(entry.name, async () => {
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
  });
}
