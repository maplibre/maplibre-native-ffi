// How the suite reaches each host capability on the Hermes test host, which
// the Hermes bundle uses in place of test/support/host-capabilities.ts.

import type { Capabilities } from "../support/host-capabilities.ts";
import process from "./node/process.ts";

declare const gc: () => void;

// Hermes has no ES2024 ArrayBuffer.prototype.transfer, so the target decides
// that the case that detaches bytes does not run. A Hermes that gains it fails
// the run here, so the case starts running there.
if ("transfer" in ArrayBuffer.prototype) {
  throw new Error(
    "this Hermes has ArrayBuffer.prototype.transfer; declare bufferTransfer",
  );
}

export const capabilities: Capabilities = {
  host: "hermes",
  // The host runs one runtime with no network, process, or worker API of its
  // own.
  unavailable: [
    "httpOrigin",
    "realmSpawn",
    "childProcess",
    "processExit",
    "bufferTransfer",
  ],
  forcedGc: () => gc(),
  testGraphics: () => {
    const path = process.env.MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS;
    if (!path) {
      throw new Error(
        "MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS names the graphics addon",
      );
    }
    return hermesHost.loadAddon(path);
  },
  unrefTimer: (timer) => hermesHost.unrefTimer(timer),
};
