/**
 * The conformance suite, as data.
 *
 * These are the cases that must hold on every runtime and transport. Each
 * runtime's runner registers this same tree in its own framework, so a case
 * that passes under Node and fails under Bun is a real difference between the
 * runtimes rather than between two suites.
 *
 * Behavior a single runtime owns — module formats, the transport's own
 * internals — is tested beside that runtime instead.
 */

import { DOMAIN_GROUPS } from "./domains.ts";
import type { ConformanceCase, ConformanceGroup } from "./harness.ts";
import { LIBRARY_GROUP, RUNTIME_GROUP } from "./runtime.ts";

export type {
  CaseContext,
  ConformanceCase,
  ConformanceGroup,
  Expect,
} from "./harness.ts";

export const CONFORMANCE: readonly ConformanceGroup[] = [
  LIBRARY_GROUP,
  RUNTIME_GROUP,
  ...DOMAIN_GROUPS,
];

/** Every case, flattened, for a runner that wants one list. */
export function conformanceCases(): readonly (ConformanceCase & {
  group: string;
})[] {
  return CONFORMANCE.flatMap((group) =>
    group.cases.map((entry) => ({ ...entry, group: group.name })),
  );
}

/** The specification cases this suite claims to prove. */
export function coveredSpecCases(): readonly string[] {
  const covered = new Set<string>();
  for (const entry of conformanceCases()) {
    for (const id of entry.spec ?? []) {
      covered.add(id);
    }
  }
  return [...covered].sort();
}
