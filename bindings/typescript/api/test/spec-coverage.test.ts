/**
 * Every specification case is accounted for.
 *
 * The suite tags a case with what it proves, and `UNCLAIMED` names the rest
 * with a reason. Counting only what is claimed would let a gap sit unnoticed,
 * so this fails when an identifier is neither, and when one is both — an entry
 * that a case has since covered has to be removed rather than left to rot.
 */

import { coveredSpecCases } from "../src/conformance/index.ts";
import { UNCLAIMED } from "../src/conformance/unclaimed.ts";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { expect, it } from "vitest";

const specification = readFileSync(
  fileURLToPath(
    new URL(
      "../../../../docs/src/content/docs/development/binding-specification.md",
      import.meta.url,
    ),
  ),
  "utf8",
);

const declared = [...specification.matchAll(/^\|\s*(BND-\d+)\s*\|/gm)].map(
  (match) => match[1]!,
);

it("declares specification cases this suite can account for", () => {
  expect(declared.length).toBeGreaterThan(0);
  expect(new Set(declared).size).toBe(declared.length);
});

it("proves or explains every specification case", () => {
  const covered = new Set(coveredSpecCases());
  const unclaimed = new Set(UNCLAIMED.map((entry) => entry.id));

  expect(
    declared.filter((id) => !covered.has(id) && !unclaimed.has(id)),
    "specification cases neither proven nor listed as unclaimed",
  ).toEqual([]);
  expect(
    declared.filter((id) => covered.has(id) && unclaimed.has(id)),
    "specification cases both proven and listed as unclaimed",
  ).toEqual([]);
  expect(
    [...covered, ...unclaimed].filter((id) => !declared.includes(id)),
    "identifiers the specification does not declare",
  ).toEqual([]);
});

it("gives every unclaimed case a reason", () => {
  expect(
    UNCLAIMED.filter((entry) => entry.note.trim().length === 0).map(
      (entry) => entry.id,
    ),
    "unclaimed cases with no reason",
  ).toEqual([]);
  expect(new Set(UNCLAIMED.map((entry) => entry.id)).size).toBe(
    UNCLAIMED.length,
  );
});
