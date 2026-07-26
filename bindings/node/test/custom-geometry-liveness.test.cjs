"use strict";

const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");
const test = require("node:test");

test("retained custom-geometry callbacks do not keep Node alive", () => {
  const nativeAddonPath = require.resolve("../index.js");
  const result = spawnSync(
    process.execPath,
    [
      "-e",
      `
        const native = require(${JSON.stringify(nativeAddonPath)});
        native.nativeTestRetainCustomGeometryCallbacks(() => {}, () => {});
      `,
    ],
    {
      encoding: "utf8",
      timeout: 5_000,
    },
  );
  assert.equal(
    result.error,
    undefined,
    result.error?.message || result.stderr || "child process failed",
  );
  assert.equal(result.signal, null, result.stderr);
  assert.equal(result.status, 0, result.stderr);
});
