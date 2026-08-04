/**
 * The Node-API payload handshake.
 *
 * The call ABI and the memory model are proved by the shared suite, on both
 * transports. What is left here is the one thing that belongs to this payload:
 * refusing an addon that describes another ABI, which needs a doctored
 * Node-API addon to arrange.
 */

import {
  AbiMismatchError,
  type NodeApiAddon,
  nodeApiTransport,
} from "../src/internal/node-transport.ts";
import { EP } from "../src/raw/entrypoints.ts";
import { ABI_FINGERPRINT } from "../src/raw/fingerprint.ts";
import { describe, expect, it } from "vitest";

const { addon } = (await import("@maplibre/native-ffi-runtime-node")) as {
  addon: NodeApiAddon;
};
const transport = nodeApiTransport(addon);

describe("the ABI handshake", () => {
  it("refuses a payload whose fingerprint differs", () => {
    const doctored: NodeApiAddon = {
      ...addon,
      abiFingerprint: () => "0".repeat(64),
    };
    expect(() => nodeApiTransport(doctored)).toThrow(AbiMismatchError);
  });

  it("refuses a payload built against other headers", () => {
    const doctored: NodeApiAddon = {
      ...addon,
      abiHeaderDigest: () => "0".repeat(64),
    };
    expect(() => nodeApiTransport(doctored)).toThrow(AbiMismatchError);
  });

  it("agrees with the payload it loaded", () => {
    expect(transport.abiFingerprint).toBe(ABI_FINGERPRINT);
    expect(addon.entrypointCount()).toBe(Object.keys(EP).length);
  });
});
