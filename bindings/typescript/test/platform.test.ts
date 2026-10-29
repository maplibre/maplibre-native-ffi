// Loading, collection, and process exit: how the binding meets its host.

import assert from "node:assert/strict";
import { cpSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

import {
  cVersion,
  clearLogCallback,
  RuntimeEventType,
  RuntimeHandle,
} from "../src/index.ts";
import { mapOptions, openRuntime, styleRoute } from "./support/fixture.ts";
import {
  collectGarbageUntil,
  teardown,
  test,
  watchCollection,
} from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";
import { captureLeaks, runtimeIsLive } from "./support/internals.ts";
import {
  nextMessage,
  spawnRealm,
  startRealm,
  terminateRealm,
} from "./support/realms.ts";

// A host that starts processes loads the packages that the test task packed
// and installed into a consumer project, with no addon override. ArkTS starts
// no process: a fresh realm loads the OHPM package that the test task packed,
// through its bundled ArkTS entry and the addon in its libs, which the
// application carries.
const startsProcesses = !capabilities.unavailable.includes("childProcess");

/** Runs the consumer scripts of the installed project under this runtime. */
async function loadFromInstalledPackages() {
  const consumer = process.env.MAPLIBRE_NATIVE_FFI_TEST_CONSUMER;
  if (!consumer) {
    throw new Error(
      "MAPLIBRE_NATIVE_FFI_TEST_CONSUMER names the installed consumer project",
    );
  }
  // Deno imports the facade through an npm specifier. Node and Bun import it
  // as an ES module, and require() it from CommonJS.
  const scripts =
    capabilities.host === "deno" ? ["deno.mjs"] : ["esm.mts", "cjs.cjs"];
  for (const script of scripts) {
    const result = await capabilities.childProcess!(join(consumer, script), {
      env: { MAPLIBRE_NATIVE_FFI_ADDON: undefined },
    });
    assert.equal(result.code, 0, `${script}: ${result.stderr}`);
    assert.match(result.stdout, /^loaded C API version \d+$/m);
  }
}

/** Loads the packed OHPM package in a fresh realm of the application. */
async function loadFromApplication() {
  const realm = spawnRealm({ scenario: "load" });
  const { version } = await nextMessage<{ version: number }>(
    realm,
    "the realm's runtime",
  );
  assert.equal(version, cVersion());
}

test(
  "loads the addon from its installed package",
  startsProcesses ? loadFromInstalledPackages : loadFromApplication,
  { needs: startsProcesses ? ["childProcess"] : ["realmSpawn"] },
);

test(
  "a second copy of the binding in one realm refuses to load",
  async () => {
    // Two copies of the facade, as a duplicated package or two bundles bring,
    // load the same addon module. The second copy must fail before it can
    // take the first copy's completions and callbacks.
    const root = mkdtempSync(join(tmpdir(), "maplibre-native-ffi-"));
    teardown(() => rmSync(root, { recursive: true, force: true }));
    const source = fileURLToPath(new URL("../src", import.meta.url).href);
    for (const copy of ["first", "second"]) {
      cpSync(source, join(root, copy, "src"), { recursive: true });
    }
    writeFileSync(
      join(root, "package.json"),
      JSON.stringify({ type: "module" }),
    );
    writeFileSync(
      join(root, "load.ts"),
      [
        'import { RuntimeHandle } from "./first/src/index.ts";',
        "const runtime = await RuntimeHandle.create();",
        "try {",
        '  await import("./second/src/index.ts");',
        '  console.log("the second copy loaded");',
        "} catch (error) {",
        "  console.log(`the second copy failed: ${(error as Error).message}`);",
        "}",
        "const { map, ready } = runtime.createMap({",
        "  initialExtent: { width: 64, height: 64, scaleFactor: 1 },",
        "});",
        "await ready;",
        "await map.close();",
        "await runtime.close();",
        'console.log("the first copy still works");',
      ].join("\n"),
    );
    const result = await capabilities.childProcess!(join(root, "load.ts"));
    assert.equal(result.code, 0, result.stderr);
    assert.match(
      result.stdout,
      /^the second copy failed: another copy of the binding already uses the addon in this realm/m,
    );
    assert.match(result.stdout, /^the first copy still works$/m);
  },
  { needs: ["childProcess"] },
);

test(
  "loading refuses no backend package and refuses several",
  async () => {
    // A copy of the sources resolves backend packages from a node_modules of
    // its own, so the case controls which ones are installed.
    const root = mkdtempSync(join(tmpdir(), "maplibre-native-ffi-"));
    teardown(() => rmSync(root, { recursive: true, force: true }));
    const source = fileURLToPath(new URL("../src", import.meta.url).href);
    cpSync(source, join(root, "src"), { recursive: true });
    writeFileSync(
      join(root, "package.json"),
      JSON.stringify({ type: "module" }),
    );
    writeFileSync(
      join(root, "load.ts"),
      [
        'import { loadAddon } from "./src/internal/loader.ts";',
        "try {",
        "  loadAddon();",
        '  console.log("loaded");',
        "} catch (error) {",
        "  console.log((error as Error).message);",
        "}",
      ].join("\n"),
    );
    async function load(): Promise<string> {
      const result = await capabilities.childProcess!(join(root, "load.ts"), {
        env: { MAPLIBRE_NATIVE_FFI_ADDON: undefined },
      });
      assert.equal(result.code, 0, result.stderr);
      return result.stdout;
    }

    assert.match(
      await load(),
      /^no MapLibre Native backend package is installed; install one of /m,
    );
    for (const backend of ["opengl", "vulkan"]) {
      const name = `@maplibre/native-ffi-${backend}`;
      const directory = join(root, "node_modules", name);
      mkdirSync(directory, { recursive: true });
      writeFileSync(join(directory, "package.json"), JSON.stringify({ name }));
    }
    assert.match(
      await load(),
      /^@maplibre\/native-ffi-opengl and @maplibre\/native-ffi-vulkan are installed; install exactly one backend package$/m,
    );
  },
  { needs: ["childProcess"] },
);

/** Leaves a runtime and a map open for the collector. */
async function abandonRuntimeWithMap() {
  const runtime = await RuntimeHandle.create();
  const { map, ready } = runtime.createMap(mapOptions);
  await ready;
  return {
    runtime: runtime.id,
    map: map.id,
    collected: [watchCollection(runtime), watchCollection(map)],
  };
}

test(
  "an abandoned runtime with a live map is disposed and reported when collected",
  async () => {
    const leaks = captureLeaks();
    const abandoned = await abandonRuntimeWithMap();
    const reported = (name: string, id: bigint) =>
      leaks.some((line) => line.includes(`${name} 0x${id.toString(16)}`));
    // Owner-token finalizers dispose the runtime and its map with no
    // JavaScript on the stack; the leak reports follow on the event loop.
    await collectGarbageUntil(
      () =>
        abandoned.collected.every((collected) => collected()) &&
        reported("RuntimeHandle", abandoned.runtime) &&
        reported("MapHandle", abandoned.map),
      "the abandoned runtime and its map",
    );
    assert.equal(runtimeIsLive(abandoned.runtime), false);
  },
  { needs: ["forcedGc"] },
);

/** A runtime whose event handler names it, and nothing else does. */
async function runtimeHeldByItsEventHandler() {
  const runtime: RuntimeHandle = await RuntimeHandle.create();
  await runtime.setEventHandler(() => void runtime.id);
  return { id: runtime.id, collected: watchCollection(runtime) };
}

/** A runtime whose provider names it, and nothing else does. */
async function runtimeHeldByItsProvider() {
  const runtime = await RuntimeHandle.create();
  await runtime.setResourceProvider({
    routes: [styleRoute("capture://style.json")],
    callback(_request, handle) {
      void runtime.id;
      handle.close();
    },
  });
  return { id: runtime.id, collected: watchCollection(runtime) };
}

test(
  "callback registrations do not keep their owners reachable",
  async () => {
    captureLeaks();
    const runtimes = [
      await runtimeHeldByItsEventHandler(),
      await runtimeHeldByItsProvider(),
    ];
    // Each runtime and the callback that captures it form a cycle through
    // the registration, which the collector reclaims as a whole.
    await collectGarbageUntil(
      () => runtimes.every((runtime) => runtime.collected()),
      "runtimes whose callbacks capture them",
    );
    await collectGarbageUntil(
      () => runtimes.every((runtime) => !runtimeIsLive(runtime.id)),
      "the disposal of those runtimes",
    );
  },
  { needs: ["forcedGc"] },
);

/**
 * Ends a child process with process.exit(0), or Deno.exit(0) on Deno, while
 * native work is live.
 */
async function exitProcessWithLiveWork() {
  const result = await capabilities.childProcess!(
    new URL("./fixtures/process-exit.ts", import.meta.url),
    { milliseconds: 120_000 },
  );
  assert.equal(result.code, 0, result.stderr);
  assert.match(result.stdout, /exiting with live native work/);
  assert.match(result.stdout, /the session was abandoned at exit/);
  // A Node-API fatal error or a core panic reports itself on standard error.
  assert.doesNotMatch(result.stderr, /FATAL ERROR|panicked/);
}

/**
 * Terminates a worker realm while native work is live. ArkTS has no exit
 * event, and an application process ends without running exit handlers, so
 * a worker's teardown is the end that the binding meets there.
 */
async function terminateRealmWithLiveWork() {
  const realm = startRealm({ scenario: "liveNativeWork" });
  await nextMessage(realm, "the realm's live native work");
  // ArkTS resolves the termination once the realm's environment has ended.
  await terminateRealm(realm, "the realm's termination");
  // The log callback is process-global, so the ended realm's registration
  // stays until another realm replaces or clears it.
  clearLogCallback();
  // This realm keeps receiving after the other one ended mid-frame.
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
}

test(
  "process exit with a mid-frame session, a pending provider request, and live registrations exits cleanly",
  startsProcesses ? exitProcessWithLiveWork : terminateRealmWithLiveWork,
  { needs: startsProcesses ? ["childProcess"] : ["realmSpawn"] },
);
