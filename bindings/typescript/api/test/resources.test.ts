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
