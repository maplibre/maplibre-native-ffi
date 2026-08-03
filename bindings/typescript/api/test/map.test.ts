/**
 * Maps, style loading, and camera commands, through the public API.
 */

import {
  type CameraOptions,
  cameraOptionsEquals,
  copyCameraOptions,
  MaplibreError,
  Maplibre,
  type Map,
  MapMode,
  type Runtime,
  type RuntimeEvent,
  RuntimeEventType,
} from "../src/index.ts";
import { afterEach, describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

/** A style with no sources, so a load completes without touching the network. */
const EMPTY_STYLE = JSON.stringify({
  version: 8,
  name: "empty",
  sources: {},
  layers: [],
});

let runtime: Runtime | undefined;
const maps: Map[] = [];

function open(options = { width: 256, height: 256 }): {
  runtime: Runtime;
  map: Map;
} {
  runtime ??= maplibre.createRuntime();
  const map = runtime.createMap(options);
  maps.push(map);
  return { runtime, map };
}

/** Pumps until an event of this type arrives, or the budget runs out. */
function pumpFor(
  created: Runtime,
  type: RuntimeEventType,
  attempts = 200,
): boolean {
  for (let attempt = 0; attempt < attempts; attempt += 1) {
    created.pump(25);
    for (
      let event = created.pollEvent();
      event !== undefined;
      event = created.pollEvent()
    ) {
      if (event.type.equals(type)) {
        return true;
      }
    }
  }
  return false;
}

afterEach(() => {
  // Maps keep their runtime valid, so they close first however a test ended.
  while (maps.length > 0) {
    maps.pop()!.close();
  }
  runtime?.close();
  runtime = undefined;
});

describe("a map", () => {
  it("applies its creation options and closes through its runtime", () => {
    const { map } = open({ width: 320, height: 240 });
    const size = map.getSize();
    expect(size.width).toBe(320);
    expect(size.height).toBe(240);
    expect(size.scaleFactor).toBeGreaterThan(0);
    map.close();
    expect(map.isClosed).toBe(true);
    map.close();
  });

  it("keeps its runtime open while it is live", () => {
    const { runtime: created, map } = open();
    try {
      created.close();
      expect.unreachable("a live map keeps its runtime valid");
    } catch (error) {
      expect((error as MaplibreError).kind).toBe("childrenLive");
    }
    // The runtime closes once the map does, so the retention is a count rather
    // than a permanent mark.
    map.close();
    created.close();
    expect(created.isClosed).toBe(true);
    runtime = undefined;
  });

  it("names the live map a style-loaded event came from", () => {
    const { runtime: created, map } = open();
    const other = created.createMap({ width: 64, height: 64 });
    maps.push(other);
    map.setStyleJson(EMPTY_STYLE);

    let loaded: RuntimeEvent | undefined;
    for (let attempt = 0; attempt < 200 && loaded === undefined; attempt += 1) {
      created.pump(25);
      for (
        let event = created.pollEvent();
        event !== undefined;
        event = created.pollEvent()
      ) {
        if (event.type.equals(RuntimeEventType.mapStyleLoaded)) {
          loaded = event;
          break;
        }
      }
    }

    expect(loaded).toBeDefined();
    // Two maps are live, so the event has to name the one that produced it.
    expect(loaded!.map).toBe(map);
    expect(loaded!.map).not.toBe(other);
    expect(loaded!.source?.equals(map.identity)).toBe(true);
    expect(loaded!.source?.equals(other.identity)).toBe(false);
  });

  it("carries no wrapper for a map that has already closed", () => {
    const { runtime: created, map } = open();
    map.setStyleJson(EMPTY_STYLE);
    // Pump once so the load starts, then close before draining the events it
    // produced. An event for a released map names its identity and no wrapper.
    created.pump(25);
    map.close();
    for (
      let event = created.pollEvent();
      event !== undefined;
      event = created.pollEvent()
    ) {
      expect(event.map).toBeUndefined();
    }
  });

  it("loads a style document and reads it back byte for byte", () => {
    const { runtime: created, map } = open();
    map.setStyleJson(EMPTY_STYLE);
    expect(pumpFor(created, RuntimeEventType.mapStyleLoaded)).toBe(true);
    // The C API hands back the bytes it parsed, so a document read here is the
    // document that crossed the boundary.
    expect(map.copyLoadedStyleJson()).toBe(EMPTY_STYLE);
    // No URL was requested, so the URL reads back empty even though a style
    // loaded.
    expect(map.copyStyleUrl()).toBe("");
    map.close();
  });

  it("reports the URL a style was last requested from", () => {
    const { map } = open();
    map.setStyleUrl("https://example.invalid/style.json");
    expect(map.copyStyleUrl()).toBe("https://example.invalid/style.json");
    map.close();
  });

  it("reads back a camera it was jumped to", () => {
    const { map } = open();
    const camera: CameraOptions = {
      center: { latitude: 45.5, longitude: -122.6 },
      zoom: 11,
      bearing: 30,
      pitch: 15,
    };
    map.jumpTo(camera);
    const read = map.getCamera();
    expect(read.center?.latitude).toBeCloseTo(45.5, 6);
    expect(read.center?.longitude).toBeCloseTo(-122.6, 6);
    expect(read.zoom).toBeCloseTo(11, 6);
    expect(read.bearing).toBeCloseTo(30, 6);
    expect(read.pitch).toBeCloseTo(15, 6);
    // The anchor is input-only, so a snapshot never reports one.
    expect(read.anchor).toBeUndefined();
    map.close();
  });

  it("separates an omitted camera field from one set to zero", () => {
    const { map } = open();
    map.jumpTo({
      center: { latitude: 10, longitude: 20 },
      zoom: 5,
      bearing: 40,
    });
    // Bearing is present and zero here, which the field mask has to distinguish
    // from the omitted pitch below.
    map.jumpTo({ bearing: 0 });
    const read = map.getCamera();
    expect(read.bearing).toBe(0);
    expect(read.center?.latitude).toBeCloseTo(10, 6);
    expect(read.zoom).toBeCloseTo(5, 6);
    map.close();
  });

  it("accepts an eased transition and reports the camera it started from", () => {
    const { runtime: created, map } = open();
    map.setStyleJson(EMPTY_STYLE);
    expect(pumpFor(created, RuntimeEventType.mapStyleLoaded)).toBe(true);
    map.jumpTo({ center: { latitude: 0, longitude: 0 }, zoom: 1 });
    // A transition is a command: the call reports acceptance, and the camera
    // reaches its target as the map advances, which needs a renderer. This
    // asserts acceptance and the pre-transition snapshot; the arrival event is
    // covered where a render session exists.
    map.easeTo(
      { center: { latitude: 1, longitude: 2 }, zoom: 4 },
      { durationMs: 10_000 },
    );
    expect(map.getCamera().zoom).toBeCloseTo(1, 3);
    map.cancelTransitions();
    map.close();
  });

  it("cancels transitions and brackets a gesture", () => {
    const { map } = open();
    map.flyTo(
      { center: { latitude: 3, longitude: 4 }, zoom: 6 },
      { durationMs: 5_000 },
    );
    map.cancelTransitions();
    expect(map.isGestureInProgress()).toBe(false);
    map.setGestureInProgress(true);
    expect(map.isGestureInProgress()).toBe(true);
    map.setGestureInProgress(false);
    expect(map.isGestureInProgress()).toBe(false);
    map.close();
  });

  it("propagates a native invalid-argument diagnostic", () => {
    const { map } = open();
    try {
      map.jumpTo({ zoom: Number.NaN });
      expect.unreachable("the C API rejects a non-finite zoom");
    } catch (error) {
      expect(error).toBeInstanceOf(MaplibreError);
      expect((error as MaplibreError).kind).toBe("invalidArgument");
      expect((error as MaplibreError).diagnostic).not.toBe("");
    }
    map.close();
  });

  it("reports a mode the C API rejects rather than guessing one", () => {
    const { runtime: created } = open();
    expect(() =>
      created.createMap({ width: 0, height: 256, mode: MapMode.continuous }),
    ).toThrow(MaplibreError);
  });
});

describe("camera values", () => {
  it("compares by content and copies independently", () => {
    const camera: CameraOptions = {
      center: { latitude: 1, longitude: 2 },
      padding: { top: 1, left: 2, bottom: 3, right: 4 },
      zoom: 3,
    };
    const copy = copyCameraOptions(camera);
    expect(cameraOptionsEquals(camera, copy)).toBe(true);
    expect(copy.center).not.toBe(camera.center);
    // An absent field and a present zero are different values.
    expect(cameraOptionsEquals({ zoom: 0 }, {})).toBe(false);
    expect(cameraOptionsEquals({ zoom: 0 }, { zoom: 0 })).toBe(true);
    expect(cameraOptionsEquals(camera, { ...copy, zoom: 4 })).toBe(false);
  });
});
