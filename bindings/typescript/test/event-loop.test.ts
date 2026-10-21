import assert from "node:assert/strict";

import { test } from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";

test(
  "a script that awaits a style-loaded event and closes its runtimes exits on its own",
  async () => {
    const result = await capabilities.childProcess!(
      new URL("./fixtures/clean-exit.ts", import.meta.url),
    );
    assert.equal(result.code, 0, result.stderr);
    assert.match(result.stdout, /style loaded and runtime closed/);
  },
  { needs: ["childProcess"] },
);

test(
  "a runtime left open does not keep the process alive",
  async () => {
    const result = await capabilities.childProcess!(
      new URL("./fixtures/left-open.ts", import.meta.url),
    );
    assert.equal(result.code, 0, result.stderr);
    assert.match(result.stdout, /returning with the runtime open/);
  },
  { needs: ["childProcess"] },
);
