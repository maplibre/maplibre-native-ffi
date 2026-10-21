// The child side of a spawned @ohos.worker realm, for the ArkTS bundle. The
// first message is the realm's data; the port takes it as soon as this module
// loads, so it is kept until the realm asks for it.

import worker from "@ohos.worker";

const port = worker.workerPort;
const data = new Promise<unknown>((resolve) => {
  port.onmessage = (event) => {
    port.onmessage = null;
    resolve(event.data);
  };
});

/** Resolves with the data that the parent sent when it spawned this realm. */
export function realmData<T>(): Promise<T> {
  return data as Promise<T>;
}

export function postToParent(message: unknown): void {
  port.postMessage(message);
}
