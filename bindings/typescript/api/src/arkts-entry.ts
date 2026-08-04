/**
 * What the ArkTS conformance application imports.
 *
 * ArkTS resolves modules through its own build rather than through
 * node_modules, so the suite and the public API reach the device as one bundle.
 * The payload's addon stays outside it: the runtime resolves a native module by
 * library name, not by path.
 */

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
