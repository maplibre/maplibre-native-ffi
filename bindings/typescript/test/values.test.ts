// One representative per generated value shape, against the real library.

import assert from "node:assert/strict";

import {
  CameraTransitionOutcome,
  type CameraTransitionEnd,
  CameraUpdateMode,
  geoJsonSourceDataCreate,
  type LatLng,
  MaplibreError,
  mapProjectionCreate,
  runtimeCreate,
  Status,
  styleExpressionCreate,
} from "../src/index.ts";
import { openRuntime } from "./support/fixture.ts";
import { bounded, callbackArrival, test } from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";
import { backgroundStyle } from "./support/styles.ts";

test("host strings use views and native-lent strings decode", async () => {
  // A path crosses as a view with an explicit length, and native code rejects
  // the embedded NUL where the path reaches a C-string consumer.
  assert.throws(
    () => runtimeCreate({ cachePath: "cache\u0000tail" }),
    (error) =>
      error instanceof MaplibreError &&
      error.status === Status.invalidArgument &&
      /embedded NUL/.test(error.diagnostic),
  );

  // A layer ID crosses as a view with an explicit length, so an ID with an
  // embedded NUL names that layer and not the prefix before the NUL.
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap({
    version: 8,
    sources: {},
    layers: [{ id: "a\u0000b", type: "background" }],
  });
  assert.equal((await map.getStyleLayer("a\u0000b"))?.type, "background");
  assert.equal(await map.getStyleLayer("a"), undefined);
  assert.deepEqual(
    (await map.listStyleLayers()).map((layer) => layer.id),
    ["a\u0000b"],
  );
});

test("presence fields keep a present zero apart from an absent one", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();

  const options = { durationMs: 0, enablePlacementTransitions: false };
  await bounded(map.setStyleTransitionOptions(options), "a command");
  const read = await bounded(map.getStyleTransitionOptions(), "the options");
  // An absent member stays out of the object, so the present zero round-trips
  // and the absent delay is not read back as zero.
  assert.deepEqual(read, options);
  assert.equal("delayMs" in read, false);
});

test(
  "bytes that a later option getter detaches are read as they were passed",
  async () => {
    // The options convert after the image, and their getter frees the
    // image's buffer. Native code must still read the bytes that were passed.
    const fixture = await openRuntime();
    const map = await fixture.openStyledMap();
    const pixels = new Uint8Array([1, 2, 3, 4]);
    await bounded(
      map.setStyleImage(
        "pixel",
        { width: 1, height: 1, stride: 4, pixels },
        {
          get sdf() {
            capabilities.bufferTransfer!(pixels.buffer as ArrayBuffer, 16);
            return false;
          },
        },
      ),
      "a command",
    );
    assert.equal(pixels.byteLength, 0);
    const image = await bounded(map.getStyleImage("pixel"), "the image");
    assert.deepEqual(image?.pixels, new Uint8Array([1, 2, 3, 4]));
  },
  { needs: ["bufferTransfer"] },
);

test("JSON crosses as JSON.stringify and JSON.parse values", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap(backgroundStyle("#00ff00"));

  // A value goes in as JSON.stringify writes it and comes back parsed, so a
  // style-spec string needs no inner quotes.
  await bounded(map.setGlobalStateProperty("mode", "dark"), "a command");
  assert.deepEqual(await map.getGlobalState(), { mode: "dark" });
  await bounded(
    map.setStyleLayerProperty("background", "background-opacity", 0.5),
    "a command",
  );
  assert.equal(
    await map.getStyleLayerProperty("background", "background-opacity"),
    0.5,
  );
  // null is JSON null, which restores the default, and a property without a
  // value comes back absent as undefined.
  await bounded(
    map.setStyleLayerProperty("background", "background-opacity", null),
    "a command",
  );
  assert.equal(
    await map.getStyleLayerProperty("background", "background-opacity"),
    undefined,
  );

  // A record member that is undefined is absent, an empty view.
  const expression = styleExpressionCreate(["get", "name"]);
  try {
    assert.equal(
      expression.evaluate({
        feature: {
          type: "Feature",
          properties: { name: "a" },
          geometry: { type: "Point", coordinates: [0, 0] },
        },
        featureState: undefined,
      }),
      "a",
    );
  } finally {
    expression.close();
  }

  // A value that JSON.stringify rejects throws its error before native code
  // runs, and one that it skips throws a TypeError.
  assert.throws(() => styleExpressionCreate(1n), TypeError);
  assert.throws(
    () => styleExpressionCreate(() => 0),
    (error) =>
      error instanceof TypeError &&
      error.message === "expression: expected a JSON value",
  );
});

