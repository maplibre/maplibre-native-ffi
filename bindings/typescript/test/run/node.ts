// The Node entry: one process, every case.
//
//   node --expose-gc --test --test-isolation=none test/run/node.ts

import { test } from "node:test";

import "../callbacks.test.ts";
import "../completions.test.ts";
import "../errors.test.ts";
import "../event-loop.test.ts";
import "../lifecycle.test.ts";
import "../log.test.ts";
import "../platform.test.ts";
import "../realms.test.ts";
import "../render.test.ts";
import "../resource-provider.test.ts";
import "../values.test.ts";
import { execute, scheduled } from "../support/harness.ts";

for (const entry of scheduled()) {
  void test(entry.name, () => execute(entry));
}
