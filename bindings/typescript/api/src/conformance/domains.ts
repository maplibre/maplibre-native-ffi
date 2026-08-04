/**
 * Maps, styles, cameras, projections, values, logging, and resources.
 *
 * These moved out of the Node-only vitest files so every runtime proves the
 * same behavior. Cases that belong to one runtime — module formats, the
 * transport's own internals — stay beside that runtime.
 */

import { cameraOptionsEquals, copyCameraOptions } from "../camera.ts";
import { RuntimeEventType } from "../events.ts";
import { jsonEquals, jsonFrom, jsonUint } from "../json.ts";
import type { ConformanceGroup } from "./harness.ts";
import { EMPTY_STYLE, pumpFor, withRuntime } from "./harness.ts";

export const DOMAIN_GROUPS: readonly ConformanceGroup[] = [
  {
    name: "maps and styles",
    cases: [
      {
        name: "applies creation options",
        spec: ["BND-100"],
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
        spec: ["BND-042"],
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
        spec: ["BND-101"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            map.setStyleJson(EMPTY_STYLE);
            expect.defined(
              pumpFor(runtime, RuntimeEventType.mapStyleLoaded),
              "the style-loaded event",
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
        spec: ["BND-081"],
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
      {
        name: "carries no wrapper for a map that has already closed",
        spec: ["BND-086"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            map.setStyleJson(EMPTY_STYLE);
            // Pump once so the load starts, then close before draining what it
            // produced. An event for a released map names its identity and no
            // wrapper.
            runtime.pump(25);
            map.close();
            for (
              let event = runtime.pollEvent();
              event !== undefined;
              event = runtime.pollEvent()
            ) {
              expect.absent(event.map, "no wrapper for a closed map");
            }
          });
        },
      },
      {
        name: "reports the URL a style was last requested from",
        spec: ["BND-108"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime, open) => {
            const map = open();
            expect.equal(map.copyStyleUrl(), "", "no URL before a load");
            map.setStyleUrl("https://example.invalid/style.json");
            expect.equal(
              map.copyStyleUrl(),
              "https://example.invalid/style.json",
              "the requested URL",
            );
          });
        },
      },
      {
        name: "reports a map extent the C API rejects",
        spec: ["BND-104"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (runtime) => {
            const error = expect.throws(
              () => runtime.createMap({ width: 0, height: 256 }),
              "a zero-width map",
            );
            expect.equal(error.kind, "invalidArgument", "the error kind");
            expect.notEqual(error.diagnostic, "", "the native diagnostic");
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
        spec: ["BND-061"],
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
        name: "accepts a transition and brackets a gesture",
        spec: ["BND-102"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open();
            map.jumpTo({ center: { latitude: 0, longitude: 0 }, zoom: 1 });
            // A transition is a command: the call reports acceptance, and the
            // camera reaches its target as the map advances, which needs a
            // renderer. This asserts acceptance and the snapshot it started
            // from.
            map.easeTo(
              { center: { latitude: 1, longitude: 2 }, zoom: 4 },
              { durationMs: 10_000 },
            );
            expect.closeTo(map.getCamera().zoom ?? 0, 1, 3, "the start zoom");
            map.cancelTransitions();

            expect.ok(!map.isGestureInProgress(), "no gesture at rest");
            map.setGestureInProgress(true);
            expect.ok(map.isGestureInProgress(), "a gesture in progress");
            map.setGestureInProgress(false);
            expect.ok(!map.isGestureInProgress(), "the gesture ended");
          });
        },
      },
      {
        name: "keeps a copy the caller's later mutation cannot reach",
        spec: ["BND-069"],
        run({ maplibre, expect }) {
          withRuntime(maplibre, (_runtime, open) => {
            const map = open();
            // The caller keeps its own object and changes it afterwards. What
            // the map holds was copied at the boundary, so it is unaffected.
            const centre = { latitude: 10, longitude: 20 };
            const camera = { center: centre, zoom: 5 };
            map.jumpTo(camera);
            (centre as { latitude: number }).latitude = -80;
            (camera as { zoom: number }).zoom = 1;

            const held = map.getCamera();
            expect.closeTo(
              held.center?.latitude ?? 0,
              10,
              6,
              "the latitude the map kept",
            );
            expect.closeTo(held.zoom ?? 0, 5, 6, "the zoom the map kept");

            // The same holds outward: a value read back is this caller's, and
            // changing it cannot reach what the map holds.
            const read = map.getCamera();
            (read as { zoom?: number }).zoom = 99;
            expect.closeTo(
              map.getCamera().zoom ?? 0,
              5,
              6,
              "the zoom after the reader mutated its copy",
            );
          });
        },
      },
      {
        name: "compares and copies camera options by content",
        run({ expect }) {
          const camera = {
            center: { latitude: 1, longitude: 2 },
            padding: { top: 1, left: 2, bottom: 3, right: 4 },
            zoom: 3,
          };
          const copy = copyCameraOptions(camera);
          expect.ok(cameraOptionsEquals(camera, copy), "a copy compares equal");
          expect.ok(copy.center !== camera.center, "the copy is independent");
          // An absent field and a present zero are different values.
          expect.ok(
            !cameraOptionsEquals({ zoom: 0 }, {}),
            "present zero is not absent",
          );
          expect.ok(
            cameraOptionsEquals({ zoom: 0 }, { zoom: 0 }),
            "two present zeros",
          );
          expect.ok(
            !cameraOptionsEquals(camera, { ...copy, zoom: 4 }),
            "a changed field",
          );
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
        spec: ["BND-103"],
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
        name: "keeps a projection usable after its map closes",
        spec: ["BND-043"],
        run({ maplibre, expect }) {
          const runtime = maplibre.createRuntime();
          try {
            const map = runtime.createMap({ width: 256, height: 256 });
            map.jumpTo({
              center: { latitude: 45.5, longitude: -122.6 },
              zoom: 10,
            });
            const projection = map.createProjection();
            const before = projection.pixelForLatLng({
              latitude: 45.5,
              longitude: -122.6,
            });

            // A projection holds its own native handle rather than borrowing
            // the map's, so closing the map leaves it answering.
            map.close();
            const after = projection.pixelForLatLng({
              latitude: 45.5,
              longitude: -122.6,
            });
            expect.closeTo(after.x, before.x, 6, "the projected x");
            expect.closeTo(after.y, before.y, 6, "the projected y");

            projection.close();
            expect.throws(
              () => projection.pixelForLatLng({ latitude: 0, longitude: 0 }),
              "using a projection after its own release",
            );
          } finally {
            runtime.close();
          }
        },
      },
      {
        name: "converts to projected meters without a map",
        spec: ["BND-103"],
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
            expect.defined(
              pumpFor(runtime, RuntimeEventType.mapStyleLoaded),
              "the style-loaded event",
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
        spec: ["BND-142", "BND-143"],
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
