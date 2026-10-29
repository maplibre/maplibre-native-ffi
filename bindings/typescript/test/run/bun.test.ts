// The Bun entry: one process, every case.
//
//   bun test --timeout 300000 test/run/bun.test.ts

import { test } from "bun:test";

import "../suite.ts";
import { execute, scheduled } from "../support/harness.ts";

for (const entry of scheduled()) {
  test(entry.name, () => execute(entry));
}