test("integer narrowing is checked and 64-bit carriers round-trip", async () => {
  // A value outside a 32-bit field's range is rejected before it crosses,
  // rather than truncated into a valid one.
  assert.throws(
    () =>
      geoJsonSourceDataCreate(
        { type: "FeatureCollection", features: [] },
        { buffer: 2 ** 32 },
      ),
    (error) =>
      error instanceof RangeError &&
      error.message ===
        "options.buffer: expected an integer from 0 to 4294967295",
  );
  const fixture = await openRuntime();
  await assert.rejects(
    fixture.runtime.setMaximumAmbientCacheSize(-1n),
    (error) =>
      error instanceof RangeError &&
      error.message === "size: expected a bigint from 0 to 2^64-1",
  );

  // Cancelling the largest identity ends the ease that carries it, so the
  // identity crossed whole both times.
  const map = await fixture.openStyledMap();
  const maximum = 2n ** 64n - 1n;
  let end!: (value: CameraTransitionEnd) => void;
  const ended = new Promise<CameraTransitionEnd>((resolve) => (end = resolve));
  await bounded(
    map.updateCamera({
      camera: { zoom: 2 },
      mode: CameraUpdateMode.ease,
      animation: { durationMs: 60_000, transitionId: maximum, endHandler: end },
    }),
    "a command",
  );
  await bounded(map.cancelCameraTransition(maximum), "a command");
  assert.equal(
    (await callbackArrival(ended, "the end handler")).outcome,
    CameraTransitionOutcome.cancelled,
  );
});

test("an array input is copied when the call submits it", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  const points: LatLng[] = [
    { latitude: -10, longitude: -10 },
    { latitude: 10, longitude: 10 },
  ];
  const expected = await bounded(
    map.cameraForLatLngs([...points]),
    "the camera",
  );

  const fitted = map.cameraForLatLngs(points);
  points.length = 0;
  points.push({ latitude: 40, longitude: 40 });
  assert.deepEqual(await bounded(fitted, "the camera"), expected);
});

test("a synchronous call fills a caller-sized output array in input order", async () => {
  const fixture = await openRuntime();
  const map = await fixture.openMap();
  const projection = mapProjectionCreate(map.getSnapshot().view);
  try {
    const coordinates: LatLng[] = [
      { latitude: 0, longitude: 0 },
      { latitude: 10, longitude: 20 },
      { latitude: -12, longitude: -30 },
    ];
    const points = projection.pixelsForLatLngs(coordinates);
    assert.deepEqual(
      points,
      coordinates.map((coordinate) => projection.pixelForLatLng(coordinate)),
    );
    assert.deepEqual(
      projection.latLngsForPixels(points),
      points.map((point) => projection.latLngForPixel(point)),
    );
    assert.deepEqual(
      projection.latLngsForPixelsUnwrapped(points),
      points.map((point) => projection.latLngForPixelUnwrapped(point)),
    );
    assert.deepEqual(projection.pixelsForLatLngs([]), []);
    assert.deepEqual(projection.latLngsForPixels([]), []);
    assert.deepEqual(projection.latLngsForPixelsUnwrapped([]), []);
  } finally {
    projection.close();
  }
});

test("caller-sized text returns a result longer than its first storage", () => {
  const zoom = styleExpressionCreate([
    "interpolate",
    ["linear"],
    ["zoom"],
    0,
    0,
    10,
    100,
  ]);
  try {
    assert.equal(zoom.evaluate({ zoom: 5 }), 50);
  } finally {
    zoom.close();
  }

  // The result outgrows the addon's first storage, so the call runs again.
  const text = "a".repeat(1000);
  const long = styleExpressionCreate(text);
  try {
    assert.equal(long.evaluate(), text);
  } finally {
    long.close();
  }
});
