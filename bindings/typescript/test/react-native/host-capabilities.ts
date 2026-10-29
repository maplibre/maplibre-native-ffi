import type { Capabilities } from "../support/host-capabilities.ts";

const globals = globalThis as typeof globalThis & {
  __mlnTestGraphics: unknown;
  __mlnCollect: () => void;
};

export const capabilities: Capabilities = {
  host: "react-native",
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
  testGraphics: () => globals.__mlnTestGraphics,
  unrefTimer: () => {},
};
