// The child side of a spawned realm: its first message is its data, and it
// posts its results back to the realm that spawned it.

declare const Deno: unknown;

interface Port {
  once(event: "message", listener: (message: unknown) => void): void;
  postMessage(message: unknown): void;
}

interface WebWorkerScope {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage(message: unknown): void;
}

const port: Port | undefined =
  typeof Deno === "undefined"
    ? ((await import("node:worker_threads")).parentPort as Port)
    : undefined;
const scope = globalThis as unknown as WebWorkerScope;

/** Resolves with the data that the parent sent when it spawned this realm. */
export function realmData<T>(): Promise<T> {
  return new Promise((resolve) => {
    if (port !== undefined) {
      port.once("message", (message) => resolve(message as T));
    } else {
      scope.onmessage = (event) => {
        scope.onmessage = null;
        resolve(event.data as T);
      };
    }
  });
}

export function postToParent(message: unknown): void {
  if (port !== undefined) {
    port.postMessage(message);
  } else {
    scope.postMessage(message);
  }
}
