// The Node entry: one process, every case.
//
//   node --expose-gc --test --test-isolation=none test/run/node.ts

import { test } from "node:test";

import "../suite.ts";
import { execute, scheduled } from "../support/harness.ts";

for (const entry of scheduled()) {
  void test(entry.name, () => execute(entry));
}
