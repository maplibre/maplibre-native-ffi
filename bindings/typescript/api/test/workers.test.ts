/**
 * The binding under more than one JavaScript realm in one process.
 *
 * `runtime.h` says each owner thread may hold one live runtime, so the shape a
 * server takes is one worker thread per runtime. In `node:worker_threads` each
 * worker is a realm of its own with its own module graph, and therefore its own
 * callback registry, while every worker shares the one loaded library. Anything
 * the support layer keys on a process-wide identity is therefore shared by
 * realms that know nothing about each other.
 *
 * These cases hold two workers side by side and ask the two questions that
 * arrangement raises: does a record reach the realm that registered for it, and
 * does a realm still get woken once another realm has loaded the binding.
 */

import { Worker } from "node:worker_threads";
import { afterAll, beforeAll, describe, expect, it } from "vitest";

/**
 * What each worker runs.
 *
 * The source is inline rather than a file of its own so the two realms are
 * legible next to the assertions about them. It loads the built package the way
 * a consumer would, and answers one command at a time so the interleaving
 * between the realms is the test's rather than the scheduler's.
 */
const WORKER_SOURCE = `
const { parentPort, workerData } = require("node:worker_threads");

let maplibre;
const seen = [];

/** Provokes MapLibre into logging, without reaching for the network. */
async function provoke() {
  const runtime = maplibre.createRuntime();
  try {
    const map = runtime.createMap({ width: 64, height: 64 });
    try {
      // A style the parser rejects makes MapLibre log from its own thread, with
      // no network involved.
      map.setStyleJson('{"version": 8, "sources": 42, "layers": []}');
      const before = seen.length;
      for (let attempt = 0; attempt < 40 && seen.length === before; attempt += 1) {
        runtime.pump(25);
        while (runtime.pollEvent() !== undefined) {
          // Drained so the queue does not hold the pump open.
        }
        await new Promise((resolve) => setTimeout(resolve, 1));
      }
    } finally {
      map.close();
    }
  } finally {
    runtime.close();
  }
}

const commands = {
  async load() {
    const { Maplibre } = await import(workerData.distUrl);
    maplibre = await Maplibre.load();
  },
  register() {
    maplibre.setLogCallback((record) => seen.push(record.message));
  },
  clearLog() {
    maplibre.clearLogCallback();
  },
  async provoke() {
    await provoke();
  },
  seen() {
    return seen.slice();
  },
  pending() {
    return maplibre.pendingCallbackCount;
  },
  async idle({ milliseconds }) {
    await new Promise((resolve) => setTimeout(resolve, milliseconds));
  },
  /**
   * Holds this realm's thread without letting its event loop turn.
   *
   * A realm that is merely idle would still run a scheduled drain, so a case
   * about who gets woken needs one realm that cannot run anything at all.
   */
  block({ milliseconds }) {
    Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, milliseconds);
  },
  /** Closes this realm's facade and reports what closing left behind. */
  close() {
    maplibre.close();
    // Closing twice is a no-op rather than an error.
    maplibre.close();
    let refusedKind = "";
    try {
      maplibre.setLogCallback(() => {});
    } catch (error) {
      refusedKind = String(error.kind);
    }
    return {
      isClosed: maplibre.isClosed,
      pending: maplibre.pendingCallbackCount,
      refusedKind,
    };
  },
};

parentPort.on("message", async (message) => {
  try {
    const value = await commands[message.command](message);
    parentPort.postMessage({ id: message.id, value });
  } catch (error) {
    parentPort.postMessage({ id: message.id, failure: String(error && error.stack || error) });
  }
});
`;

/** One worker, driven one command at a time. */
class Realm {
  readonly #worker: Worker;
  #nextId = 1;
  readonly #pending = new Map<
    number,
    { resolve: (value: unknown) => void; reject: (error: Error) => void }
  >();

