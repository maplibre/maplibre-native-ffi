/**
 * The conformance suite, as data.
 *
 * Every runtime this binding supports has its own test framework, and none of
 * them agrees with the others about how a test is declared. The suite is
 * therefore a tree of named cases with plain async bodies, and each runtime's
 * runner registers the same tree in whatever its framework expects. A case that
 * passes under Node and fails under Bun is then a real difference between the
 * runtimes rather than a difference between two suites.
 *
 * These are the cases that must hold on every runtime and transport. Behavior a
 * single runtime owns — module formats, worker transfer — is tested beside that
 * runtime instead.
 */

import { MaplibreError } from "./errors.ts";
import { RuntimeEventType } from "./events.ts";
import { type Map, Maplibre } from "./index.ts";
import { jsonEquals, jsonFrom, jsonUint } from "./json.ts";
import type { Runtime } from "./runtime.ts";

/** Fails the case when the value is not what it should be. */
export interface Expect {
  equal<T>(actual: T, expected: T, what: string): void;
  ok(actual: boolean, what: string): void;
  throws(body: () => void, what: string): MaplibreError;
}

export interface ConformanceCase {
  readonly name: string;
  run(context: { maplibre: Maplibre; expect: Expect }): Promise<void> | void;
}

export interface ConformanceGroup {
  readonly name: string;
  readonly cases: readonly ConformanceCase[];
}

const EMPTY_STYLE = JSON.stringify({
  version: 8,
  name: "empty",
  sources: {},
  layers: [],
});

/** Runs `body` with a runtime, closing it and its maps however the body ends. */
function withRuntime<T>(
  maplibre: Maplibre,
  body: (
    runtime: Runtime,
    open: (options?: { width: number; height: number }) => Map,
  ) => T,
): T {
  const runtime = maplibre.createRuntime();
  const maps: Map[] = [];
  try {
    return body(runtime, (options = { width: 256, height: 256 }) => {
      const map = runtime.createMap(options);
      maps.push(map);
      return map;
    });
  } finally {
    while (maps.length > 0) {
      maps.pop()!.close();
    }
    runtime.close();
  }
}

/** Pumps until an event of this type arrives, reporting whether it did. */
function pumpFor(
  runtime: Runtime,
  type: RuntimeEventType,
  attempts = 200,
): boolean {
  for (let attempt = 0; attempt < attempts; attempt += 1) {
    runtime.pump(25);
    for (
      let event = runtime.pollEvent();
      event !== undefined;
      event = runtime.pollEvent()
    ) {
      if (event.type.equals(type)) {
        return true;
      }
    }
  }
  return false;
}

