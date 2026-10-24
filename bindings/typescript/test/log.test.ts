// The process-global log callback. Every case here clears it and restores the
// default asynchronous severity mask before it ends.

import assert from "node:assert/strict";

import {
  type LogEvent,
  type LogSeverity,
  LogEvent as LogEvents,
  logClearCallback,
  logSetAsyncSeverityMask,
  logSetCallback,
  LogSeverity as LogSeverities,
  LogSeverityMask,
  type MapHandle,
} from "../src/index.ts";
import { openRuntime } from "./support/fixture.ts";
import { bounded, teardown, test } from "./support/harness.ts";
import { liveRoots, released } from "./support/internals.ts";

/** Whether a message is one of the two rules around a debug-log dump. */
function isDumpRule(message: string): boolean {
  return message.startsWith("-----");
}

/** Installs a log callback that collects dump rules, and returns them. */
function collectDumpRules(): [LogSeverity, LogEvent][] {
  const rules: [LogSeverity, LogEvent][] = [];
  logSetCallback((severity, event, _code, message) => {
    if (isDumpRule(message)) {
      rules.push([severity, event]);
    }
  });
  return rules;
}

/**
 * A map whose debug-log dump fences its own records: with no asynchronous
 * severities, MapLibre logs each record on the thread that raises it, so the
 * records reach the realm's queue before the dump's completion does.
 */
async function openLoggingMap(): Promise<MapHandle> {
  const roots = liveRoots();
  logSetAsyncSeverityMask(0);
  teardown(async () => {
    logClearCallback();
    logSetAsyncSeverityMask(LogSeverityMask.defaultValue);
    // The release arrives through the realm's queue; a later case that counts roots
    // must not see it.
    await released(() => liveRoots() === roots, "the log registration release");
  });
  const fixture = await openRuntime();
  return fixture.openMap();
}

test("replacing the log callback releases the previous registration", async () => {
  const map = await openLoggingMap();
  const baseline = liveRoots();
  const dump = () => bounded(map.dumpDebugLogs(), "a command");

  const first = collectDumpRules();
  assert.equal(liveRoots(), baseline + 1);
  await dump();
  const rules = first.length;
  assert.ok(rules > 0);
  assert.deepEqual(
    first,
    Array.from({ length: rules }, () => [
      LogSeverities.info,
      LogEvents.general,
    ]),
  );

  const replacement = collectDumpRules();
  await released(
    () => liveRoots() === baseline + 1,
    "the replaced registration release",
  );
  await dump();
  assert.equal(replacement.length, rules);
  assert.equal(first.length, rules);

  logClearCallback();
  await released(
    () => liveRoots() === baseline,
    "the cleared registration release",
  );
  await dump();
  assert.equal(replacement.length, rules);
});

test("a log record reaches the event loop after its native call returns", async () => {
  const map = await openLoggingMap();
  const order: string[] = [];
  logSetCallback((_severity, _event, _code, message) => {
    if (isDumpRule(message)) {
      order.push("record");
    }
  });
  const dumped = map.dumpDebugLogs();
  order.push("returned");
  await bounded(dumped, "a command");
  // The records were raised on a native thread while this thread still ran,
  // and they waited in the realm's queue until it returned to the event loop.
  assert.equal(order[0], "returned");
  assert.ok(order.length > 1, "the dump logged no record");
  assert.ok(order.slice(1).every((entry) => entry === "record"));
});
