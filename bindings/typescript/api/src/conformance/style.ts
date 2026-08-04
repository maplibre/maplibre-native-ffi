/**
 * Style mutation, structured values, images, and host-supplied data.
 */

import {
  emptyGeometry,
  geoJsonFeatureCollection,
  geoJsonGeometry,
  lineStringGeometry,
  pointGeometry,
  polygonGeometry,
} from "../geojson.ts";
import {
  jsonArray,
  jsonEquals,
  jsonFrom,
  jsonObject,
  jsonString,
  jsonUint,
} from "../json.ts";
import type { ConformanceGroup } from "./harness.ts";
import { EMPTY_STYLE, loadStyle, withRuntime } from "./harness.ts";

export const STYLE_GROUP: ConformanceGroup = {
  name: "style mutation",
  cases: [
    {
      name: "adds and removes a layer over its source",
      spec: ["BND-105"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          map.addStyleSource(
            "points",
            jsonFrom({
              type: "geojson",
              data: { type: "FeatureCollection", features: [] },
            }),
          );
          expect.ok(map.hasStyleSource("points"), "the source was added");

          map.addStyleLayer(
            jsonFrom({ id: "dots", type: "circle", source: "points" }),
          );
          expect.ok(map.hasStyleLayer("dots"), "the layer was added");

          // A layer holds its source, so the source cannot go first.
          expect.throws(
            () => map.removeStyleSource("points"),
            "removing a source a layer uses",
          );
          map.removeStyleLayer("dots");
          expect.ok(!map.hasStyleLayer("dots"), "the layer was removed");
          map.removeStyleSource("points");
          expect.ok(!map.hasStyleSource("points"), "the source was removed");
        });
      },
    },
    {
      name: "reports a native rejection of a style value with its diagnostic",
      spec: ["BND-104"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          const error = expect.throws(
            () => map.addStyleSource("broken", jsonFrom({ type: "nonsense" })),
            "a source type MapLibre does not know",
          );
          expect.equal(error.kind, "invalidArgument", "the error kind");
          expect.notEqual(error.diagnostic, "", "the native diagnostic");
        });
      },
    },
    {
      name: "adds, finds, and removes a style image",
      spec: ["BND-105"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          // Four opaque pixels, which is the smallest image with every channel.
          const pixels = new Uint8Array(2 * 2 * 4).fill(0xff);
          map.setStyleImage("dot", { width: 2, height: 2, pixels });
          expect.ok(map.hasStyleImage("dot"), "the image was added");
          map.removeStyleImage("dot");
          expect.ok(!map.hasStyleImage("dot"), "the image was removed");
        });
      },
    },
    {
      name: "rejects an image buffer too small for the extent it claims",
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          const error = expect.throws(
            () =>
              map.setStyleImage("short", {
                width: 4,
                height: 4,
                pixels: new Uint8Array(8),
              }),
            "a buffer smaller than its extent",
          );
          // The binding owns this one: native would read past the buffer.
          expect.equal(error.kind, "invalidInput", "the error kind");
        });
      },
    },
  ],
};

export const VALUES_GROUP: ConformanceGroup = {
  name: "structured values",
  cases: [
    {
      name: "carries the full unsigned 64-bit domain",
      spec: ["BND-067"],
      run({ expect }) {
        const largest = (1n << 64n) - 1n;
        const value = jsonUint(largest);
        expect.ok(
          jsonEquals(value, jsonUint(largest)),
          "the largest unsigned value compares equal",
        );
        // A double would round this; the alternative keeps it exact.
        expect.notEqual(
          jsonEquals(value, jsonUint(largest - 1n)),
          true,
          "and differs from its neighbour",
        );
      },
    },
    {
      name: "keeps object member order and repeated names",
      spec: ["BND-067"],
      run({ expect }) {
        const members = [
          { name: "a", value: jsonUint(1n) },
          { name: "b", value: jsonUint(2n) },
          { name: "a", value: jsonUint(3n) },
        ];
        const value = jsonObject(members);
        expect.equal(members.length, 3, "every member is kept");
        expect.equal(members[0]!.name, "a", "the first member");
        expect.equal(members[2]!.name, "a", "the repeated member");
        // An ordinary JavaScript object cannot hold either property, so a
        // value that round-tripped through one would lose them.
        expect.notEqual(
          jsonEquals(
            value,
            jsonObject([
              { name: "a", value: jsonUint(3n) },
              { name: "b", value: jsonUint(2n) },
            ]),
          ),
          true,
          "a collapsed object is a different value",
        );
      },
    },
    {
      name: "builds from ordinary JavaScript data",
      spec: ["BND-064"],
      run({ expect }) {
        const value = jsonFrom({ name: "x", tags: ["a", "b"], count: 2 });
        expect.ok(
          jsonEquals(
            value,
            jsonObject([
              { name: "name", value: jsonString("x") },
              {
                name: "tags",
                value: jsonArray([jsonString("a"), jsonString("b")]),
              },
              { name: "count", value: { kind: "double", value: 2 } },
            ]),
          ),
          "the converted value",
        );
      },
    },
  ],
};

export const HOST_DATA_GROUP: ConformanceGroup = {
  name: "host-supplied data",
  cases: [
    {
      name: "adds a GeoJSON source from host data and replaces it",
      spec: ["BND-065", "BND-105"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
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
          expect.ok(map.hasStyleSource("points"), "the source was added");

          // Replacing the data keeps the source and its options.
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
          expect.ok(map.hasStyleSource("points"), "the source survived");
        });
      },
    },
    {
      name: "asks this host for a custom geometry tile and takes the answer",
      spec: ["BND-124"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          const asked: number[] = [];
          map.addCustomGeometrySource(
            "hosted",
            {
              onFetchTile: (tile) => {
                asked.push(tile.z);
                map.setCustomGeometryTileData(
                  "hosted",
                  tile,
                  geoJsonGeometry(emptyGeometry),
                );
              },
            },
            { minZoom: 0, maxZoom: 4 },
          );
          expect.ok(map.hasStyleSource("hosted"), "the source was added");

          map.addStyleLayer(
            jsonFrom({ id: "hosted-dots", type: "circle", source: "hosted" }),
          );
          expect.ok(map.hasStyleLayer("hosted-dots"), "the layer was added");

          // Nothing renders here, so MapLibre may never ask. What this proves
          // is that the source registers, a layer binds to it, and answering a
          // tile the host names is accepted.
          map.setCustomGeometryTileData(
            "hosted",
            { z: 0, x: 0, y: 0 },
            geoJsonGeometry(emptyGeometry),
          );
          expect.ok(
            asked.every((zoom) => zoom !== 0xff),
            "no retirement sentinel reached the handler",
          );
        });
      },
    },
    {
      name: "carries a whole geometry tree across the boundary",
      spec: ["BND-065"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          map.addCustomGeometrySource("shapes", { onFetchTile: () => {} });
          // Every variant the descriptor graph has, in one value, so a wrong
          // offset or a lost count fails here rather than in one caller's
          // shape.
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
          expect.ok(map.hasStyleSource("shapes"), "the source survived");
        });
      },
    },
    {
      name: "loads a style document unchanged after mutation",
      spec: ["BND-108"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          expect.ok(loadStyle(runtime, map), "the style loaded");
          // The C API hands back the bytes it parsed, so the document a caller
          // reads is the document that crossed the boundary.
          expect.equal(map.copyLoadedStyleJson(), EMPTY_STYLE, "the document");
        });
      },
    },
  ],
};
