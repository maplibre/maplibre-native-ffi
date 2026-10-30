import "./globals.ts";
import { RuntimeHandle } from "@maplibre/native-ffi/hermes";

const host = globalThis as typeof globalThis & {
  __mlnReleaseEarly: () => void;
  __mlnDone: (error?: string) => void;
};
async function run(): Promise<void> {
  host.__mlnReleaseEarly();
  const runtime = await RuntimeHandle.create();
  const { map, ready } = runtime.createMap({
    initialExtent: { width: 32, height: 32, scaleFactor: 1 },
  });
  await ready;
  await map.close();
  await runtime.close();
  host.__mlnDone();
}
void run().catch((error: unknown) =>
  host.__mlnDone(`${String(error)}\n${(error as Error)?.stack ?? ""}`),
);
