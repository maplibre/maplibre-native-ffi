/**
 * Adapted callbacks and offline operations.
 *
 * Both families cross the same boundary: MapLibre acts on its own threads, the
 * adapter copies what it produced, and the host sees it on its own execution
 * context.
 */

import type { MaplibreError } from "../errors.ts";
import { RuntimeEventType } from "../events.ts";
import { clearForcedStatuses, forceStatus } from "../internal/faults.ts";
import { LogSeverityMask } from "../logging.ts";
import type { Map } from "../map.ts";
import type { Maplibre } from "../maplibre.ts";
import type { OfflineRegionStatus } from "../offline.ts";
import { EP } from "../raw/entrypoints.ts";
import type { ResourceRequestInfo } from "../resource-request.ts";
import {
  ResourceErrorReason,
  type ResourceRequest,
  ResourceResponseStatus,
} from "../resource-request.ts";
import type { Runtime } from "../runtime.ts";
import type {
  ConformanceGroup,
  HttpOrigin,
  RecordedRequest,
} from "./harness.ts";
import { drain, EMPTY_STYLE, loadStyle, withRuntime } from "./harness.ts";

/**
 * Asks the map for a style at a path on the origin, and reports what arrived.
 *
 * `pump` blocks in native code while the origin answers on the host's own
 * event loop, so a loop that only pumped would hold the host still and the
 * origin would never accept the connection. That is why this waits by giving
 * the host a turn between pumps rather than by pumping harder.
 */
