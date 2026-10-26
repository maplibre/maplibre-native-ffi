// How the suite reaches each host capability on ArkTS, which the ArkTS bundle
// uses in place of test/support/host-capabilities.ts.

import http from "@ohos.net.http";
import worker from "@ohos.worker";
import testGraphics from "libmaplibre_native_ffi_test_graphics.so";

import type {
  Capabilities,
  Origin,
  OriginRequest,
  Realm,
} from "../support/host-capabilities.ts";
import process from "./node/process.ts";

declare const ArkTools: { forceFullGC(): void };

/** A request to the origin server on the build machine. */
async function control(
  method: string,
  path: string,
  body?: unknown,
): Promise<unknown> {
  const base = process.env.MAPLIBRE_NATIVE_FFI_ORIGIN;
  if (base === undefined) {
    throw new Error("aa test -s origin names the origin server");
  }
  const request = http.createHttp();
  try {
    const response = await request.request(`${base}${path}`, {
      method,
      header: { "content-type": "application/json" },
      expectDataType: http.HttpDataType.STRING,
      ...(body === undefined ? {} : { extraData: JSON.stringify(body) }),
    });
    if (response.responseCode >= 300) {
      throw new Error(`${method} ${path}: HTTP ${response.responseCode}`);
    }
    const text = response.result as string;
    return text === "" ? undefined : (JSON.parse(text) as unknown);
  } finally {
    request.destroy();
  }
}

/**
 * An origin on the build machine, reached through `hdc rport` on the same
 * port. test/arkts/origin.ts serves it and records its requests.
 */
async function httpOrigin(
  routes: Readonly<Record<string, string>>,
): Promise<Origin> {
  const { id } = (await control("POST", "/origins", { routes })) as {
    id: number;
  };
  return {
    url: `${process.env.MAPLIBRE_NATIVE_FFI_ORIGIN}/o/${id}`,
    requests: async () =>
      (await control("GET", `/origins/${id}/requests`)) as OriginRequest[],
    close: async () => {
      await control("DELETE", `/origins/${id}`);
    },
  };
}

/** A queue of a realm's messages, which next() takes in order. */
function inbox(): { push(message: unknown): void; next(): Promise<unknown> } {
  const messages: unknown[] = [];
  const waiters: ((message: unknown) => void)[] = [];
  return {
    push(message) {
      const waiter = waiters.shift();
      if (waiter) {
        waiter(message);
      } else {
        messages.push(message);
      }
    },
    next() {
      if (messages.length > 0) {
        return Promise.resolve(messages.shift());
      }
      return new Promise((resolve) => waiters.push(resolve));
    },
  };
}

/**
 * An @ohos.worker realm that runs the bundle's realm entry, or its consumer
 * entry for the load scenario, which loads the packed package instead of the
 * sources.
 */
function realmSpawn(data: unknown): Realm {
  const entry =
    (data as { scenario?: string }).scenario === "load" ? "consumer" : "realm";
  const realm = new worker.ThreadWorker(`entry_test/ets/${entry}.js`);
  const messages = inbox();
  const exited = new Promise<void>((resolve) => {
    realm.onexit = () => resolve();
  });
  realm.onmessage = (event) => messages.push(event.data);
  realm.onerror = (error) => messages.push({ error: error.message });
  realm.postMessage(data);
  return {
    next: () => messages.next(),
    post: (message) => realm.postMessage(message),
    terminate: async () => {
      realm.terminate();
      await exited;
    },
  };
}

// The OpenHarmony target's ArkTS has no ES2024 ArrayBuffer.prototype.transfer,
// so the target decides that the case that detaches bytes does not run. An
// ArkTS that gains it fails the run here, so the case starts running there.
if ("transfer" in ArrayBuffer.prototype) {
  throw new Error(
    "this ArkTS has ArrayBuffer.prototype.transfer; declare bufferTransfer",
  );
}

// Nor does it define the explicit resource management symbols, so owners
// define no disposers there.
if ("asyncDispose" in Symbol) {
  throw new Error("this ArkTS has Symbol.asyncDispose; declare disposers");
}

export const capabilities: Capabilities = {
  host: "arkts",
  unavailable: ["childProcess", "processExit", "bufferTransfer", "disposers"],
  httpOrigin,
  realmSpawn,
  forcedGc: () => ArkTools.forceFullGC(),
  testGraphics: () => testGraphics,
  // An application process does not end when its event loop empties, so no
  // timer holds it.
  unrefTimer: () => {},
};
