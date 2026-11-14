// The child side of a spawned @ohos.worker realm, for the ArkTS bundle. The
// first message is the realm's data and the spawning realm's `process.env`,
// which this realm's own `process` shim starts without. The port takes it as
// soon as this module loads, so it is kept until the realm asks for it.

import worker from "@ohos.worker";

import process from "./node/process.ts";

const port = worker.workerPort;
const data = new Promise<unknown>((resolve) => {
  port.onmessage = (event) => {
    port.onmessage = null;
    const message = event.data as {
      env: Record<string, string | undefined>;
      data: unknown;
    };
    Object.assign(process.env, message.env);
    resolve(message.data);
  };
});

/** Resolves with the data that the parent sent when it spawned this realm. */
export function realmData<T>(): Promise<T> {
  return data as Promise<T>;
}

export function postToParent(message: unknown): void {
  port.postMessage(message);
}
