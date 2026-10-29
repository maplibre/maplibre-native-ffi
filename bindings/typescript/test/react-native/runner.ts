import "../../src/react-native.ts";
import { RuntimeHandle, setLogCallback } from "../../src/react-native.ts";
import { styleRoute } from "../support/fixture.ts";
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
import { bounded, execute, scheduled } from "../support/harness.ts";

const live: object[] = [];

/** Reload with live owners, callback registrations and an unanswered request. */
export async function leaveLiveWork(): Promise<void> {
  setLogCallback(() => {});
  const runtime = await RuntimeHandle.create();
  await runtime.setEventHandler(() => {});
  let arrived!: () => void;
  const pending = new Promise<void>((resolve) => {
    arrived = resolve;
  });
  await runtime.setResourceProvider({
    routes: [styleRoute("reload://style.json")],
    callback: (_request, handle) => {
      live.push(handle);
      arrived();
    },
  });
  const { map, ready } = runtime.createMap({
    initialExtent: { width: 32, height: 32, scaleFactor: 1 },
  });
  live.push(runtime, map);
  await ready;
  await map.setStyleUrl("reload://style.json");
  await bounded(pending, "the request abandoned at reload");
  console.log("MlnReload live native work");
}

export async function runSuite(): Promise<void> {
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
}
