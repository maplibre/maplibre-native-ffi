/**
 * Adapted callbacks and offline operations.
 *
 * Both families cross the same boundary: MapLibre acts on its own threads, the
 * adapter copies what it produced, and the host sees it on its own execution
 * context.
 */

import { RuntimeEventType } from "../events.ts";
import { LogSeverityMask } from "../logging.ts";
import type { Runtime } from "../runtime.ts";
import type { ConformanceGroup } from "./harness.ts";
import { drain, withRuntime } from "./harness.ts";

/** Pumps until an offline operation reports that it finished. */
function awaitCompletion(runtime: Runtime): boolean {
  for (let attempt = 0; attempt < 200; attempt += 1) {
    runtime.pump(25);
    for (
      let event = runtime.pollEvent();
      event !== undefined;
      event = runtime.pollEvent()
    ) {
      if (event.type.equals(RuntimeEventType.offlineOperationCompleted)) {
        return true;
      }
    }
  }
  return false;
}

/** Provokes MapLibre into logging, then delivers what it produced. */
function provokeLogging(
  maplibre: Parameters<ConformanceGroup["cases"][number]["run"]>[0]["maplibre"],
): void {
  withRuntime(maplibre, (runtime, open) => {
    const map = open({ width: 64, height: 64 });
    // A style the parser rejects makes MapLibre log from its own thread, with
    // no network involved: a case that reached for one would be testing the
    // host's transport rather than the callback boundary.
    map.setStyleJson('{"version": 8, "sources": 42, "layers": []}');
    drain(runtime, maplibre);
  });
}

export const LOGGING_GROUP: ConformanceGroup = {
  name: "the log callback",
  cases: [
    {
      name: "delivers copied records to this execution context",
      spec: ["BND-120"],
      run({ maplibre, expect }) {
        const records: { message: string; severity: number; event: string }[] =
          [];
        maplibre.setLogCallback((record) => {
          records.push({
            message: record.message,
            severity: record.severity.rawValue,
            event: record.event.name,
          });
        });
        try {
          provokeLogging(maplibre);
          expect.ok(records.length > 0, "records arrived");
          const record = records[0]!;
          // Every field is a copy: the record native handed over is destroyed
          // as soon as the callback returns.
          expect.ok(record.message.length > 0, "a copied message");
          expect.ok(record.severity > 0, "a severity");
          expect.notEqual(record.event, "", "an event category");
        } finally {
          maplibre.clearLogCallback();
        }
      },
    },
    {
      name: "stops delivering to a callback it replaced",
      spec: ["BND-120", "BND-122"],
      run({ maplibre, expect }) {
        const first: string[] = [];
        const second: string[] = [];
        maplibre.setLogCallback((record) => first.push(record.message));
        try {
          provokeLogging(maplibre);
          expect.ok(first.length > 0, "the first callback saw records");
          const seen = first.length;

          maplibre.setLogCallback((record) => second.push(record.message));
          provokeLogging(maplibre);
          expect.ok(second.length > 0, "the replacement saw records");
          expect.equal(first.length, seen, "the replaced callback saw no more");
        } finally {
          maplibre.clearLogCallback();
        }
      },
    },
    {
      name: "stops delivering after a clear",
      spec: ["BND-120"],
      run({ maplibre, expect }) {
        const records: string[] = [];
        maplibre.setLogCallback((record) => records.push(record.message));
        provokeLogging(maplibre);
        const seen = records.length;
        expect.ok(seen > 0, "records arrived before the clear");

        maplibre.clearLogCallback();
        provokeLogging(maplibre);
        expect.equal(records.length, seen, "no records after the clear");
      },
    },
    {
      name: "contains a failing callback and keeps delivering",
      spec: ["BND-121"],
      run({ maplibre, expect }) {
        let calls = 0;
        maplibre.setLogCallback(() => {
          calls += 1;
          // A host failure must not unwind into the native callback boundary,
          // and must not stop the records behind it.
          throw new Error("the host callback failed");
        });
        try {
          provokeLogging(maplibre);
          const afterFirst = calls;
          expect.ok(afterFirst > 0, "the failing callback ran");
          provokeLogging(maplibre);
          expect.ok(calls > afterFirst, "delivery continued after a failure");
        } finally {
          maplibre.clearLogCallback();
        }
      },
    },
    {
      name: "treats a clear with no callback installed as a no-op",
      run({ maplibre, expect }) {
        maplibre.clearLogCallback();
        // A record queued just before a clear is still an owned record, so the
        // drain releases it rather than leaving it outstanding.
        maplibre.deliverCallbacks();
        expect.equal(maplibre.pendingCallbackCount, 0, "nothing outstanding");
      },
    },
    {
      name: "combines, tests, and applies a severity mask",
      spec: ["BND-060"],
      run({ maplibre, expect }) {
        const mask = LogSeverityMask.warning.with(LogSeverityMask.error);
        expect.ok(mask.has(LogSeverityMask.warning), "the mask has warning");
        expect.ok(mask.has(LogSeverityMask.error), "the mask has error");
        expect.ok(!mask.has(LogSeverityMask.info), "and not info");
        maplibre.setAsyncLogSeverities(mask);
        maplibre.setAsyncLogSeverities(LogSeverityMask.none);
      },
    },
  ],
};

export const OFFLINE_GROUP: ConformanceGroup = {
  name: "offline operations",
  cases: [
    {
      name: "lists an empty database through start, event, and take",
      spec: ["BND-084"],
      async run({ maplibre, expect, cacheDirectory }) {
        const runtime = maplibre.createRuntime({
          cachePath: `${await cacheDirectory()}/cache.db`,
        });
        try {
          const operation = runtime.startOfflineRegionList();
          expect.ok(operation > 0n, "an operation id");
          expect.ok(awaitCompletion(runtime), "the operation completed");
          expect.equal(
            runtime.takeOfflineRegionList(operation).length,
            0,
            "an empty database lists nothing",
          );
          // Ownership transferred, so the same result cannot be taken twice.
          expect.throws(
            () => runtime.takeOfflineRegionList(operation),
            "a second take",
          );
        } finally {
          runtime.close();
        }
      },
    },
    {
      name: "reports an unknown operation id rather than inventing a result",
      spec: ["BND-045"],
      async run({ maplibre, expect, cacheDirectory }) {
        const runtime = maplibre.createRuntime({
          cachePath: `${await cacheDirectory()}/cache.db`,
        });
        try {
          expect.throws(
            () => runtime.takeOfflineRegionList(999_999n),
            "an id the runtime never issued",
          );
        } finally {
          runtime.close();
        }
      },
    },
  ],
};
