// Statuses and diagnostics: a failed call reports its own, and an addon built
// from other headers fails to load.

import assert from "node:assert/strict";

import {
  AbiVersionMismatchError,
  MaplibreError,
  Status,
} from "../src/index.ts";
import { type Addon, bindAddon } from "../src/internal/addon.ts";
import { openRuntime } from "./support/fixture.ts";
import { test } from "./support/harness.ts";

test("a failed call carries its status, an unknown status code, and its own diagnostic", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();

  assert.throws(
    () => fixture.runtime.setEventMask(1n << 40n),
    (error) =>
      error instanceof MaplibreError &&
      error.status === Status.invalidArgument &&
      error.statusName === "invalidArgument" &&
      error.diagnostic === "mask contains unknown bits" &&
      error.message === "invalidArgument: mask contains unknown bits",
  );
  // An asynchronous failure carries the diagnostic of its own completion.
  await assert.rejects(
    map.getImageSourceCoordinates("missing"),
    (error) =>
      error instanceof MaplibreError &&
      error.status === Status.notFound &&
      error.diagnostic === "source does not exist",
  );

  // A status this binding does not know keeps its code.
  const unknown = new MaplibreError(-1234, "from a later C API");
  assert.equal(unknown.status, -1234);
  assert.equal(unknown.statusName, "unknown");
  assert.equal(unknown.diagnostic, "from a later C API");
});

test("an addon generated from other headers fails to load before any other call", () => {
  // The addon has abiDigest alone, so any other call would throw a TypeError
  // instead of the digest mismatch.
  const stub = { abiDigest: () => "0".repeat(16) } as unknown as Addon;
  assert.throws(
    () =>
      bindAddon(stub, {}, () => {
        throw new Error("the realm was installed");
      }),
    (error) =>
      error instanceof AbiVersionMismatchError &&
      error instanceof MaplibreError &&
      error.status === Status.unsupported &&
      /ABI digest 0000000000000000/.test(error.diagnostic),
  );
});
