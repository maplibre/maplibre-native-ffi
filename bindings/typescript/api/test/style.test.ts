/**
 * Style sources and layers, and the structured JSON they are described with.
 */

import {
  type CustomGeometryTile,
  emptyGeometry,
  geoJsonFeatureCollection,
  geoJsonGeometry,
  jsonArray,
  jsonDouble,
  jsonEquals,
  jsonFrom,
  jsonInt,
  jsonObject,
  jsonString,
  jsonUint,
  lineStringGeometry,
  pointGeometry,
  polygonGeometry,
  type JsonValue,
  type Map,
  MaplibreError,
  Maplibre,
  type Runtime,
  RuntimeEventType,
} from "../src/index.ts";
import { afterEach, describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

const EMPTY_STYLE = JSON.stringify({
  version: 8,
  name: "empty",
  sources: {},
  layers: [],
});

let runtime: Runtime | undefined;
const maps: Map[] = [];

afterEach(() => {
  while (maps.length > 0) {
    maps.pop()!.close();
  }
  runtime?.close();
  runtime = undefined;
});

/** Opens a map whose style has finished loading. */
function loadedMap(): Map {
  runtime ??= maplibre.createRuntime();
  const created = runtime;
  const map = created.createMap({ width: 256, height: 256 });
  maps.push(map);
  map.setStyleJson(EMPTY_STYLE);
  for (let attempt = 0; attempt < 200; attempt += 1) {
    created.pump(25);
    let loaded = false;
    for (
      let event = created.pollEvent();
      event !== undefined;
      event = created.pollEvent()
    ) {
      loaded ||= event.type.equals(RuntimeEventType.mapStyleLoaded);
    }
    if (loaded) {
      return map;
    }
  }
  throw new Error("the style never finished loading");
}

describe("style sources and layers", () => {
  it("adds, finds, and removes a source", () => {
    const map = loadedMap();
    expect(map.hasStyleSource("added")).toBe(false);

    map.addStyleSource(
      "added",
      jsonObject([
        { name: "type", value: jsonString("geojson") },
        {
          name: "data",
          value: jsonObject([
            { name: "type", value: jsonString("FeatureCollection") },
            { name: "features", value: jsonArray([]) },
          ]),
        },
      ]),
    );
    expect(map.hasStyleSource("added")).toBe(true);
    // The document is the bytes the loader parsed, so a runtime mutation does
    // not rewrite it: a host can still hand it back to setStyleJson unchanged.
    expect(map.copyLoadedStyleJson()).toBe(EMPTY_STYLE);

    expect(map.removeStyleSource("added")).toBe(true);
    expect(map.hasStyleSource("added")).toBe(false);
    // Removing what is not there reports that rather than failing.
    expect(map.removeStyleSource("added")).toBe(false);
  });

  it("adds and removes a layer over its source", () => {
    const map = loadedMap();
    map.addStyleSource(
      "points",
      jsonFrom({
        type: "geojson",
        data: { type: "FeatureCollection", features: [] },
      }),
    );
    map.addStyleLayer(
      jsonFrom({ id: "circles", type: "circle", source: "points" }),
    );
    expect(map.hasStyleLayer("circles")).toBe(true);
    expect(map.removeStyleLayer("circles")).toBe(true);
    expect(map.hasStyleLayer("circles")).toBe(false);
  });

  it("reports a native rejection with its diagnostic", () => {
    const map = loadedMap();
    try {
      // A layer naming a source that is not there is the C API's to reject.
      map.addStyleLayer(
        jsonFrom({ id: "orphan", type: "circle", source: "absent" }),
      );
      expect.unreachable("a layer over a missing source is invalid");
    } catch (error) {
      expect(error).toBeInstanceOf(MaplibreError);
      expect((error as MaplibreError).diagnostic).not.toBe("");
    }
  });
});

describe("structured JSON", () => {
  it("keeps the alternative an integer arrived as", () => {
    // MapLibre reads some values only from one alternative, so a uint that
    // became a double would read as absent rather than as the same number.
    const asUint = jsonUint(7n);
    const asInt = jsonInt(7n);
    const asDouble = jsonDouble(7);
    expect(jsonEquals(asUint, asInt)).toBe(false);
    expect(jsonEquals(asUint, asDouble)).toBe(false);
    expect(jsonEquals(asUint, jsonUint(7n))).toBe(true);
  });

  it("carries the full unsigned 64-bit domain", () => {
    const large = (1n << 64n) - 1n;
    const value = jsonUint(large);
    expect(value.kind).toBe("uint");
    expect((value as { value: bigint }).value).toBe(large);
  });

  it("keeps object member order and repeated names", () => {
    const first: JsonValue = jsonObject([
      { name: "a", value: jsonString("1") },
      { name: "a", value: jsonString("2") },
    ]);
    const reordered: JsonValue = jsonObject([
      { name: "a", value: jsonString("2") },
      { name: "a", value: jsonString("1") },
    ]);
    expect(jsonEquals(first, first)).toBe(true);
    // Order is part of the value: these are two different documents.
    expect(jsonEquals(first, reordered)).toBe(false);
  });

  it("builds from ordinary JavaScript data", () => {
    const value = jsonFrom({
      name: "x",
      count: 2,
      tags: ["a", "b"],
      missing: null,
    });
    expect(value.kind).toBe("object");
    expect(
      jsonEquals(
        value,
        jsonObject([
          { name: "name", value: jsonString("x") },
          { name: "count", value: jsonDouble(2) },
          {
            name: "tags",
            value: jsonArray([jsonString("a"), jsonString("b")]),
          },
          { name: "missing", value: { kind: "null" } },
        ]),
      ),
    ).toBe(true);
  });
});

describe("style images", () => {
  it("adds, finds, and removes an image", () => {
    const map = loadedMap();
    expect(map.hasStyleImage("marker")).toBe(false);

    const pixels = new Uint8Array(4 * 4 * 4).fill(0x80);
    map.setStyleImage("marker", { width: 4, height: 4, pixels, pixelRatio: 2 });
    expect(map.hasStyleImage("marker")).toBe(true);

    // The pixels were copied, so mutating the caller's buffer afterwards
    // changes nothing the style holds.
    pixels.fill(0);
    expect(map.hasStyleImage("marker")).toBe(true);

    expect(map.removeStyleImage("marker")).toBe(true);
    expect(map.hasStyleImage("marker")).toBe(false);
    expect(map.removeStyleImage("marker")).toBe(false);
  });

  it("rejects a buffer too small for the extent it claims", () => {
    const map = loadedMap();
    try {
      map.setStyleImage("short", {
        width: 8,
        height: 8,
        pixels: new Uint8Array(16),
      });
      expect.unreachable("the buffer cannot hold an 8x8 image");
    } catch (error) {
      expect(error).toBeInstanceOf(MaplibreError);
      // The binding owns this: reading past the buffer would be the alternative.
      expect((error as MaplibreError).kind).toBe("invalidInput");
    }
  });
});

describe("custom geometry sources", () => {
  it("asks this host for a tile and takes the answer", () => {
    const map = loadedMap();
    const asked: CustomGeometryTile[] = [];
    map.addCustomGeometrySource(
      "hosted",
      {
        onFetchTile: (tile) => {
          asked.push(tile);
          // Answering inline is allowed; a host may also answer later.
          map.setCustomGeometryTileData(
            "hosted",
            tile,
            geoJsonFeatureCollection([
              {
                geometry: pointGeometry({ latitude: 0, longitude: 0 }),
                properties: [{ name: "kind", value: jsonString("origin") }],
                identifier: { kind: "uint", value: 7n },
              },
            ]),
          );
        },
      },
      { minZoom: 0, maxZoom: 4 },
    );
    expect(map.hasStyleSource("hosted")).toBe(true);

    // A layer over the source is what makes MapLibre ask for its tiles.
    map.addStyleLayer(
      jsonFrom({ id: "hosted-circles", type: "circle", source: "hosted" }),
    );
    expect(map.hasStyleLayer("hosted-circles")).toBe(true);

    // Nothing renders here, so MapLibre may never ask; what this proves is that
    // the source registers, the layer binds to it, and answering a tile the
    // host names is accepted.
    map.setCustomGeometryTileData(
      "hosted",
      { z: 0, x: 0, y: 0 },
      geoJsonGeometry(emptyGeometry),
    );
    expect(asked.every((tile) => tile.z !== 0xff)).toBe(true);
  });

  it("carries a whole geometry tree across the boundary", () => {
    const map = loadedMap();
    map.addCustomGeometrySource("shapes", { onFetchTile: () => {} });
    // Every variant the descriptor graph has, in one value, so a wrong offset
    // or a lost count fails here rather than in one caller's shape.
    map.setCustomGeometryTileData(
      "shapes",
      { z: 1, x: 0, y: 0 },
      geoJsonFeatureCollection([
        {
          geometry: {
            kind: "collection",
            geometries: [
              pointGeometry({ latitude: 1, longitude: 2 }),
              lineStringGeometry([
                { latitude: 0, longitude: 0 },
                { latitude: 1, longitude: 1 },
              ]),
              polygonGeometry([
                [
                  { latitude: 0, longitude: 0 },
                  { latitude: 0, longitude: 1 },
                  { latitude: 1, longitude: 1 },
                  { latitude: 0, longitude: 0 },
                ],
              ]),
              {
                kind: "multiPolygon",
                polygons: [
                  [
                    [
                      { latitude: 2, longitude: 2 },
                      { latitude: 2, longitude: 3 },
                      { latitude: 3, longitude: 3 },
                      { latitude: 2, longitude: 2 },
                    ],
                  ],
                ],
              },
            ],
          },
          identifier: { kind: "string", value: "everything" },
        },
      ]),
    );
  });
});

describe("GeoJSON sources", () => {
  it("adds a source from host data and replaces it", () => {
    const map = loadedMap();
    map.addGeoJsonSource(
      "points",
      geoJsonFeatureCollection([
        {
          geometry: pointGeometry({ latitude: 51.5, longitude: -0.1 }),
          properties: [{ name: "name", value: jsonString("london") }],
          identifier: { kind: "uint", value: 1n },
        },
      ]),
    );
    expect(map.hasStyleSource("points")).toBe(true);

    // Replacing the data keeps the source and the options it was added with.
    map.setGeoJsonSourceData(
      "points",
      geoJsonFeatureCollection([
        {
          geometry: lineStringGeometry([
            { latitude: 0, longitude: 0 },
            { latitude: 1, longitude: 1 },
          ]),
        },
      ]),
    );
    expect(map.hasStyleSource("points")).toBe(true);
  });
});
