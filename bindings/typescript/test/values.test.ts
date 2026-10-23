// One representative per generated value shape, against the real library.

import assert from "node:assert/strict";

import {
  CameraTransitionOutcome,
  type CameraTransitionEnd,
  CameraUpdateMode,
  geojsonSourceDataCreate,
  type LatLng,
  MaplibreError,
  mapOptionsDefault,
  mapProjectionCreate,
  runtimeCreate,
  Status,
  styleExpressionCreate,
} from "../src/index.ts";
import { expectCommitted, openRuntime, utf8 } from "./support/fixture.ts";
import { bounded, callbackArrival, test } from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";

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
  const map = await fixture.openStyledMap(
    JSON.stringify({
      version: 8,
      sources: {},
      layers: [{ id: "a\u0000b", type: "background" }],
    }),
  );
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
  await expectCommitted(map.setStyleTransitionOptions(options));
  const read = await bounded(map.getStyleTransitionOptions(), "the options");
  // An absent member stays out of the object, so the present zero round-trips
  // and the absent delay is not read back as zero.
  assert.deepEqual(read, options);
  assert.equal("delayMs" in read, false);
});

test(
  "bytes that a later option getter detaches are read as they were passed",
  () => {
    // The options convert after the data, and their getter frees the data's
    // buffer. Native code must still read the bytes that were passed.
    const bytes = utf8('{"type":"FeatureCollection","features":[]}');
    const data = geojsonSourceDataCreate(bytes, {
      get minZoom() {
        capabilities.bufferTransfer!(bytes.buffer as ArrayBuffer, 16);
        return 0;
      },
    });
    assert.equal(bytes.byteLength, 0);
    data.close();
  },
  { needs: ["bufferTransfer"] },
);

test("integer narrowing is checked and 64-bit carriers round-trip", async () => {
  // A value outside a 32-bit field's range is rejected before it crosses,
  // rather than truncated into a valid one.
  assert.throws(
    () =>
      geojsonSourceDataCreate(
        utf8('{"type":"FeatureCollection","features":[]}'),
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
  await expectCommitted(
    map.updateCamera({
      camera: { zoom: 2 },
      mode: CameraUpdateMode.ease,
      animation: { durationMs: 60_000, transitionId: maximum, endHandler: end },
    }),
  );
  await expectCommitted(map.cancelCameraTransition(maximum));
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

test("a record with omitted members equals the native default", async () => {
  // Every record converter starts from the native default function, so an
  // empty record and the default's copy configure the same map.
  const fixture = await openRuntime();
  const omitted = await fixture.openMap({});
  const defaulted = await fixture.openMap(mapOptionsDefault());
  const { generation: _first, ...fromOmitted } = omitted.getSnapshot();
  const { generation: _second, ...fromDefault } = defaulted.getSnapshot();
  assert.deepEqual(fromOmitted, fromDefault);
  assert.deepEqual(fromOmitted.view.extent, mapOptionsDefault().initialExtent);
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
  using zoom = styleExpressionCreate(
    '["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]',
  );
  assert.equal(zoom.evaluate({ zoom: 5 }), "50.0");

  // The result outgrows the addon's first storage, so the call runs again.
  const text = `"${"a".repeat(1000)}"`;
  using long = styleExpressionCreate(text);
  assert.equal(long.evaluate(), text);
});
