// Statuses and diagnostics: a failed call reports its own, and an addon with
// another C ABI version fails to load.

import assert from "node:assert/strict";

import {
  AbiVersionMismatchError,
  MaplibreError,
  Status,
} from "../src/index.ts";
import {
  type Addon,
  bindAddon,
  EXPECTED_C_ABI_VERSION,
} from "../src/internal/addon.ts";
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

test("an addon with another C ABI version fails to load before any other call", () => {
  // The addon has mln_c_version alone, so any other call would throw a
  // TypeError instead of the version mismatch.
  const stub = {
    mln_c_version: () => EXPECTED_C_ABI_VERSION + 1,
  } as unknown as Addon;
  assert.throws(
    () =>
      bindAddon(stub, {}, () => {
        throw new Error("the realm was installed");
      }),
    (error) =>
      error instanceof AbiVersionMismatchError &&
      error instanceof MaplibreError &&
      error.status === Status.unsupported &&
      error.diagnostic ===
        `unsupported MapLibre Native C ABI version ${EXPECTED_C_ABI_VERSION + 1}; expected ${EXPECTED_C_ABI_VERSION}`,
  );
});