  constructor(readonly name: string) {
    this.#worker = new Worker(WORKER_SOURCE, {
      eval: true,
      workerData: {
        distUrl: new URL("../dist/index.mjs", import.meta.url).href,
      },
    });
    this.#worker.on("message", (message: Record<string, unknown>) => {
      const entry = this.#pending.get(message.id as number);
      this.#pending.delete(message.id as number);
      if (message.failure !== undefined) {
        entry?.reject(new Error(`${name}: ${String(message.failure)}`));
        return;
      }
      entry?.resolve(message.value);
    });
    this.#worker.on("error", (error) => {
      for (const entry of this.#pending.values()) {
        entry.reject(error);
      }
      this.#pending.clear();
    });
    this.#worker.on("exit", (code) => {
      for (const entry of this.#pending.values()) {
        entry.reject(new Error(`${name} exited with code ${code}`));
      }
      this.#pending.clear();
    });
  }

  send<T>(command: string, payload: Record<string, unknown> = {}): Promise<T> {
    const id = this.#nextId;
    this.#nextId += 1;
    return new Promise<T>((resolve, reject) => {
      this.#pending.set(id, {
        resolve: resolve as (value: unknown) => void,
        reject,
      });
      this.#worker.postMessage({ ...payload, command, id });
    });
  }

  async dispose(): Promise<void> {
    await this.#worker.terminate();
  }
}

describe("two realms in one process", () => {
  let first: Realm;
  let second: Realm;

  beforeAll(async () => {
    // Loaded one after the other rather than together, so which realm reached
    // the library last is the test's choice. It decides which realm a
    // process-wide notifier would have belonged to, and a case that leaves that
    // to the scheduler reports a different failure each run.
    first = new Realm("first");
    await first.send("load");
    second = new Realm("second");
    await second.send("load");
  }, 60_000);

  afterAll(async () => {
    await Promise.all([first.dispose(), second.dispose()]);
  });

  it("delivers a record to the realm that registered for it", async () => {
    // The log callback is process-global in the C API, so the realm that
    // registered last owns delivery. What each realm owes is that the records
    // its own registration produced reach it and nothing else does.
    await first.send("register");
    await first.send("provoke");
    expect(await first.send("seen"), "the first realm saw its own").not.toEqual(
      [],
    );
    expect(await second.send("seen"), "and the second saw nothing").toEqual([]);

    // Registering here replaces the first realm's callback natively, and the
    // first realm's registration is retired. The retirement names a
    // registration in the first realm, so a second realm that took it would be
    // retiring one of its own.
    await second.send("register");
    const firstBefore = (await first.send<string[]>("seen")).length;
    await second.send("provoke");
    expect(
      await second.send<string[]>("seen"),
      "the second realm saw its own",
    ).not.toEqual([]);
    expect(
      (await first.send<string[]>("seen")).length,
      "the first realm saw nothing more",
    ).toBe(firstBefore);

    // And back again, so neither realm's identities are a prefix of the
    // other's by accident of ordering.
    await first.send("register");
    const secondBefore = (await second.send<string[]>("seen")).length;
    await first.send("provoke");
    expect(
      (await first.send<string[]>("seen")).length,
      "the first realm saw its own again",
    ).toBeGreaterThan(firstBefore);
    expect(
      (await second.send<string[]>("seen")).length,
      "and the second saw nothing more",
    ).toBe(secondBefore);

    await first.send("clearLog");
  }, 120_000);

  it("keeps waking a realm after another realm has loaded", async () => {
    await first.send("register");
    // Clearing retires the registration, which queues one final record for
    // this realm from this realm's own thread. Nothing drains it here: the
    // realm returns to its event loop, so only a wake signal can deliver it.
    const blocked = second.send("block", { milliseconds: 1500 });
    await first.send("clearLog");
    await first.send("idle", { milliseconds: 500 });
    expect(
      await first.send<number>("pending"),
      "the first realm was woken and drained its own retirement",
    ).toBe(0);
    await blocked;
  }, 120_000);

  it("closes each realm's owner independently", async () => {
    // Each realm holds a place of its own in the shared layer, so each releases
    // its own without waiting for the other or taking the other's with it.
    for (const realm of [first, second]) {
      expect(
        await realm.send("close"),
        `${realm.name} released its own place in the library`,
      ).toEqual({ isClosed: true, pending: 0, refusedKind: "closedHandle" });
    }
  }, 60_000);
});
