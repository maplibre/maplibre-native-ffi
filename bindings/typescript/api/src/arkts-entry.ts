/**
 * What the ArkTS conformance application imports.
 *
 * ArkTS resolves modules through its own build rather than through
 * node_modules, so the suite and the public API reach the device as one bundle.
 * The payload's addon stays outside it: the runtime resolves a native module by
 * library name, not by path.
 */

// Installed before anything below can encode a string.
import "./internal/arkts-globals.ts";
import type { NodeApiAddon } from "./internal/node-transport.ts";
import { nodeApiTransport } from "./internal/node-transport.ts";
import { Maplibre } from "./maplibre.ts";
// The specifier is the packed library's own name, which the bundler is told to
// leave alone and the ArkTS runtime resolves against the application's libs.
// @ts-expect-error the ArkTS runtime resolves this, and no type describes it
import arktsAddon from "libmaplibre-native-ffi.so";

export {
  CONFORMANCE,
  conformanceCases,
  coveredSpecCases,
} from "./conformance/index.ts";
export type {
  CaseContext,
  ConformanceCase,
  ConformanceGroup,
  Expect,
} from "./conformance/index.ts";
export * from "./index.ts";

/**
 * Loads the addon the way ArkTS resolves a native module: by library name.
 *
 * `Maplibre.load()` discovers a payload by importing the package that carries
 * it, which is how every runtime that resolves through node_modules finds one.
 * ArkTS resolves neither packages nor paths, only the libraries packed into the
 * application, so the ArkTS payload is named here instead of discovered.
 */
export function loadArkTs(): Maplibre {
  return Maplibre.fromTransport(nodeApiTransport(arktsAddon as NodeApiAddon));
}