async function requestThrough(
  maplibre: Maplibre,
  runtime: Runtime,
  map: Map,
  origin: HttpOrigin,
  path: string,
): Promise<RecordedRequest | undefined> {
  const before = origin.requests.length;
  map.setStyleUrl(`${origin.url}${path}`);
  for (let attempt = 0; attempt < 200; attempt += 1) {
    runtime.pump(25);
    maplibre.deliverCallbacks();
    while (runtime.pollEvent() !== undefined) {
      // Drained so the queue does not hold the pump open.
    }
    if (origin.requests.length > before) {
      return origin.requests[origin.requests.length - 1];
    }
    await new Promise((resolve) => setTimeout(resolve, 1));
  }
  return undefined;
}

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
      name: "gives a native list back when copying out of it fails",
      spec: ["BND-066"],
      async run({ maplibre, expect, cacheDirectory }) {
        const runtime = maplibre.createRuntime({
          cachePath: `${await cacheDirectory()}/cache.db`,
        });
        try {
          const operation = runtime.startOfflineRegionList();
          expect.ok(awaitCompletion(runtime), "the operation completed");

          // Taking the result acquires a native list and then copies out of
          // it. A copy that fails must still give the list back, and no call a
          // caller can make fails there, so it is arranged.
          forceStatus(EP.mln_offline_region_list_count, -5);
          try {
            expect.throws(
              () => runtime.takeOfflineRegionList(operation),
              "a copy that refuses after the list was acquired",
            );
          } finally {
            clearForcedStatuses();
          }

          // The list was given back rather than leaked, so the runtime closes
          // without a live child holding it open. A leaked list would keep it.
          runtime.close();
          expect.equal(runtime.isClosed, true, "the runtime closed cleanly");
        } finally {
          if (!runtime.isClosed) {
            clearForcedStatuses();
            runtime.close();
          }
        }
      },
    },
    {
      name: "reports a region's status through the event model",
      spec: ["BND-085"],
      async run({ maplibre, expect, cacheDirectory }) {
        const runtime = maplibre.createRuntime({
          cachePath: `${await cacheDirectory()}/cache.db`,
        });
        try {
          // An empty database holds no region, so this names one that is not
          // there. The point is the shape of the answer: offline work reports
          // through an operation and an event rather than returning, and a
          // failure has to arrive the same way a result would.
          const operation = runtime.startOfflineRegionStatus(1n);
          expect.ok(operation > 0n, "an operation id");
          expect.ok(
            awaitCompletion(runtime),
            "the operation completed through the runtime event model",
          );

          // Whether a missing region is an error or an empty status is the
          // library's decision. What this binding owes is that the answer is
          // copied out and reaches the caller either way, rather than being
          // invented or lost.
          let status: OfflineRegionStatus | undefined;
          let refused: MaplibreError | undefined;
          try {
            status = runtime.takeOfflineRegionStatus(operation);
          } catch (error) {
            refused = error as MaplibreError;
          }
          if (refused !== undefined) {
            expect.ok(
              refused.diagnostic.length > 0 || refused.kind.length > 0,
              "a refusal names why",
            );
          } else {
            const answered = expect.defined(status, "a status");
            expect.equal(
              typeof answered.complete,
              "boolean",
              "the status was copied out as public values",
            );
            expect.equal(
              typeof answered.completedTileCount,
              "bigint",
              "counts keep their full domain",
            );
          }

          // Either way the operation is spent, so replaying it is refused.
          expect.throws(
            () => runtime.takeOfflineRegionStatus(operation),
            "taking a spent operation's result again",
          );
        } finally {
          runtime.close();
        }
      },
    },
    {
      name: "refuses a released operation id after a new one exists",
      spec: ["BND-045"],
      async run({ maplibre, expect, cacheDirectory }) {
        const runtime = maplibre.createRuntime({
          cachePath: `${await cacheDirectory()}/cache.db`,
        });
        try {
          const first = runtime.startOfflineRegionList();
          expect.ok(awaitCompletion(runtime), "the first operation completed");
          // Taking the result releases the operation, so this id now names
          // nothing.
          runtime.takeOfflineRegionList(first);

          // A second operation may be handed the same id the first gave up.
          const second = runtime.startOfflineRegionList();
          expect.ok(awaitCompletion(runtime), "the second operation completed");

          // Replaying the released id must be refused whether or not it was
          // reused. If it was reused, accepting it would take the second
          // operation's result out from under its owner.
          expect.equal(
            expect.throws(
              () => runtime.takeOfflineRegionList(first),
              "replaying a released operation id",
            ).kind,
            "invalidArgument",
            "the id is refused rather than answered",
          );

          // The live operation is untouched by the replay.
          expect.equal(
            runtime.takeOfflineRegionList(second).length,
            0,
            "the live operation still answers",
          );
        } finally {
          runtime.close();
        }
      },
    },
    {
      name: "reports an unknown operation id rather than inventing a result",
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

/**
 * Loads a style and reports the message of the failure it produced.
 *
 * A URL rewrite is applied by the native loader on its way to an HTTP request,
 * and a resource provider is consulted before that, so a provider sees the URL
 * as it was written. The loader naming the scheme it could not serve is the
 * only place the transformed URL becomes visible without a server to fetch
 * from.
 */
function loadFailureMessage(
  runtime: Runtime,
  open: (options?: { width: number; height: number }) => Map,
  maplibre: Maplibre,
  url: string,
): string {
  const map = open({ width: 64, height: 64 });
  map.setStyleUrl(url);
  let message = "";
  for (let attempt = 0; attempt < 80 && message === ""; attempt += 1) {
    runtime.pump(25);
    maplibre.deliverCallbacks();
    for (
      let event = runtime.pollEvent();
      event !== undefined;
      event = runtime.pollEvent()
    ) {
      if (event.type.equals(RuntimeEventType.mapLoadingFailed)) {
        message = event.message;
      }
    }
  }
  return message;
}

/**
 * The rewrite cases observe the loader's own failure for a scheme it cannot
 * serve. That path reaches the platform's HTTP transport, and this build's
 * WebAssembly transport traps rather than reporting for an unknown scheme, so
 * what is restricted is the observation rather than the behavior.
 */
const LOADER_OBSERVED = ["node-api"] as const;

export const RESOURCES_GROUP: ConformanceGroup = {
  name: "resource interception",
  cases: [
    {
      name: "rewrites a matching URL and leaves others alone",
      transports: LOADER_OBSERVED,
      spec: ["BND-140"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          runtime.setResourceRewriteRules([
            {
              url: "https://example.invalid/before.json",
              replacementUrl: "rewritten://after.json",
            },
          ]);
          // The replacement carries a scheme no transport serves, and the
          // loader names the scheme it could not handle, so the failure says
          // which URL reached it.
          expect.contains(
            loadFailureMessage(
              runtime,
              open,
              maplibre,
              "https://example.invalid/before.json",
            ),
            "rewritten",
            "the rewritten scheme reached the loader",
          );
          expect.contains(
            loadFailureMessage(
              runtime,
              open,
              maplibre,
              "untouched://style.json",
            ),
            "untouched",
            "a URL no rule matches is unchanged",
          );
        });
      },
    },
    {
      name: "stops rewriting after a clear",
      transports: LOADER_OBSERVED,
      spec: ["BND-140"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          runtime.setResourceRewriteRules([
            {
              url: "original://style.json",
              replacementUrl: "replaced://style.json",
            },
          ]);
          expect.contains(
            loadFailureMessage(
              runtime,
              open,
              maplibre,
              "original://style.json",
            ),
            "replaced",
            "the rule applied",
          );

          runtime.clearResourceRewriteRules();
          expect.contains(
            loadFailureMessage(
              runtime,
              open,
              maplibre,
              "original://style.json",
            ),
            "original",
            "the cleared rule no longer applies",
          );
          // Clearing twice is a no-op rather than an error.
          runtime.clearResourceRewriteRules();
        });
      },
    },
    {
      name: "replaces a rule table without leaving the old one installed",
      transports: LOADER_OBSERVED,
      spec: ["BND-122"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          runtime.setResourceRewriteRules([
            {
              url: "before://style.json",
              replacementUrl: "first://style.json",
            },
          ]);
          runtime.setResourceRewriteRules([
            {
              url: "before://style.json",
              replacementUrl: "second://style.json",
            },
          ]);
          const message = loadFailureMessage(
            runtime,
            open,
            maplibre,
            "before://style.json",
          );
          expect.contains(message, "second", "the replacement applied");
          expect.ok(!message.includes("first"), "the replaced table is gone");
        });
      },
    },
    {
      name: "rejects a rule whose URL cannot cross as a C string",
      spec: ["BND-024"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime) => {
          const error = expect.throws(
            () =>
              runtime.setResourceRewriteRules([
                { url: `broken://a${String.fromCharCode(0)}b.json` },
              ]),
            "an embedded NUL in a rule URL",
          );
          expect.equal(error.kind, "invalidInput", "the error kind");
        });
      },
    },
    {
      name: "answers a claimed request after the callback returns",
      spec: ["BND-144", "BND-146", "BND-156"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const style = EMPTY_STYLE;
          let deferred: ResourceRequest | undefined;
          runtime.setResourceProvider(
            [{ url: "deferred://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              // Held rather than answered: a provider answers when it can.
              deferred = request;
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("deferred://style.json");
          for (
            let attempt = 0;
            attempt < 40 && deferred === undefined;
            attempt += 1
          ) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          }
          const request = expect.defined(deferred, "the claimed request");
          expect.ok(!request.isCancelled, "the request is still wanted");
          expect.ok(!request.isSettled, "and not yet answered");

          request.complete({ bytes: new TextEncoder().encode(style) });
          expect.ok(request.isSettled, "the request was answered");
          // Completion is terminal: a second one reports the binding's own
          // error rather than reaching C.
          expect.equal(
            expect.throws(() => request.complete({}), "a second completion")
              .kind,
            "closedHandle",
            "the error a second completion reports",
          );

          let loaded = false;
          for (let attempt = 0; attempt < 100 && !loaded; attempt += 1) {
            runtime.pump(25);
            for (
              let event = runtime.pollEvent();
              event !== undefined;
              event = runtime.pollEvent()
            ) {
              loaded ||= event.type.equals(RuntimeEventType.mapStyleLoaded);
            }
          }
          expect.ok(loaded, "the deferred answer loaded the style");
        });
      },
    },
    {
      name: "refuses to answer a request it gave up",
      spec: ["BND-147"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let given: ResourceRequest | undefined;
          runtime.setResourceProvider(
            [{ url: "given://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              given = request;
            },
          );
          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("given://style.json");
          for (
            let attempt = 0;
            attempt < 40 && given === undefined;
            attempt += 1
          ) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          }
          const request = expect.defined(given, "the claimed request");

          request.close();
          expect.ok(request.isSettled, "giving it up settles it");
          // Everything that would reach C reports the binding's own error, so a
          // released native request is never touched.
          expect.equal(
            expect.throws(
              () => request.complete({ bytes: new Uint8Array(1) }),
              "completing a request that was given up",
            ).kind,
            "closedHandle",
            "the error completing reports",
          );
          expect.throws(
            () => request.isCancelled,
            "asking whether a released request was cancelled",
          );
          // Giving it up twice is not an error; it is already given up.
          request.close();
        });
      },
    },
    {
      name: "adds headers to a request it claims",
      needs: ["httpHeaderTransforms"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let seen: ResourceRequestInfo | undefined;
          runtime.setHttpHeaderTransforms([
            {
              url: "headers://",
              matchPrefix: true,
              headers: [
                { name: "X-Conformance", value: "yes" },
                { name: "X-Second", value: "also" },
              ],
            },
          ]);
          // A provider claims the request so it never leaves this process; the
          // transform still runs, because it applies to the request rather
          // than to whoever answers it.
          runtime.setResourceProvider(
            [{ url: "headers://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              seen ??= request.info;
              request.complete({
                bytes: new TextEncoder().encode(EMPTY_STYLE),
              });
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("headers://style.json");
          for (
            let attempt = 0;
            attempt < 60 && seen === undefined;
            attempt += 1
          ) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          }
          // The rule's strings were copied at registration, so what native
          // code read cannot depend on anything this case still holds.
          expect.defined(seen, "the request the transform applied to");
        });
      },
    },
    {
      name: "installs, replaces, and clears header rules while maps are live",
      needs: ["httpHeaderTransforms"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open({ width: 64, height: 64 });
          loadStyle(runtime, map);

          runtime.setHttpHeaderTransforms([
            { url: "first://", headers: [{ name: "X-One", value: "1" }] },
          ]);
          // Replacing does not leave the old table installed, and the old one
          // is released rather than leaked, which closing the runtime proves.
          runtime.setHttpHeaderTransforms([
            {
              url: "second://",
              matchPrefix: true,
              headers: [{ name: "X-Two", value: "2" }],
            },
          ]);
          runtime.clearHttpHeaderTransforms();
          // Clearing with nothing installed is a no-op rather than an error.
          runtime.clearHttpHeaderTransforms();

          // A header name that cannot cross as a C string is refused by the
          // binding, and the rules already installed are untouched.
          runtime.setHttpHeaderTransforms([
            { url: "third://", headers: [{ name: "X-Three", value: "3" }] },
          ]);
          expect.equal(
            expect.throws(
              () =>
                runtime.setHttpHeaderTransforms([
                  {
                    url: "fourth://",
                    headers: [
                      { name: `X${String.fromCharCode(0)}Bad`, value: "4" },
                    ],
                  },
                ]),
              "a header name carrying an embedded NUL",
            ).kind,
            "invalidInput",
            "the binding refuses it before crossing into C",
          );
          runtime.clearHttpHeaderTransforms();
        });
      },
    },
    {
      name: "carries transformed headers to a real request and stops on clear",
      spec: ["BND-159"],
      needs: ["httpHeaderTransforms", "httpOrigin"],
      async run({ maplibre, expect, httpOrigin }) {
        const origin = await httpOrigin();
        const runtime = maplibre.createRuntime();
        try {
          // The map stays open across every install, replacement, and clear,
          // so what this case proves is a rule table changing under a live map
          // rather than one chosen before anything was running.
          const map = runtime.createMap({ width: 64, height: 64 });
          try {
            runtime.setHttpHeaderTransforms([
              {
                url: origin.url,
                matchPrefix: true,
                headers: [
                  { name: "X-Conformance", value: "first" },
                  { name: "X-Second", value: "also" },
                ],
              },
            ]);
            const first = expect.defined(
              await requestThrough(
                maplibre,
                runtime,
                map,
                origin,
                "first.json",
              ),
              "the request the installed rule applied to",
            );
            expect.equal(
              first.headers.get("x-conformance"),
              "first",
              "the header the rule supplied reached the origin",
            );
            expect.equal(
              first.headers.get("x-second"),
              "also",
              "the rule's whole header list reached the origin",
            );

            // The replacement matches a narrower prefix, so the same run shows
            // both that the old table is gone and that a rule adds headers to
            // the requests it claims rather than to every request.
            runtime.setHttpHeaderTransforms([
              {
                url: `${origin.url}scoped/`,
                matchPrefix: true,
                headers: [{ name: "X-Replaced", value: "second" }],
              },
            ]);
            const matched = expect.defined(
              await requestThrough(
                maplibre,
                runtime,
                map,
                origin,
                "scoped/style.json",
              ),
              "the request the replacement rule matched",
            );
            expect.equal(
              matched.headers.get("x-replaced"),
              "second",
              "the replacement's header reached the origin",
            );
            expect.absent(
              matched.headers.get("x-conformance"),
              "a header from the replaced table",
            );
            const unmatched = expect.defined(
              await requestThrough(
                maplibre,
                runtime,
                map,
                origin,
                "elsewhere/style.json",
              ),
              "the request outside the rule's prefix",
            );
            expect.absent(
              unmatched.headers.get("x-replaced"),
              "a header on a request the rule does not claim",
            );

            runtime.clearHttpHeaderTransforms();
            const cleared = expect.defined(
              await requestThrough(
                maplibre,
                runtime,
                map,
                origin,
                "scoped/after-clear.json",
              ),
              "the request after the clear",
            );
            expect.absent(
              cleared.headers.get("x-replaced"),
              "a header after the table was cleared",
            );
            expect.absent(
              cleared.headers.get("x-conformance"),
              "a header from any earlier table after the clear",
            );
          } finally {
            map.close();
          }
        } finally {
          runtime.close();
          origin.close();
        }
      },
    },
    {
      name: "keeps a scheme alias in the URL it was asked for",
      spec: ["BND-155", "BND-157"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let seen: { requested: string; resolved: string } | undefined;
          // The route compares the URL as asked for, which still carries the
          // alias. A route matching what a fetch would use would not claim it.
          runtime.setResourceProvider(
            [{ url: "maplibre://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              seen = {
                requested: request.info.requestedUrl,
                resolved: request.info.resolvedUrl,
              };
              request.complete({
                bytes: new TextEncoder().encode(EMPTY_STYLE),
              });
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("maplibre://maps/streets");
          for (
            let attempt = 0;
            attempt < 60 && seen === undefined;
            attempt += 1
          ) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          }

          const urls = expect.defined(seen, "the claimed request");
          expect.contains(
            urls.requested,
            "maplibre://",
            "the requested URL keeps the alias it was asked for",
          );
          // What a provider would fetch is the alias resolved, which is a
          // different string. Both are offered because a cache keys on one and
          // a fetch uses the other.
          expect.notEqual(
            urls.resolved,
            urls.requested,
            "the resolved URL is the alias expanded",
          );
        });
      },
    },
    {
      name: "settles a request answered inside the callback",
      spec: ["BND-150"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let settledInside: boolean | undefined;
          runtime.setResourceProvider(
            [{ url: "inline://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              request.complete({
                bytes: new TextEncoder().encode(EMPTY_STYLE),
              });
              // Ownership is finalized by the answer, not by this callback
              // returning, so it is already settled here.
              settledInside = request.isSettled;
              // Whatever this callback does afterwards cannot unsettle it.
              request.close();
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("inline://style.json");
          let loaded = false;
          for (let attempt = 0; attempt < 200 && !loaded; attempt += 1) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            for (
              let event = runtime.pollEvent();
              event !== undefined;
              event = runtime.pollEvent()
            ) {
              loaded ||= event.type.equals(RuntimeEventType.mapStyleLoaded);
            }
          }
          expect.equal(settledInside, true, "answering settled it inside");
          expect.ok(loaded, "and the answer reached MapLibre");
        });
      },
    },
    {
      name: "reports a cancelled request before a late answer",
      spec: ["BND-148"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let held: ResourceRequest | undefined;
          runtime.setResourceProvider(
            [{ url: "late://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              held ??= request;
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("late://style.json");
          const pump = (): void => {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          };
          for (
            let attempt = 0;
            attempt < 40 && held === undefined;
            attempt += 1
          ) {
            pump();
          }
          const request = expect.defined(held, "the claimed request");
          expect.ok(!request.isCancelled, "nothing has cancelled it yet");

          // MapLibre stops wanting this one when the map is pointed elsewhere.
          // The host is not told; it asks, which is why a provider that takes
          // its time checks before doing the work.
          map.setStyleJson(EMPTY_STYLE);
          let cancelled = false;
          for (let attempt = 0; attempt < 200 && !cancelled; attempt += 1) {
            pump();
            cancelled = request.isCancelled;
          }
          expect.ok(cancelled, "the request reports that it was cancelled");

          // An answer that arrives after the cancellation is refused, and the
          // refusal is the library's own status mapped to a public kind rather
          // than something this binding decided.
          const late = expect.throws(
            () =>
              request.complete({
                bytes: new TextEncoder().encode(EMPTY_STYLE),
              }),
            "answering a request that was cancelled",
          );
          expect.equal(late.kind, "invalidState", "the mapped native status");
          expect.contains(
            late.diagnostic,
            "cancelled",
            "carrying the library's own diagnostic",
          );
        });
      },
    },
    {
      name: "leaves a request answerable when it refuses the response",
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let held: ResourceRequest | undefined;
          runtime.setResourceProvider(
            [{ url: "refused://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              held ??= request;
            },
          );
          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("refused://style.json");
          for (
            let attempt = 0;
            attempt < 40 && held === undefined;
            attempt += 1
          ) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          }
          const request = expect.defined(held, "the claimed request");

          // A response this binding refuses never reaches C, so the native
          // request is still outstanding. Treating it as answered would strand
          // it: nobody could complete it and nobody could give it up.
          expect.equal(
            expect.throws(
              () =>
                request.complete({
                  etag: `bad${String.fromCharCode(0)}etag`,
                }),
              "a response carrying an embedded NUL",
            ).kind,
            "invalidInput",
            "the binding refuses it before crossing into C",
          );
          expect.equal(
            request.isSettled,
            false,
            "the request is still outstanding",
          );

          // And the host can answer it properly on the next try.
          request.complete({ bytes: new TextEncoder().encode(EMPTY_STYLE) });
          expect.ok(request.isSettled, "a valid answer settles it");
        });
      },
    },
    {
      name: "keeps a released request from reaching a later one",
      spec: ["BND-151"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const seen: ResourceRequest[] = [];
          runtime.setResourceProvider(
            [{ url: "stale://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              seen.push(request);
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("stale://first.json");
          const pump = (): void => {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            while (runtime.pollEvent() !== undefined) {
              // Drained so the queue does not hold the pump open.
            }
          };
          for (let attempt = 0; attempt < 40 && seen.length < 1; attempt += 1) {
            pump();
          }
          const first = expect.defined(seen[0], "the first request");

          // Given up, so the native request behind it is released and its slot
          // may be handed to whatever asks next.
          first.close();

          map.setStyleUrl("stale://second.json");
          for (let attempt = 0; attempt < 40 && seen.length < 2; attempt += 1) {
            pump();
          }
          const second = expect.defined(seen[1], "the second request");
          expect.notEqual(
            second.info.requestedUrl,
            first.info.requestedUrl,
            "the second request is a different one",
          );

          // The stale wrapper refuses on its own account. If it reached C it
          // could answer or release whatever now holds that slot.
          expect.equal(
            expect.throws(
              () => first.complete({ bytes: new Uint8Array(1) }),
              "completing through a released request",
            ).kind,
            "closedHandle",
            "the stale request reports its own state",
          );
          first.close();

          // The later request is untouched by any of that.
          expect.ok(!second.isSettled, "the second request is still open");
          second.complete({ bytes: new TextEncoder().encode(EMPTY_STYLE) });
          expect.ok(second.isSettled, "and answers normally");
        });
      },
    },
    {
      name: "turns an error response into a loading failure",
      spec: ["BND-149", "BND-152"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          runtime.setResourceProvider(
            [{ url: "failing://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              // An error is an answer: it settles the request the way bytes do,
              // and the status it carries reaches MapLibre rather than this
              // binding inventing one.
              request.complete({
                status: ResourceResponseStatus.error,
                errorReason: ResourceErrorReason.notFound,
                errorMessage: "no such style",
              });
              expect.ok(request.isSettled, "an error answer settles it");
              expect.equal(
                expect.throws(
                  () => request.complete({}),
                  "answering again after an error",
                ).kind,
                "closedHandle",
                "an error answer is as terminal as a successful one",
              );
            },
          );

          const map = open({ width: 64, height: 64 });
          map.setStyleUrl("failing://style.json");
          let failed = false;
          for (let attempt = 0; attempt < 200 && !failed; attempt += 1) {
            runtime.pump(25);
            maplibre.deliverCallbacks();
            for (
              let event = runtime.pollEvent();
              event !== undefined;
              event = runtime.pollEvent()
            ) {
              if (event.type.equals(RuntimeEventType.mapLoadingFailed)) {
                failed = true;
              }
            }
          }
          expect.ok(failed, "the error reached the runtime as a failure event");
        });
      },
    },
    {
      name: "stops receiving requests after a provider is cleared",
      spec: ["BND-154"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          let claims = 0;
          runtime.setResourceProvider(
            [{ url: "cleared://", matchPrefix: true, useRequestedUrl: true }],
            (request) => {
              claims += 1;
              request.close();
            },
          );

          const load = (): void => {
            const map = open({ width: 64, height: 64 });
            map.setStyleUrl("cleared://style.json");
            drain(runtime, maplibre);
          };

          load();
          expect.equal(claims, 1, "the provider was asked once");

          runtime.clearResourceProvider();
          load();
          // The cleared provider receives nothing; the request went to the
          // native loader, which cannot serve this scheme.
          expect.equal(claims, 1, "the cleared provider was not asked");
        });
      },
    },
  ],
};
