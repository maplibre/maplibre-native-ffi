/**
 * Resource URL rewriting.
 *
 * The rules are evaluated in native code because MapLibre needs the answer on
 * its own thread, so these tests assert that a rule reaches the loader and that
 * the table's lifetime follows the registration rather than the call.
 */

import {
  MaplibreError,
  Maplibre,
  ResourceKind,
  type ResourceRequest,
  type Runtime,
  RuntimeEventType,
} from "../src/index.ts";
import { afterEach, describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

let runtime: Runtime | undefined;

afterEach(() => {
  runtime?.close();
  runtime = undefined;
});

/** Loads a style and reports the message of the failure it produced. */
function loadFailureMessage(url: string): string {
  const created = runtime!;
  const map = created.createMap({ width: 64, height: 64 });
  map.setStyleUrl(url);
  let message = "";
  for (let attempt = 0; attempt < 80 && message === ""; attempt += 1) {
    created.pump(25);
    for (
      let event = created.pollEvent();
      event !== undefined;
      event = created.pollEvent()
    ) {
      if (event.type.equals(RuntimeEventType.mapLoadingFailed)) {
        message = event.message;
      }
    }
  }
  map.close();
  return message;
}

describe("resource rewrite rules", () => {
  it("rewrites a matching URL and leaves others alone", () => {
    runtime = maplibre.createRuntime();
    runtime.setResourceRewriteRules([
      {
        kind: ResourceKind.style,
        url: "https://example.invalid/before.json",
        replacementUrl: "rewritten://after.json",
      },
    ]);
    // The replacement carries a scheme no transport serves, and the loader
    // names the scheme it could not handle, so the failure says which URL
    // reached it.
    expect(loadFailureMessage("https://example.invalid/before.json")).toContain(
      "rewritten",
    );
    // A URL no rule matches reaches the loader unchanged.
    expect(loadFailureMessage("untouched://style.json")).toContain("untouched");
  });

  it("stops rewriting after a clear", () => {
    runtime = maplibre.createRuntime();
    runtime.setResourceRewriteRules([
      { url: "original://style.json", replacementUrl: "replaced://style.json" },
    ]);
    expect(loadFailureMessage("original://style.json")).toContain("replaced");

    runtime.clearResourceRewriteRules();
    expect(loadFailureMessage("original://style.json")).toContain("original");
    // Clearing twice is a no-op rather than an error.
    runtime.clearResourceRewriteRules();
  });

  it("replaces a table without leaving the old one installed", () => {
    runtime = maplibre.createRuntime();
    runtime.setResourceRewriteRules([
      { url: "before://style.json", replacementUrl: "first://style.json" },
    ]);
    runtime.setResourceRewriteRules([
      { url: "before://style.json", replacementUrl: "second://style.json" },
    ]);
    const message = loadFailureMessage("before://style.json");
    expect(message).toContain("second");
    expect(message).not.toContain("first");
  });

  it("rejects a rule whose URL cannot cross as a C string", () => {
    runtime = maplibre.createRuntime();
    const embeddedNul = `broken://a${String.fromCharCode(0)}b.json`;
    expect(() =>
      runtime!.setResourceRewriteRules([{ url: embeddedNul }]),
    ).toThrow(MaplibreError);
  });
});

describe("a resource provider", () => {
  it("serves a claimed request and passes the rest through", () => {
    runtime = maplibre.createRuntime();
    const served: string[] = [];
    const style = JSON.stringify({ version: 8, sources: {}, layers: [] });
    runtime.setResourceProvider(
      [{ url: "served://", matchPrefix: true, useRequestedUrl: true }],
      (request) => {
        served.push(request.info.requestedUrl);
        request.complete({ bytes: new TextEncoder().encode(style) });
      },
    );

    const created = runtime;
    const map = created.createMap({ width: 64, height: 64 });
    map.setStyleUrl("served://style.json");
    let loaded = false;
    for (let attempt = 0; attempt < 100 && !loaded; attempt += 1) {
      created.pump(25);
      maplibre.deliverCallbacks();
      for (
        let event = created.pollEvent();
        event !== undefined;
        event = created.pollEvent()
      ) {
        if (event.type.equals(RuntimeEventType.mapStyleLoaded)) {
          loaded = true;
        }
      }
    }

    expect(served).toEqual(["served://style.json"]);
    expect(loaded).toBe(true);
    // The style the provider served is the style the map loaded.
    expect(map.copyLoadedStyleJson()).toBe(style);

    // A URL outside the route reaches the native loader instead, which reports
    // the scheme it cannot serve.
    expect(loadFailureMessage("elsewhere://style.json")).toContain("elsewhere");
    expect(served).toHaveLength(1);
    map.close();
  });

  it("answers a request after the callback returns", () => {
    runtime = maplibre.createRuntime();
    const style = JSON.stringify({ version: 8, sources: {}, layers: [] });
    let deferred: ResourceRequest | undefined;
    runtime.setResourceProvider(
      [{ url: "deferred://", matchPrefix: true, useRequestedUrl: true }],
      (request) => {
        // Held rather than answered: a provider may answer whenever it can.
        deferred = request;
      },
    );

    const created = runtime;
    const map = created.createMap({ width: 64, height: 64 });
    map.setStyleUrl("deferred://style.json");
    for (
      let attempt = 0;
      attempt < 40 && deferred === undefined;
      attempt += 1
    ) {
      created.pump(25);
      maplibre.deliverCallbacks();
      while (created.pollEvent() !== undefined) {
        // Drained so the queue does not hold the pump open.
      }
    }
    expect(deferred).toBeDefined();
    expect(deferred!.isCancelled).toBe(false);
    expect(deferred!.isSettled).toBe(false);

    deferred!.complete({ bytes: new TextEncoder().encode(style) });
    expect(deferred!.isSettled).toBe(true);
    // Completion is terminal: a second one reports the binding's own error
    // rather than reaching C.
    expect(() => deferred!.complete({})).toThrow(MaplibreError);

    let loaded = false;
    for (let attempt = 0; attempt < 100 && !loaded; attempt += 1) {
      created.pump(25);
      for (
        let event = created.pollEvent();
        event !== undefined;
        event = created.pollEvent()
      ) {
        loaded ||= event.type.equals(RuntimeEventType.mapStyleLoaded);
      }
    }
    expect(loaded).toBe(true);
    map.close();
  });

  it("stops receiving requests after a clear", () => {
    runtime = maplibre.createRuntime();
    const created = runtime;
    let claims = 0;
    runtime.setResourceProvider(
      [{ url: "cleared://", matchPrefix: true, useRequestedUrl: true }],
      (request) => {
        claims += 1;
        request.close();
      },
    );

    /** Loads a style and delivers whatever the provider was handed. */
    const load = (): void => {
      const map = created.createMap({ width: 64, height: 64 });
      map.setStyleUrl("cleared://style.json");
      for (let attempt = 0; attempt < 40; attempt += 1) {
        created.pump(25);
        maplibre.deliverCallbacks();
        while (created.pollEvent() !== undefined) {
          // Drained so the queue does not hold the pump open.
        }
      }
      map.close();
    };

    load();
    expect(claims).toBe(1);

    created.clearResourceProvider();
    load();
    // The cleared provider receives nothing; the request went to the native
    // loader, which cannot serve this scheme.
    expect(claims).toBe(1);
  });
});
