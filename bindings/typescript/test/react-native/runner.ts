import "../../src/react-native.ts";
import "../suite.ts";
import "./shim.test.ts";
import { execute, scheduled } from "../support/harness.ts";
import { liveNativeWork } from "../support/scenarios.ts";

export async function runSuite(reload: () => void): Promise<void> {
  const failures: string[] = [];
  const cases = scheduled();
  for (const entry of cases) {
    try {
      await execute(entry);
      console.log(`PASS ${entry.name}`);
    } catch (error) {
      const detail = `${entry.name}: ${String(error)}\n${(error as Error)?.stack ?? ""}`;
      failures.push(detail);
      console.error(`FAIL ${detail}`);
    }
  }
  if (failures.length) throw new Error(failures.join("\n"));
  console.log(`MlnSuite ${cases.length} passed`);
  await execute({
    name: "reload with live native work",
    needs: [],
    run: async () => {
      await liveNativeWork();
      reload();
      // The host ends this realm; its teardowns must not close the work first.
      await new Promise<never>(() => {});
    },
  });
}
