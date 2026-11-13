import type { Capabilities } from "../support/host-capabilities.ts";

const globals = globalThis as typeof globalThis & {
  __mlnTestGraphics: unknown;
  __mlnCollect: () => void;
  __mlnTestBackend: string | undefined;
};

export const capabilities: Capabilities = {
  host: "hermes",
  unavailable: [
    "httpOrigin",
    "realmSpawn",
    "childProcess",
    "processExit",
    "bufferTransfer",
    "disposers",
    "beforeLoad",
  ],
  forcedGc: () => globals.__mlnCollect(),
  borrowedTextures: true,
  testGraphics: () => globals.__mlnTestGraphics,
  // The host reads MLN_FFI_TEST_BACKEND from its environment.
  testBackend: () => globals.__mlnTestBackend,
  unrefTimer: () => {},
};
