/**
 * What this package offers a consumer, as opposed to what it contains.
 *
 * The conformance suite used to be exported from the main entry point, so an
 * application that installed the package got the test harness with it. The
 * suite is repository-internal: a runner imports it by path, and the ArkTS
 * bundle re-exports it because that device application is a runner. Nothing
 * here should be reachable from a published entry point.
 *
 * The browser entry point is checked the other way around, because the gap it
 * closes was the opposite one: a page could only reach a WebAssembly runtime
 * through internals, so it has to be reachable from what the package publishes.
 */

import * as api from "../src/index.ts";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { expect, it } from "vitest";

const manifest = JSON.parse(
  readFileSync(
    fileURLToPath(new URL("../package.json", import.meta.url)),
    "utf8",
  ),
) as { exports: Record<string, Record<string, string>> };

it("keeps the conformance harness out of the public API", () => {
  const harness = [
    "CONFORMANCE",
    "conformanceCases",
    "coveredSpecCases",
    "groupsFor",
    "runsHere",
  ];
  expect(
    harness.filter((name) => name in api),
    "test harness names a consumer can import",
  ).toEqual([]);
  // The entry point still has to carry the API itself, so this is not passing
  // by exporting nothing.
  expect(Object.keys(api)).toContain("Maplibre");
});

it("publishes the browser entry point", async () => {
  const browser = manifest.exports["./browser"];
  expect(browser, "the exports map offers a browser entry point").toBeDefined();

  // Loaded from the built distribution rather than from source, because what a
  // page resolves is the build.
  const distribution = fileURLToPath(new URL("../dist/", import.meta.url));
  const built = (await import(`${distribution}browser.mjs`)) as Record<
    string,
    unknown
  >;
  expect(Object.keys(built).sort()).toEqual([
    "createWebGlContext",
    "instantiateWasmPayload",
    "loadBrowser",
  ]);
});
