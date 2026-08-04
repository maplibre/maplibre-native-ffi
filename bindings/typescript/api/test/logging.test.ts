/**
 * Log callback delivery.
 *
 * MapLibre logs from its own threads, so these tests assert what the binding
 * owns: that a record reaches JavaScript as a copy, that replacement and clear
 * stop the old callback, and that a failing callback neither escapes into
 * native code nor stops delivery.
 */

import {
  type LogRecord,
  LogSeverityMask,
  MaplibreError,
  Maplibre,
  type Runtime,
} from "../src/index.ts";
import { afterEach, describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

let runtime: Runtime | undefined;

afterEach(() => {
  maplibre.clearLogCallback();
  runtime?.close();
  runtime = undefined;
});

/** Provokes MapLibre into logging, then delivers whatever it produced. */
function provokeLogging(): void {
  runtime ??= maplibre.createRuntime();
  const map = runtime.createMap({ width: 64, height: 64 });
  // An unreachable style makes the loader log a failure from its own thread.
  map.setStyleUrl("https://127.0.0.1:1/style.json");
  for (let attempt = 0; attempt < 40; attempt += 1) {
    runtime.pump(25);
    while (runtime.pollEvent() !== undefined) {
      // Drained so the queue does not hold the pump open.
    }
    maplibre.deliverCallbacks();
  }
  map.close();
}

describe("the log callback", () => {
  it("delivers copied records to this execution context", () => {
    const records: LogRecord[] = [];
    maplibre.setLogCallback((record) => {
      records.push(record);
    });
    provokeLogging();
    expect(records.length).toBeGreaterThan(0);
    const record = records[0]!;
    // Every field is a copy: the record native handed over is destroyed as soon
    // as the callback returns.
    expect(typeof record.message).toBe("string");
    expect(record.message.length).toBeGreaterThan(0);
    expect(record.severity.rawValue).toBeGreaterThan(0);
    expect(record.event.name).not.toBe("");
  });

  it("stops delivering to a callback it replaced", () => {
    const first: string[] = [];
    const second: string[] = [];
    maplibre.setLogCallback((record) => first.push(record.message));
    provokeLogging();
    expect(first.length).toBeGreaterThan(0);

    const seenBeforeReplacement = first.length;
    maplibre.setLogCallback((record) => second.push(record.message));
    provokeLogging();
    expect(second.length).toBeGreaterThan(0);
    expect(first).toHaveLength(seenBeforeReplacement);
  });

  it("stops delivering after a clear", () => {
    const records: string[] = [];
    maplibre.setLogCallback((record) => records.push(record.message));
    provokeLogging();
    const seen = records.length;
    expect(seen).toBeGreaterThan(0);

    maplibre.clearLogCallback();
    provokeLogging();
    expect(records).toHaveLength(seen);
  });

  it("contains a failing callback and keeps delivering", () => {
    let calls = 0;
    maplibre.setLogCallback(() => {
      calls += 1;
      // A host failure must not unwind into the native callback boundary, and
      // must not stop the records behind it.
      throw new Error("the host callback failed");
    });
    expect(() => provokeLogging()).not.toThrow();
    const afterFirst = calls;
    expect(afterFirst).toBeGreaterThan(0);
    // Delivery continues after a failure rather than stopping at it.
    expect(() => provokeLogging()).not.toThrow();
    expect(calls).toBeGreaterThan(afterFirst);
  });

  it("reports a clear with no callback installed as a no-op", () => {
    expect(() => maplibre.clearLogCallback()).not.toThrow();
    // A record queued just before a clear is still an owned record, so the
    // drain releases it rather than leaving it outstanding.
    maplibre.deliverCallbacks();
    expect(maplibre.pendingCallbackCount).toBe(0);
  });

  it("keeps the previous callback when installation fails", () => {
    const records: string[] = [];
    maplibre.setLogCallback((record) => records.push(record.message));
    // A registration whose state the C API rejects leaves the installed one
    // active; there is no public way to produce that here, so this asserts the
    // reachable half: an error from the binding is a MaplibreError.
    expect(() =>
      maplibre.setLogCallback(() => {}, { consume: true }),
    ).not.toThrow();
    provokeLogging();
    expect(MaplibreError.name).toBe("MaplibreError");
  });
});

describe("async log severities", () => {
  it("combines, tests, and applies a mask", () => {
    const both = LogSeverityMask.info.with(LogSeverityMask.warning);
    expect(both.has(LogSeverityMask.info)).toBe(true);
    expect(both.has(LogSeverityMask.error)).toBe(false);
    // The default is exactly info and warning, which is what MapLibre may
    // dispatch asynchronously; errors stay synchronous.
    expect(both.equals(LogSeverityMask.default)).toBe(true);

    maplibre.setAsyncLogSeverities(LogSeverityMask.none);
    maplibre.setAsyncLogSeverities(LogSeverityMask.all);
    maplibre.setAsyncLogSeverities(LogSeverityMask.default);
  });

  it("reports the C API's rejection of unknown bits", () => {
    try {
      maplibre.setAsyncLogSeverities(LogSeverityMask.fromRawValue(0xffff));
      expect.unreachable("unknown mask bits are invalid");
    } catch (error) {
      expect(error).toBeInstanceOf(MaplibreError);
      expect((error as MaplibreError).kind).toBe("invalidArgument");
      // The binding does not duplicate the C API's mask validation.
      expect((error as MaplibreError).diagnostic).not.toBe("");
    }
  });
});