export const CONFORMANCE: readonly ConformanceGroup[] = [
  {
    name: "the loaded library",
    cases: [
      {
        name: "reports one compiled render backend",
        run({ maplibre, expect }) {
          const enabled = Object.values(maplibre.renderBackends).filter(
            Boolean,
          );
          // MapLibre Native compiles exactly one renderer per build.
          expect.equal(enabled.length, 1, "compiled render backends");
        },
      },
      {
        name: "reports the C ABI version it was built against",
        run({ maplibre, expect }) {
          expect.equal(maplibre.cVersion, 0, "C ABI version");
        },
      },
    ],
  },
  {
    name: "runtime lifetime",
    cases: [
      {
        name: "creates, pumps, drains, and closes",
        run({ maplibre, expect }) {
          const runtime = maplibre.createRuntime();
          runtime.pump(0);
          expect.equal(runtime.pollEvent(), undefined, "an empty event queue");
          runtime.close();
          expect.ok(runtime.isClosed, "the runtime is closed");
          // Closing twice succeeds without crossing into native code.
          runtime.close();
        },
      },
      {
        name: "reports its own closed state before reaching native code",
        run({ maplibre, expect }) {
          const runtime = maplibre.createRuntime();
          runtime.close();
          const error = expect.throws(
            () => runtime.pump(0),
            "a pump after close",
          );
          expect.equal(error.kind, "closedHandle", "the error kind");
          expect.equal(error.diagnostic, "", "no native diagnostic");
        },
      },
      {
        name: "wakes a parked pump from a wake source",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime) => {
            const wake = runtime.acquireWakeSource();
            wake.signal();
            const started = Date.now();
            runtime.pump(5_000);
            expect.ok(
              Date.now() - started < 2_000,
              "the pump returned on the wake",
            );
            wake.close();
          });
        },
      },
    ],
  },
  {
    name: "maps and styles",
    cases: [
      {
        name: "applies creation options",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const size = open({ width: 320, height: 240 }).getSize();
            expect.equal(size.width, 320, "map width");
            expect.equal(size.height, 240, "map height");
          });
        },
      },
      {
        name: "keeps its runtime open while a map is live",
        run({ maplibre, expect }) {
          const runtime = maplibre.createRuntime();
          const map = runtime.createMap({ width: 64, height: 64 });
          const error = expect.throws(
            () => runtime.close(),
            "closing a runtime with a live map",
          );
          expect.equal(error.kind, "childrenLive", "the error kind");
          map.close();
          runtime.close();
        },
      },
      {
        name: "loads a style and reads the document back unchanged",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            map.setStyleJson(EMPTY_STYLE);
            expect.ok(
              pumpFor(runtime, RuntimeEventType.mapStyleLoaded),
              "the style loaded",
            );
            expect.equal(
              map.copyLoadedStyleJson(),
              EMPTY_STYLE,
              "the document",
            );
            expect.equal(map.copyStyleUrl(), "", "no URL was requested");
          });
        },
      },
      {
        name: "names the live map an event came from",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            const other = open();
            map.setStyleJson(EMPTY_STYLE);
            let named = false;
            for (let attempt = 0; attempt < 200 && !named; attempt += 1) {
              runtime.pump(25);
              for (
                let event = runtime.pollEvent();
                event !== undefined;
                event = runtime.pollEvent()
              ) {
                if (event.type.equals(RuntimeEventType.mapStyleLoaded)) {
                  expect.ok(event.map === map, "the event names its map");
                  expect.ok(event.map !== other, "and not the other map");
                  named = true;
                }
              }
            }
            expect.ok(named, "a style-loaded event arrived");
          });
        },
      },
    ],
  },
  {
    name: "the camera",
    cases: [
      {
        name: "reads back a camera it was jumped to",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open();
            map.jumpTo({
              center: { latitude: 45.5, longitude: -122.6 },
              zoom: 11,
              bearing: 30,
            });
            const camera = map.getCamera();
            expect.ok(
              Math.abs((camera.center?.latitude ?? 0) - 45.5) < 1e-6,
              "the latitude",
            );
            expect.ok(Math.abs((camera.zoom ?? 0) - 11) < 1e-6, "the zoom");
            // The anchor is input-only, so a snapshot never reports one.
            expect.equal(camera.anchor, undefined, "no anchor in a snapshot");
          });
        },
      },
      {
        name: "separates an omitted field from one set to zero",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open();
            map.jumpTo({
              center: { latitude: 10, longitude: 20 },
              zoom: 5,
              bearing: 40,
            });
            map.jumpTo({ bearing: 0 });
            const camera = map.getCamera();
            expect.equal(camera.bearing, 0, "the bearing is present and zero");
            expect.ok(
              Math.abs((camera.zoom ?? 0) - 5) < 1e-6,
              "the zoom was untouched",
            );
          });
        },
      },
      {
        name: "propagates a native invalid-argument diagnostic",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open();
            const error = expect.throws(
              () => map.jumpTo({ zoom: Number.NaN }),
              "a non-finite zoom",
            );
            expect.equal(error.kind, "invalidArgument", "the error kind");
            expect.ok(error.diagnostic !== "", "the native diagnostic");
          });
        },
      },
    ],
  },
  {
    name: "projections",
    cases: [
      {
        name: "round-trips screen and geographic space",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open({ width: 512, height: 512 });
            map.jumpTo({
              center: { latitude: 45.5, longitude: -122.6 },
              zoom: 10,
            });
            const projection = map.createProjection();
            try {
              const centre = projection.pixelForLatLng({
                latitude: 45.5,
                longitude: -122.6,
              });
              expect.ok(Math.abs(centre.x - 256) < 1e-3, "the centre x");
              const back = projection.latLngForPixel(centre);
              expect.ok(
                Math.abs(back.latitude - 45.5) < 1e-6,
                "the round trip",
              );
            } finally {
              projection.close();
            }
          });
        },
      },
      {
        name: "converts to projected meters without a map",
        run({ maplibre, expect }) {
          const meters = maplibre.projectedMetersForLatLng({
            latitude: 45,
            longitude: -122,
          });
          expect.ok(
            Math.abs(meters.northing - 5_621_521.486) < 0.01,
            "the northing",
          );
          const back = maplibre.latLngForProjectedMeters(meters);
          expect.ok(Math.abs(back.latitude - 45) < 1e-6, "the round trip");
        },
      },
    ],
  },
  {
    name: "style values",
    cases: [
      {
        name: "adds and removes a source described with structured JSON",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            map.setStyleJson(EMPTY_STYLE);
            expect.ok(
              pumpFor(runtime, RuntimeEventType.mapStyleLoaded),
              "the style loaded",
            );
            map.addStyleSource(
              "added",
              jsonFrom({
                type: "geojson",
                data: { type: "FeatureCollection", features: [] },
              }),
            );
            expect.ok(map.hasStyleSource("added"), "the source is there");
            expect.ok(map.removeStyleSource("added"), "it was removed");
            expect.ok(!map.hasStyleSource("added"), "and is gone");
          });
        },
      },
      {
        name: "keeps the alternative an integer arrived as",
        run({ expect }) {
          // MapLibre reads some values only from one alternative, so a uint that
          // became a double would read as absent rather than as the same number.
          expect.ok(
            !jsonEquals(jsonUint(7n), jsonFrom(7)),
            "a uint is not a double",
          );
          expect.ok(jsonEquals(jsonUint(7n), jsonUint(7n)), "a uint is itself");
        },
      },
    ],
  },
  {
    name: "logging",
    cases: [
      {
        name: "delivers copied log records to this context",
        run({ maplibre, expect }) {
          const messages: string[] = [];
          maplibre.setLogCallback((record) => messages.push(record.message));
          try {
            withRuntime(maplibre, (runtime, open) => {
              const map = open({ width: 64, height: 64 });
              // A malformed document makes the style parser log from its own
              // thread. Reaching for the network instead would test the host's
              // HTTP transport, which a WebAssembly host supplies itself.
              map.setStyleJson('{"version":8,"sources":{},"layers":[{}]}');
              for (let attempt = 0; attempt < 40; attempt += 1) {
                runtime.pump(25);
                while (runtime.pollEvent() !== undefined) {
                  // Drained so the queue does not hold the pump open.
                }
                maplibre.deliverCallbacks();
              }
            });
            expect.ok(messages.length > 0, "a log record arrived");
            expect.ok(messages[0]!.length > 0, "it carries its message");
          } finally {
            maplibre.clearLogCallback();
          }
        },
      },
    ],
  },
  {
    name: "resources",
    cases: [
      {
        name: "serves a claimed request and passes the rest through",
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const style = EMPTY_STYLE;
            const served: string[] = [];
            runtime.setResourceProvider(
              [{ url: "served://", matchPrefix: true, useRequestedUrl: true }],
              (request) => {
                served.push(request.info.requestedUrl);
                request.complete({ bytes: new TextEncoder().encode(style) });
              },
            );
            const map = open();
            map.setStyleUrl("served://style.json");
            let loaded = false;
            for (let attempt = 0; attempt < 100 && !loaded; attempt += 1) {
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
            expect.equal(served.length, 1, "the provider was asked once");
            expect.ok(loaded, "the served style loaded");
            expect.equal(
              map.copyLoadedStyleJson(),
              style,
              "the served document",
            );
          });
        },
      },
    ],
  },
];

/** Every case, flattened, for a runner that wants one list. */
export function conformanceCases(): readonly (ConformanceCase & {
  group: string;
})[] {
  return CONFORMANCE.flatMap((group) =>
    group.cases.map((entry) => ({ ...entry, group: group.name })),
  );
}
