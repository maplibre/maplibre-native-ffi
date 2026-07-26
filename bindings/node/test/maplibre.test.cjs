const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");
const path = require("node:path");
const test = require("node:test");
const { Worker } = require("node:worker_threads");

process.env.MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS = "1";

const {
  AnimationOptions,
  BoundOptions,
  CameraFitOptions,
  CameraOptions,
  clearLogCallback,
  cVersion,
  FreeCameraOptions,
  InvalidArgumentError,
  InvalidStateError,
  MaplibreError,
  MapHandle,
  MapOptions,
  MapProjectionHandle,
  MapTileOptions,
  MapViewportOptions,
  latLngForProjectedMeters,
  MaplibreStatus,
  MetalOwnedTextureFrame,
  OpenGLOwnedTextureFrame,
  NativeBuffer,
  NativePointer,
  OfflineOperationHandle,
  ResourceRequestHandle,
  networkStatus,
  projectedMetersForLatLng,
  ProjectionMode,
  RenderedFeatureQueryOptions,
  restoreDefaultAsyncLogSeverities,
  renderTargetExtentPhysicalSize,
  RenderSessionHandle,
  RuntimeHandle,
  RuntimeOptions,
  setAsyncLogSeverities,
  VulkanOwnedTextureFrame,
  setLogCallback,
  setNetworkStatus,
  SourceFeatureQueryOptions,
  StyleImageOptions,
  supportedRenderBackends,
  supportedOpenGLContextProviders,
  takeNativeLeakReports,
  threadLastErrorMessage,
  TileSourceOptions,
} = require("..");
const nativeAddon = require("../index.js");

const EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}';

/**
 * @param {string} completionToken
 * @param {string} url
 * @returns {any}
 */
function fakeResourceProviderRequest(completionToken, url) {
  return {
    url,
    kind: "source",
    rawKind: 2,
    loadingMethod: "all",
    rawLoadingMethod: 0,
    priority: "regular",
    rawPriority: 0,
    usage: "online",
    rawUsage: 0,
    storagePolicy: "volatile",
    rawStoragePolicy: 1,
    range: null,
    priorModifiedUnixMs: null,
    priorExpiresUnixMs: null,
    priorEtag: null,
    priorData: new Uint8Array(),
    completionToken,
  };
}

test("concept subpath modules expose curated public API groups", () => {
  const packageJson = require("../package.json");
  for (const subpath of Object.keys(packageJson.exports)) {
    require(
      subpath === "."
        ? "@maplibre/native-ffi-node"
        : `@maplibre/native-ffi-node${subpath.slice(1)}`,
    );
  }

  const runtimeModule = require("@maplibre/native-ffi-node/runtime");
  const renderModule = require("@maplibre/native-ffi-node/render");
  const errorModule = require("@maplibre/native-ffi-node/error");
  const geoModule = require("@maplibre/native-ffi-node/geo");
  const logModule = require("@maplibre/native-ffi-node/log");
  const mapModule = require("@maplibre/native-ffi-node/map");
  const offlineModule = require("@maplibre/native-ffi-node/offline");
  const resourceModule = require("@maplibre/native-ffi-node/resource");

  assert.equal(runtimeModule.RuntimeHandle, RuntimeHandle);
  assert.equal(runtimeModule.RuntimeOptions, RuntimeOptions);
  assert.equal(runtimeModule.networkStatus, networkStatus);
  assert.equal(
    runtimeModule.supportedOpenGLContextProviders,
    supportedOpenGLContextProviders,
  );
  assert.equal(renderModule.RenderSessionHandle, RenderSessionHandle);
  assert.equal(
    renderModule.RenderedFeatureQueryOptions,
    RenderedFeatureQueryOptions,
  );
  assert.equal(renderModule.NativeBuffer, NativeBuffer);
  assert.equal(errorModule.InvalidArgumentError, InvalidArgumentError);
  assert.equal(geoModule.projectedMetersForLatLng, projectedMetersForLatLng);
  assert.equal(logModule.setLogCallback, setLogCallback);
  assert.equal(mapModule.MapHandle, MapHandle);
  assert.equal(mapModule.MapOptions, MapOptions);
  assert.equal(offlineModule.OfflineOperationHandle, OfflineOperationHandle);
  assert.equal(resourceModule.ResourceRequestHandle, ResourceRequestHandle);
});

test("option values compare and copy every semantic field", () => {
  const bounds = {
    southwest: { latitude: 1, longitude: 2 },
    northeast: { latitude: 3, longitude: 4 },
  };
  const padding = { top: 1, left: 2, bottom: 3, right: 4 };
  /** @type {Array<[new (input?: any) => any, Record<string, any>, Record<string, any>]>} */
  const cases = [
    [
      RuntimeOptions,
      { assetPath: "asset", cachePath: "cache", maximumCacheSize: 1n },
      { assetPath: "other", cachePath: "other", maximumCacheSize: 2n },
    ],
    [
      MapOptions,
      { width: 16, height: 8, scaleFactor: 1, mapMode: "static" },
      { width: 32, height: 4, scaleFactor: 2, mapMode: "tile" },
    ],
    [
      CameraOptions,
      {
        center: { latitude: 1, longitude: 2 },
        zoom: 3,
        bearing: 4,
        pitch: 5,
        centerAltitude: 6,
        padding,
        anchor: { x: 7, y: 8 },
        roll: 9,
        fieldOfView: 10,
      },
      {
        center: { latitude: 11, longitude: 12 },
        zoom: 13,
        bearing: 14,
        pitch: 15,
        centerAltitude: 16,
        padding: { ...padding, top: 10 },
        anchor: { x: 17, y: 18 },
        roll: 19,
        fieldOfView: 20,
      },
    ],
    [
      AnimationOptions,
      {
        durationMs: 1,
        velocity: 2,
        minZoom: 3,
        easing: { x1: 0, y1: 0, x2: 1, y2: 1 },
      },
      {
        durationMs: 4,
        velocity: 5,
        minZoom: 6,
        easing: { x1: 0.1, y1: 0.2, x2: 0.8, y2: 0.9 },
      },
    ],
    [
      CameraFitOptions,
      { padding, bearing: 5, pitch: 6 },
      { padding: { ...padding, left: 10 }, bearing: 7, pitch: 8 },
    ],
    [
      FreeCameraOptions,
      {
        position: { x: 1, y: 2, z: 3 },
        orientation: { x: 0, y: 0, z: 0, w: 1 },
      },
      {
        position: { x: 4, y: 5, z: 6 },
        orientation: { x: 1, y: 0, z: 0, w: 0 },
      },
    ],
    [
      BoundOptions,
      { bounds, minZoom: 5, maxZoom: 6, minPitch: 7, maxPitch: 8 },
      {
        bounds: { ...bounds, northeast: { latitude: 30, longitude: 40 } },
        minZoom: 9,
        maxZoom: 10,
        minPitch: 11,
        maxPitch: 12,
      },
    ],
    [
      MapViewportOptions,
      {
        northOrientation: "up",
        northOrientationRaw: 0,
        constrainMode: "none",
        constrainModeRaw: 0,
        viewportMode: "default",
        viewportModeRaw: 0,
        frustumOffset: padding,
      },
      {
        northOrientation: "right",
        northOrientationRaw: 1,
        constrainMode: "screen",
        constrainModeRaw: 3,
        viewportMode: "flippedY",
        viewportModeRaw: 1,
        frustumOffset: { ...padding, bottom: 10 },
      },
    ],
    [
      MapTileOptions,
      {
        prefetchZoomDelta: 1,
        lodMinRadius: 2,
        lodScale: 3,
        lodPitchThreshold: 4,
        lodZoomShift: 5,
        lodMode: "default",
        lodModeRaw: 0,
      },
      {
        prefetchZoomDelta: 6,
        lodMinRadius: 7,
        lodScale: 8,
        lodPitchThreshold: 9,
        lodZoomShift: 10,
        lodMode: "distance",
        lodModeRaw: 1,
      },
    ],
    [
      ProjectionMode,
      { axonometric: false, xSkew: 1, ySkew: 2 },
      { axonometric: true, xSkew: 3, ySkew: 4 },
    ],
    [
      TileSourceOptions,
      {
        minZoom: 1,
        maxZoom: 2,
        attribution: "one",
        scheme: "xyz",
        bounds,
        tileSize: 256,
        vectorEncoding: "mvt",
        rasterDemEncoding: "mapbox",
      },
      {
        minZoom: 3,
        maxZoom: 4,
        attribution: "two",
        scheme: "tms",
        bounds: { ...bounds, southwest: { latitude: 10, longitude: 20 } },
        tileSize: 512,
        vectorEncoding: "mlt",
        rasterDemEncoding: "terrarium",
      },
    ],
    [
      RenderedFeatureQueryOptions,
      { layerIds: ["one"], filter: ["==", "kind", "one"] },
      { layerIds: ["two"], filter: ["==", "kind", "two"] },
    ],
    [
      SourceFeatureQueryOptions,
      { sourceLayerIds: ["one"], filter: ["==", "kind", "one"] },
      { sourceLayerIds: ["two"], filter: ["==", "kind", "two"] },
    ],
    [
      StyleImageOptions,
      { pixelRatio: 1, sdf: false },
      { pixelRatio: 2, sdf: true },
    ],
  ];

  for (const [Option, input, alternatives] of cases) {
    const value = new Option(input);
    const equal = new Option(input);
    assert.equal(value.equals(equal), true, Option.name);
    assert.equal(equal.equals(value), true, Option.name);
    assert.equal(value.equals({ ...input }), false, Option.name);
    const copy = value.copy();
    assert.notEqual(copy, value, Option.name);
    assert.equal(value.equals(copy), true, Option.name);
    for (const [field, alternative] of Object.entries(alternatives)) {
      assert.equal(
        value.equals(value.copy({ [field]: alternative })),
        false,
        `${Option.name}.${field}`,
      );
    }
    const [firstField] = Object.keys(input);
    assert.equal(
      new Option().equals(new Option({ [firstField]: null })),
      false,
      `${Option.name} absent/null`,
    );
  }

  const input = {
    layerIds: ["layer"],
    filter: ["==", "kind", { nested: ["original"] }],
  };
  const value = new RenderedFeatureQueryOptions(input);
  input.layerIds[0] = "mutated";
  /** @type {any} */ (input.filter[2]).nested[0] = "mutated";
  assert.equal(
    value.equals(
      new RenderedFeatureQueryOptions({
        layerIds: ["layer"],
        filter: ["==", "kind", { nested: ["original"] }],
      }),
    ),
    true,
  );
  const copy = value.copy();
  assert.notEqual(copy.layerIds, value.layerIds);
  assert.notEqual(copy.filter, value.filter);
});

test("map handles are created only by their owning factories", () => {
  assert.throws(
    () => Reflect.construct(MapHandle, [null]),
    InvalidArgumentError,
  );
  assert.throws(
    () => Reflect.construct(MapProjectionHandle, [null]),
    InvalidArgumentError,
  );
});

test("process-global APIs cross the native add-on", () => {
  assert.equal(cVersion(), 0);

  const backends = supportedRenderBackends();
  assert.equal(typeof backends.rawMask, "number");
  assert.equal(typeof backends.metal, "boolean");
  assert.equal(typeof backends.vulkan, "boolean");
  assert.equal(typeof backends.opengl, "boolean");

  const openglProviders = supportedOpenGLContextProviders();
  assert.equal(typeof openglProviders.rawMask, "number");
  assert.equal(typeof openglProviders.wgl, "boolean");
  assert.equal(typeof openglProviders.egl, "boolean");

  assert.equal(typeof threadLastErrorMessage(), "string");
  assert.deepEqual(
    renderTargetExtentPhysicalSize({
      width: 65,
      height: 33,
      scaleFactor: 1.5,
    }),
    { width: 98, height: 50 },
  );
  assert.deepEqual(takeNativeLeakReports(), []);

  const original = networkStatus();
  assert.match(original.kind, /^(online|offline|unknown)$/);

  setNetworkStatus("online");
  assert.equal(networkStatus().kind, "online");

  setNetworkStatus("offline");
  assert.equal(networkStatus().kind, "offline");

  setNetworkStatus({ kind: "unknown", raw: 1 });
  assert.equal(networkStatus().kind, "online");

  setNetworkStatus(original);
});

test("projection helpers round trip copied coordinate values", () => {
  const coordinate = { latitude: 45, longitude: -122 };
  const meters = projectedMetersForLatLng(coordinate);
  const roundTripped = latLngForProjectedMeters(meters);

  assert.equal(typeof meters.northing, "number");
  assert.equal(typeof meters.easting, "number");
  assert.ok(Math.abs(roundTripped.latitude - coordinate.latitude) < 1e-9);
  assert.ok(Math.abs(roundTripped.longitude - coordinate.longitude) < 1e-9);
});

test("log callback copies records through the Node event loop", async () => {
  /** @type {import("..").LogRecord[]} */
  const records = [];
  setLogCallback((record) => records.push(record));
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.dumpDebugLogs();
    await eventually(() => records.length > 0);
    assert.equal(typeof records[0].message, "string");
    assert.equal(typeof records[0].rawSeverity, "number");
    assert.equal(typeof records[0].rawEvent, "number");
    assert.equal(typeof records[0].code, "bigint");
  } finally {
    map.close();
    runtime.close();
    clearLogCallback();
  }
});

test("cleared and replaced log callbacks discard queued records", () => {
  const originalSetLogCallback = nativeAddon.nativeSetLogCallback;
  const originalClearLogCallback = nativeAddon.nativeClearLogCallback;
  /** @type {Function[]} */
  const bridges = [];
  nativeAddon.nativeSetLogCallback = (callback) => bridges.push(callback);
  nativeAddon.nativeClearLogCallback = () => {};
  let firstRecords = 0;
  let secondRecords = 0;

  try {
    setLogCallback(() => {
      firstRecords += 1;
    });
    setLogCallback(() => {
      secondRecords += 1;
    });
    bridges[0](null, { code: 1n });
    bridges[1](null, { code: 2n });
    assert.equal(firstRecords, 0);
    assert.equal(secondRecords, 1);
    clearLogCallback();
    bridges[1](null, { code: 3n });
    assert.equal(secondRecords, 1);
  } finally {
    nativeAddon.nativeSetLogCallback = originalSetLogCallback;
    nativeAddon.nativeClearLogCallback = originalClearLogCallback;
    clearLogCallback();
  }
});

test("binding-managed callbacks contain user exceptions", () => {
  const originalSetLogCallback = nativeAddon.nativeSetLogCallback;
  /** @type {undefined | ((error: Error | null, record: import("..").LogRecord) => void)} */
  let logBridge;
  /** @param {(error: Error | null, record: import("..").LogRecord) => void} callback */
  nativeAddon.nativeSetLogCallback = (callback) => {
    logBridge = callback;
  };

  try {
    setLogCallback(() => {
      throw new Error("log callback failed");
    });
    assert.ok(logBridge);
    assert.doesNotThrow(() =>
      logBridge?.(
        null,
        /** @type {import("..").LogRecord} */ ({ message: "hello" }),
      ),
    );
  } finally {
    nativeAddon.nativeSetLogCallback = originalSetLogCallback;
    clearLogCallback();
  }

  /** @type {{ fetch?: Function | null, cancel?: Function | null }} */
  const callbacks = {};
  MapHandle.prototype.addCustomGeometrySource.call(
    {
      native: {
        closed: false,
        /**
         * @param {string} _sourceId
         * @param {unknown} _options
         * @param {Function | null} fetchTile
         * @param {Function | null} cancelTile
         */
        addCustomGeometrySource(_sourceId, _options, fetchTile, cancelTile) {
          callbacks.fetch = fetchTile;
          callbacks.cancel = cancelTile;
        },
      },
    },
    "custom",
    {
      fetchTile() {
        throw new Error("fetch failed");
      },
      cancelTile() {
        throw new Error("cancel failed");
      },
    },
  );
  assert.doesNotThrow(() => callbacks.fetch?.(null, { z: 0, x: 0, y: 0 }));
  assert.doesNotThrow(() => callbacks.cancel?.(null, { z: 0, x: 0, y: 0 }));
});

test("binding-managed callbacks contain rejected promises", async () => {
  const originalSetLogCallback = nativeAddon.nativeSetLogCallback;
  /** @type {undefined | ((error: Error | null, record: import("..").LogRecord) => void)} */
  let logBridge;
  /** @param {(error: Error | null, record: import("..").LogRecord) => void} callback */
  nativeAddon.nativeSetLogCallback = (callback) => {
    logBridge = callback;
  };

  /** @type {{ fetch?: Function | null, cancel?: Function | null }} */
  const callbacks = {};
  try {
    setLogCallback(async () => {
      throw new Error("async log callback failed");
    });
    MapHandle.prototype.addCustomGeometrySource.call(
      {
        native: {
          closed: false,
          /**
           * @param {string} _sourceId
           * @param {unknown} _options
           * @param {Function | null} fetchTile
           * @param {Function | null} cancelTile
           */
          addCustomGeometrySource(_sourceId, _options, fetchTile, cancelTile) {
            callbacks.fetch = fetchTile;
            callbacks.cancel = cancelTile;
          },
        },
      },
      "custom",
      {
        async fetchTile() {
          throw new Error("async fetch failed");
        },
        async cancelTile() {
          throw new Error("async cancel failed");
        },
      },
    );

    logBridge?.(
      null,
      /** @type {import("..").LogRecord} */ ({ message: "hello" }),
    );
    callbacks.fetch?.(null, { z: 0, x: 0, y: 0 });
    callbacks.cancel?.(null, { z: 0, x: 0, y: 0 });
    await new Promise((resolve) => setImmediate(resolve));
  } finally {
    nativeAddon.nativeSetLogCallback = originalSetLogCallback;
    clearLogCallback();
  }
});

test("retired custom geometry sources discard queued callbacks", () => {
  /** @type {Function[]} */
  const fetchBridges = [];
  const fakeMap = {
    native: {
      closed: false,
      /** @param {string} _sourceId @param {unknown} _options @param {Function} fetchTile */
      addCustomGeometrySource(_sourceId, _options, fetchTile) {
        fetchBridges.push(fetchTile);
      },
      removeStyleSource() {
        return true;
      },
    },
  };
  let firstFetches = 0;
  let secondFetches = 0;

  MapHandle.prototype.addCustomGeometrySource.call(fakeMap, "custom", {
    fetchTile() {
      firstFetches += 1;
    },
  });
  assert.equal(
    MapHandle.prototype.removeStyleSource.call(fakeMap, "custom"),
    true,
  );
  MapHandle.prototype.addCustomGeometrySource.call(fakeMap, "custom", {
    fetchTile() {
      secondFetches += 1;
    },
  });

  fetchBridges[0](null, { z: 0, x: 0, y: 0 });
  fetchBridges[1](null, { z: 0, x: 0, y: 0 });
  assert.equal(firstFetches, 0);
  assert.equal(secondFetches, 1);

  /** @type {Function[]} */
  const urlFetchBridges = [];
  const urlMap = Object.create(MapHandle.prototype);
  Object.defineProperty(urlMap, "native", {
    value: {
      closed: false,
      /** @param {string} _sourceId @param {unknown} _options @param {Function} fetchTile */
      addCustomGeometrySource(_sourceId, _options, fetchTile) {
        urlFetchBridges.push(fetchTile);
      },
      setStyleUrl() {},
      setStyleJson() {},
      releaseDetachedCustomGeometrySources() {
        return ["before", "late"];
      },
    },
  });
  let beforeUrlFetches = 0;
  let lateUrlFetches = 0;
  urlMap.addCustomGeometrySource("before", {
    fetchTile() {
      beforeUrlFetches += 1;
    },
  });

  urlMap.setStyleUrl("custom://replacement");
  urlMap.addCustomGeometrySource("late", {
    fetchTile() {
      lateUrlFetches += 1;
    },
  });
  urlFetchBridges[0](null, { z: 0, x: 0, y: 0 });
  urlFetchBridges[1](null, { z: 0, x: 0, y: 0 });
  assert.equal(beforeUrlFetches, 1);
  assert.equal(lateUrlFetches, 1);

  urlMap._finishStyleReplacement();
  urlFetchBridges[0](null, { z: 0, x: 0, y: 0 });
  urlFetchBridges[1](null, { z: 0, x: 0, y: 0 });
  assert.equal(beforeUrlFetches, 1);
  assert.equal(lateUrlFetches, 1);

  /** @type {Function[]} */
  const jsonFetchBridges = [];
  const jsonMap = Object.create(MapHandle.prototype);
  Object.defineProperty(jsonMap, "native", {
    value: {
      closed: false,
      /** @param {string} _sourceId @param {unknown} _options @param {Function} fetchTile */
      addCustomGeometrySource(_sourceId, _options, fetchTile) {
        jsonFetchBridges.push(fetchTile);
      },
      setStyleJson() {},
      releaseDetachedCustomGeometrySources() {
        return [];
      },
    },
  });
  let replacementFetches = 0;
  jsonMap.setStyleJson("{}");
  jsonMap.addCustomGeometrySource("replacement", {
    fetchTile() {
      replacementFetches += 1;
    },
  });
  jsonMap._finishStyleReplacement();
  jsonFetchBridges[0](null, { z: 0, x: 0, y: 0 });
  assert.equal(replacementFetches, 1);
});

test("log callback registration does not keep a process alive", () => {
  const packageRoot = path.join(__dirname, "..");
  const result = spawnSync(
    process.execPath,
    ["-e", `require(${JSON.stringify(packageRoot)}).setLogCallback(() => {});`],
    {
      env: {
        ...process.env,
        MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS: "1",
      },
      encoding: "utf8",
      timeout: 3_000,
    },
  );
  assert.equal(result.signal, null, result.stderr);
  assert.equal(result.status, 0, result.stderr);
});

test("finalizers release abandoned request and texture scopes", () => {
  const packageRoot = path.join(__dirname, "..");
  const script = `
    process.env.MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS = "1";
    const assert = require("node:assert/strict");
    const binding = require(${JSON.stringify(packageRoot)});
    const native = require(${JSON.stringify(path.join(packageRoot, "index.js"))});

    async function collectUntil(predicate) {
      for (let attempt = 0; attempt < 100 && !predicate(); attempt += 1) {
        global.gc();
        await new Promise((resolve) => setImmediate(resolve));
      }
      assert.equal(predicate(), true);
    }

    async function main() {
      let frameReleases = 0;
      let frame = binding.RenderSessionHandle.prototype
        .acquireMetalOwnedTextureFrame.call({
          native: {
            closed: false,
            acquireMetalOwnedTextureFrame() {
              return {
                generation: 1n,
                width: 1,
                height: 1,
                scaleFactor: 1,
                frameId: 1n,
                textureAddress: 1n,
                deviceAddress: 2n,
                pixelFormat: 80n,
              };
            },
            releaseMetalOwnedTextureFrame() {
              frameReleases += 1;
            },
          },
        });
      frame = undefined;
      await collectUntil(() => frameReleases === 1);

      let providerBridge;
      let requestCloses = 0;
      let discardAttempts = 0;
      native.createNativeRuntimeHandle = () => ({
        closed: false,
        close() {
          this.closed = true;
        },
        runAmbientCacheOperation() {
          return { operationId: 1n };
        },
        discardOfflineOperation() {
          discardAttempts += 1;
          if (discardAttempts <= 2) {
            throw new Error(
              'MaplibreNativeError:{"kind":"NativeError","nativeStatusCode":5,"diagnostic":"transient discard failure"}'
            );
          }
        },
        setResourceProviderRoutes(_routes, callback) {
          providerBridge = callback;
        },
      });
      const runtime = new binding.RuntimeHandle();
      native.nativeResourceRequestClose = () => {
        requestCloses += 1;
      };
      let requestHandle;
      runtime.setResourceProviderRoutes([{ urlPrefix: "custom://" }], (request) => {
        requestHandle = request.handle;
      });
      providerBridge(null, {
        url: "custom://pending",
        kind: "source",
        rawKind: 2,
        loadingMethod: "all",
        rawLoadingMethod: 0,
        priority: "regular",
        rawPriority: 0,
        usage: "online",
        rawUsage: 0,
        storagePolicy: "permanent",
        rawStoragePolicy: 0,
        priorData: new Uint8Array(),
        completionToken: "resource-request:finalizer",
      });
      assert.equal(runtime._resourceRequestCountForTesting(), 1);
      requestHandle = undefined;
      await collectUntil(() =>
        runtime._resourceRequestCountForTesting() === 0
      );
      assert.equal(requestCloses, 1);
      let operation = runtime.runAmbientCacheOperation("clear");
      operation = undefined;
      await collectUntil(() => discardAttempts === 1);
      assert.throws(() => runtime.close(), binding.NativeError);
      assert.equal(discardAttempts, 2);
      runtime.close();
      assert.equal(discardAttempts, 3);
    }

    main().catch((error) => {
      console.error(error);
      process.exitCode = 1;
    });
  `;
  const result = spawnSync(process.execPath, ["--expose-gc", "-e", script], {
    encoding: "utf8",
    timeout: 5_000,
  });
  assert.equal(result.signal, null, result.stderr);
  assert.equal(result.status, 0, result.stderr);
});

test("runtime event lookup does not retain abandoned map wrappers", () => {
  const packageRoot = path.join(__dirname, "..");
  // A deliberately abandoned native map can keep backend work alive on Windows.
  // Exit after the leak report proves the JavaScript wrapper was collected.
  const script = `
    process.env.MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS = "1";
    const assert = require("node:assert/strict");
    const binding = require(${JSON.stringify(packageRoot)});

    async function main() {
      binding.takeNativeLeakReports();
      const runtime = new binding.RuntimeHandle();
      let map = runtime.createMap({ width: 16, height: 16 });
      map = undefined;
      let reports = [];
      for (let attempt = 0; attempt < 100 && reports.length === 0; attempt += 1) {
        global.gc();
        await new Promise((resolve) => setImmediate(resolve));
        reports = binding.takeNativeLeakReports().filter(
          (report) => report.handleType === "MapHandle"
        );
      }
      assert.equal(reports.length, 1);
    }

    main().then(
      () => process.exit(0),
      (error) => {
        console.error(error);
        process.exit(1);
      }
    );
  `;
  const result = spawnSync(process.execPath, ["--expose-gc", "-e", script], {
    encoding: "utf8",
    timeout: 20_000,
  });
  assert.equal(result.signal, null, result.stderr);
  assert.equal(result.status, 0, result.stderr);
});

test("provider callbacks do not retain abandoned runtime wrappers", () => {
  const packageRoot = path.join(__dirname, "..");
  const script = `
    process.env.MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS = "1";
    const assert = require("node:assert/strict");
    const binding = require(${JSON.stringify(packageRoot)});

    async function main() {
      binding.takeNativeLeakReports();
      let runtime = new binding.RuntimeHandle();
      runtime.setResourceProviderRoutes([], () => {});
      runtime = undefined;
      let reports = [];
      for (let attempt = 0; attempt < 100 && reports.length === 0; attempt += 1) {
        global.gc();
        await new Promise((resolve) => setImmediate(resolve));
        reports = binding.takeNativeLeakReports().filter(
          (report) => report.handleType === "RuntimeHandle"
        );
      }
      assert.equal(reports.length, 1);
    }

    main().catch((error) => {
      console.error(error);
      process.exitCode = 1;
    });
  `;
  const result = spawnSync(process.execPath, ["--expose-gc", "-e", script], {
    encoding: "utf8",
    timeout: 5_000,
  });
  assert.equal(result.signal, null, result.stderr);
  assert.equal(result.status, 0, result.stderr);
});

test("runtime construction checks the loaded C ABI version first", () => {
  const originalCVersion = nativeAddon.cVersion;
  nativeAddon.cVersion = () => 999;

  try {
    assert.throws(
      () => new RuntimeHandle(),
      (error) =>
        error instanceof MaplibreError &&
        error.status === MaplibreStatus.abiVersionMismatch,
    );
  } finally {
    nativeAddon.cVersion = originalCVersion;
  }
});

test("async log severities map string values and reject unknown values", () => {
  setAsyncLogSeverities(["info", "warning"]);
  setAsyncLogSeverities(new Set(["error"]));
  restoreDefaultAsyncLogSeverities();

  assert.throws(
    () => setAsyncLogSeverities([/** @type {any} */ ("debug")]),
    InvalidArgumentError,
  );
});

test("native pointer is a borrowed opaque address value", () => {
  const pointer = NativePointer.unsafeFromAddress(0x1234n);

  assert.equal(pointer.address, 0x1234n);
  assert.equal(pointer.isNull, false);
  assert.equal(pointer.equals(NativePointer.unsafeFromAddress(0x1234n)), true);
  assert.equal(pointer.equals(NativePointer.null), false);
  assert.equal(NativePointer.null.isNull, true);
  assert.equal(pointer.toString(), "NativePointer[address=0x1234]");
  assert.throws(
    () => NativePointer.unsafeFromAddress(-1n),
    InvalidArgumentError,
  );
  assert.throws(
    () => new /** @type {any} */ (NativePointer)(1n),
    InvalidArgumentError,
  );
});

test("texture frame scopes expose borrowed pointers only while active", () => {
  let released = false;
  /** @type {import("..").MetalOwnedTextureFrame | undefined} */
  let frame;
  /** @type {import("..").NativePointer | undefined} */
  let scopedTexture;
  frame = RenderSessionHandle.prototype.acquireMetalOwnedTextureFrame.call({
    native: {
      acquireMetalOwnedTextureFrame() {
        return {
          generation: 1n,
          width: 2,
          height: 3,
          scaleFactor: 4,
          frameId: 5n,
          textureAddress: 0x10n,
          deviceAddress: 0x20n,
          pixelFormat: 80n,
        };
      },
      /** @param {any} raw */
      releaseMetalOwnedTextureFrame(raw) {
        assert.equal(raw.frameId, 5n);
        released = true;
      },
    },
  });

  assert.equal(frame instanceof MetalOwnedTextureFrame, true);
  assert.equal(frame.width, 2);
  scopedTexture = frame.texture;
  assert.equal(scopedTexture.address, 0x10n);
  assert.equal(frame.device.address, 0x20n);
  assert.equal(frame.pixelFormat, 80n);
  assert.equal(frame.closed, false);
  frame.close();
  assert.equal(released, true);
  assert.equal(frame.closed, true);
  assert.throws(() => frame.width, InvalidStateError);
  const textureAfterScope = scopedTexture;
  assert.ok(textureAfterScope);
  assert.throws(() => textureAfterScope.address, InvalidStateError);

  assert.equal(typeof VulkanOwnedTextureFrame, "function");
  assert.equal(typeof OpenGLOwnedTextureFrame, "function");
  assert.throws(
    () =>
      RenderSessionHandle.prototype.acquireVulkanOwnedTextureFrame.call({
        native: {
          closed: false,
          acquireVulkanOwnedTextureFrame() {
            throw new InvalidStateError(null, "active frame");
          },
        },
      }),
    InvalidStateError,
  );
  assert.throws(
    () =>
      new /** @type {any} */ (OpenGLOwnedTextureFrame)({}, { generation: 1n }),
    InvalidArgumentError,
  );
});

test("texture frame release failures leave frames retryable", () => {
  let attempts = 0;
  const frame =
    RenderSessionHandle.prototype.acquireMetalOwnedTextureFrame.call({
      native: {
        closed: false,
        acquireMetalOwnedTextureFrame() {
          return {
            generation: 1n,
            width: 2,
            height: 3,
            scaleFactor: 1,
            frameId: 5n,
            textureAddress: 0x10n,
            deviceAddress: 0x20n,
            pixelFormat: 80n,
          };
        },
        releaseMetalOwnedTextureFrame() {
          attempts += 1;
          if (attempts === 1) {
            throw new InvalidStateError(null, "release failed");
          }
        },
      },
    });

  assert.throws(() => frame.close(), InvalidStateError);
  assert.equal(frame.closed, false);
  assert.equal(frame.width, 2);
  frame.close();
  assert.equal(frame.closed, true);
});

test("native buffer owns byte storage for render interop", () => {
  const allocated = NativeBuffer.allocate(4);
  allocated.asUint8Array().set([1, 2, 3, 4]);

  assert.equal(allocated.byteLength, 4);
  assert.deepEqual([...allocated.asUint8Array()], [1, 2, 3, 4]);
  assert.equal(allocated.asArrayBuffer() instanceof ArrayBuffer, true);
  assert.equal(
    Object.prototype.toString.call(allocated),
    "[object NativeBuffer]",
  );

  const copied = NativeBuffer.from(allocated.asUint8Array());
  allocated.asUint8Array()[0] = 9;
  assert.deepEqual([...copied.asUint8Array()], [1, 2, 3, 4]);
  const sourceBuffer = new ArrayBuffer(4);
  new Uint8Array(sourceBuffer).set([5, 6, 7, 8]);
  const constructorCopied = new NativeBuffer(sourceBuffer);
  new Uint8Array(sourceBuffer)[0] = 1;
  assert.deepEqual([...constructorCopied.asUint8Array()], [5, 6, 7, 8]);
  const sharedCopy = NativeBuffer.from(
    new Uint8Array(new SharedArrayBuffer(4)),
  );
  assert.equal(sharedCopy.asArrayBuffer() instanceof ArrayBuffer, true);
  assert.equal(sharedCopy.asArrayBuffer() instanceof SharedArrayBuffer, false);
  assert.throws(() => NativeBuffer.allocate(-1), InvalidArgumentError);
  assert.throws(
    () => NativeBuffer.from(/** @type {any} */ ("bytes")),
    InvalidArgumentError,
  );
});

test("texture readback can write into caller-owned storage", () => {
  const target = new Uint8Array(4);
  const info = RenderSessionHandle.prototype.readPremultipliedRgba8Into.call(
    {
      native: {
        closed: false,
        /** @param {Uint8Array} buffer */
        readPremultipliedRgba8Into(buffer) {
          buffer.set([1, 2, 3, 4]);
          return { width: 1, height: 1, stride: 4, byteLength: 4 };
        },
      },
    },
    target,
  );
  assert.deepEqual([...target], [1, 2, 3, 4]);
  assert.deepEqual(info, { width: 1, height: 1, stride: 4, byteLength: 4 });

  const nativeBuffer = NativeBuffer.allocate(4);
  RenderSessionHandle.prototype.readPremultipliedRgba8Into.call(
    {
      native: {
        closed: false,
        /** @param {Uint8Array} buffer */
        readPremultipliedRgba8Into(buffer) {
          buffer.set([5, 6, 7, 8]);
          return { width: 1, height: 1, stride: 4, byteLength: 4 };
        },
      },
    },
    nativeBuffer,
  );
  assert.deepEqual([...nativeBuffer.asUint8Array()], [5, 6, 7, 8]);
});

test("handles stay local while workers create their own runtime", async () => {
  const worker = new Worker(
    `
      const { parentPort, workerData } = require("node:worker_threads");
      const { RuntimeHandle } = require(workerData.packageRoot);
      try {
        const runtime = new RuntimeHandle();
        runtime.close();
        parentPort.postMessage({ ok: true });
      } catch (error) {
        parentPort.postMessage({
          ok: false,
          name: error?.name,
          message: error?.message,
        });
      }
    `,
    {
      eval: true,
      workerData: { packageRoot: path.join(__dirname, "..") },
    },
  );
  const runtime = new RuntimeHandle();

  try {
    const clone = structuredClone(runtime);
    assert.equal(clone instanceof RuntimeHandle, false);
    assert.equal(typeof clone.close, "undefined");

    assert.throws(
      () =>
        ResourceRequestHandle.prototype.complete.call(
          { completionToken: "detached", closed: false },
          {},
        ),
      InvalidStateError,
    );
  } finally {
    runtime.close();
  }

  const result = await new Promise((resolve, reject) => {
    worker.once("message", resolve);
    worker.once("error", reject);
    worker.once("exit", (code) => {
      if (code !== 0) {
        reject(new Error(`worker exited with code ${code}`));
      }
    });
  });
  assert.deepEqual(result, { ok: true });
});

test("offline operations expose discardable handles", () => {
  const runtime = new RuntimeHandle();

  try {
    const operation = runtime.runAmbientCacheOperation("clear");
    assert.equal(operation instanceof OfflineOperationHandle, true);
    assert.equal(operation.closed, false);
    assert.throws(() => runtime.close(), InvalidStateError);
    operation.close();
    assert.equal(operation.closed, true);
    operation.close();
    const list = runtime.offlineRegionsList();
    assert.equal(list instanceof OfflineOperationHandle, true);
    list.close();
    assert.throws(
      () => runtime.offlineRegionsListTakeResult(list),
      InvalidStateError,
    );
    const get = runtime.offlineRegionGet(1n);
    get.close();
    const status = runtime.offlineRegionGetStatus(1n);
    status.close();
    const updateMetadata = runtime.offlineRegionUpdateMetadata(
      1n,
      new Uint8Array([4, 5]),
    );
    updateMetadata.close();
    const create = runtime.offlineRegionCreate(
      {
        kind: "tilePyramid",
        styleUrl: "https://example.test/style.json",
        bounds: {
          southwest: { latitude: -1, longitude: -2 },
          northeast: { latitude: 1, longitude: 2 },
        },
        minZoom: 0,
        maxZoom: 1,
        pixelRatio: 1,
      },
      new Uint8Array([1, 2, 3]),
    );
    create.close();
    assert.throws(
      () =>
        runtime.offlineRegionSetDownloadState(
          1n,
          /** @type {any} */ ("paused"),
        ),
      InvalidArgumentError,
    );
    const active = runtime.offlineRegionSetDownloadState(1n, {
      downloadState: "active",
      rawDownloadState: 1,
    });
    active.close();
    assert.throws(
      () =>
        runtime.offlineRegionSetDownloadState(1n, {
          downloadState: "unknown",
          rawDownloadState: 1000,
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () => runtime.runAmbientCacheOperation(/** @type {any} */ ("vacuum")),
      InvalidArgumentError,
    );
  } finally {
    runtime.close();
  }
});

test("offline operation handles validate runtime, kind, and consumption", () => {
  const runtime = new RuntimeHandle();

  try {
    assert.throws(
      () => new /** @type {any} */ (OfflineOperationHandle)(runtime, 10n),
      InvalidArgumentError,
    );

    const closedOperation = runtime.offlineRegionsList();
    closedOperation.close();
    assert.throws(
      () => runtime.offlineRegionsListTakeResult(closedOperation),
      InvalidStateError,
    );

    const wrongKind = runtime.offlineRegionGet(1n);
    assert.throws(
      () => runtime.offlineRegionsListTakeResult(wrongKind),
      InvalidStateError,
    );
    wrongKind.close();

    assert.throws(
      () => runtime.offlineRegionsListTakeResult(/** @type {any} */ (13n)),
      InvalidArgumentError,
    );
  } finally {
    runtime.close();
  }
});

test("offline take errors preserve whether native ownership transferred", () => {
  const originalCreateRuntime = nativeAddon.createNativeRuntimeHandle;
  /** @param {boolean} offlineOperationConsumed */
  const nativeError = (offlineOperationConsumed) =>
    new Error(
      `MaplibreNativeError:${JSON.stringify({
        kind: "InvalidArgument",
        nativeStatusCode: 1,
        diagnostic: "snapshot copy failed",
        offlineOperationConsumed,
      })}`,
    );

  try {
    nativeAddon.createNativeRuntimeHandle =
      /** @type {any} */ (
        () => ({
          closed: false,
          close() {},
          offlineRegionsList() {
            return { operationId: 1n };
          },
          offlineRegionsListTakeResult() {
            throw nativeError(true);
          },
        })
      );
    const consumedRuntime = new RuntimeHandle();
    const consumed = consumedRuntime.offlineRegionsList();
    assert.throws(
      () => consumedRuntime.offlineRegionsListTakeResult(consumed),
      InvalidArgumentError,
    );
    assert.equal(consumed.closed, true);
    consumedRuntime.close();

    nativeAddon.createNativeRuntimeHandle =
      /** @type {any} */ (
        () => ({
          closed: false,
          close() {},
          offlineRegionsList() {
            return { operationId: 2n };
          },
          offlineRegionsListTakeResult() {
            throw nativeError(false);
          },
          discardOfflineOperation() {},
        })
      );
    const retryableRuntime = new RuntimeHandle();
    const retryable = retryableRuntime.offlineRegionsList();
    assert.throws(
      () => retryableRuntime.offlineRegionsListTakeResult(retryable),
      InvalidArgumentError,
    );
    assert.equal(retryable.closed, false);
    assert.throws(() => retryableRuntime.close(), InvalidStateError);
    retryable.close();
    retryableRuntime.close();
  } finally {
    nativeAddon.createNativeRuntimeHandle = originalCreateRuntime;
  }
});

test("resource providers must be configured before map creation", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.close();
    assert.throws(
      () => runtime.setResourceProviderRoutes([], () => {}),
      InvalidStateError,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("offline operation events expose copied typed payloads", async () => {
  const runtime = new RuntimeHandle();
  let operation;
  try {
    operation = runtime.runAmbientCacheOperation("clear");
    const event = await eventually(() => {
      runtime.runOnce();
      const event = runtime.pollEvent();
      return event?.payload.kind === "offline-operation-completed"
        ? event
        : null;
    });

    assert.equal(event.payloadKind, "offline-operation-completed");
    assert.equal(event.payload.rawType > 0, true);
    assert.equal(event.sourceType, "runtime");
    assert.equal(event.sourceMap, null);
    const completed = /** @type {any} */ (event.payload)
      .offlineOperationCompleted;
    assert.equal(completed.operation, operation);
    assert.equal("operationId" in completed, false);
    assert.equal(completed.operationKind, "ambientCache");
    assert.equal(completed.resultKind, "none");
    assert.equal(typeof completed.rawOperationKind, "number");
    assert.equal(typeof completed.rawResultKind, "number");
    assert.equal(typeof completed.resultStatus, "number");
    assert.equal(typeof completed.found, "boolean");
  } finally {
    operation?.close();
    runtime.close();
  }
});

test("map events expose proven public map identity without native addresses", async () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setStyleJson('{"version":8,"sources":{},"layers":[]}');
    const event = await eventually(() => {
      runtime.runOnce();
      const event = runtime.pollEvent();
      return event?.eventType === "map-style-loaded" ? event : null;
    });
    assert.equal(event.sourceType, "map");
    assert.equal(event.sourceMap, map);
    assert.equal("sourceAddress" in event, false);
  } finally {
    map.close();
    runtime.close();
  }
});

test("resource provider routes validate Node handoff shape", async () => {
  const runtime = new RuntimeHandle();

  try {
    runtime.setResourceProviderRoutes(
      [
        { urlPrefix: "custom://", kind: "source" },
        {
          urlPrefix: "future://",
          kind: { kind: "unknown", rawKind: 1000 },
        },
      ],
      (request) => {
        assert.equal(typeof request.url, "string");
        assert.equal(typeof request.rawKind, "number");
        assert.equal(request.handle instanceof ResourceRequestHandle, true);
      },
    );
    assert.throws(
      () =>
        runtime.setResourceProviderRoutes(/** @type {any} */ (null), () => {}),
      InvalidArgumentError,
    );
    assert.throws(
      () => runtime.setResourceProviderRoutes([], /** @type {any} */ (null)),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        runtime.setResourceProviderRoutes(
          [
            {
              kind: /** @type {any} */ ({ kind: "source", rawKind: 3 }),
              urlPrefix: "mismatch://",
            },
          ],
          () => {},
        ),
      InvalidArgumentError,
    );
  } finally {
    runtime.close();
  }

  const originalComplete = nativeAddon.nativeResourceRequestComplete;
  const originalClose = nativeAddon.nativeResourceRequestClose;
  /** @type {Array<{ completionToken: string, response: any }>} */
  const completions = [];
  /** @type {string[]} */
  const closes = [];
  nativeAddon.nativeResourceRequestComplete =
    /** @type {(completionToken: string, response: any) => void} */ (
      (completionToken, response) => {
        completions.push({ completionToken, response });
      }
    );
  nativeAddon.nativeResourceRequestClose =
    /** @type {(completionToken: string) => void} */ (
      (completionToken) => {
        closes.push(completionToken);
      }
    );
  try {
    /** @type {any} */
    let received;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(routes, callback) {
            assert.deepEqual(routes, [{ urlPrefix: "custom://" }]);
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:1",
                "custom://tile",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        received = request;
        return /** @type {any} */ ("ignored");
      },
    );
    assert.ok(received);
    assert.equal(received.completionToken, undefined);
    assert.equal(received.handle instanceof ResourceRequestHandle, true);
    assert.equal(received.handle.closed, false);
    received.handle.close();
    assert.deepEqual(closes, ["resource-request:1"]);
    assert.throws(() => received.handle.cancelled(), InvalidStateError);

    /** @type {any} */
    let thrownHandle;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:2",
                "custom://throw",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        thrownHandle = request.handle;
        throw new Error("provider boom");
      },
    );
    assert.ok(thrownHandle);
    assert.equal(thrownHandle.closed, true);
    assert.deepEqual(completions[0], {
      completionToken: "resource-request:2",
      response: {
        status: "error",
        errorReason: "other",
        errorMessage: "provider boom",
      },
    });

    /** @type {any} */
    let rejectedHandle;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:3",
                "custom://reject",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        rejectedHandle = request.handle;
        return Promise.reject(new Error("provider rejected"));
      },
    );
    await new Promise((resolve) => setImmediate(resolve));
    assert.ok(rejectedHandle);
    assert.equal(rejectedHandle.closed, true);
    assert.deepEqual(completions[1], {
      completionToken: "resource-request:3",
      response: {
        status: "error",
        errorReason: "other",
        errorMessage: "provider rejected",
      },
    });

    /** @type {any} */
    let thenableHandle;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:4",
                "custom://thenable",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        thenableHandle = request.handle;
        return /** @type {PromiseLike<void>} */ ({
          then(_resolve, reject) {
            setImmediate(() => reject?.(new Error("thenable rejected")));
            return /** @type {any} */ (this);
          },
        });
      },
    );
    await new Promise((resolve) => setImmediate(resolve));
    await new Promise((resolve) => setImmediate(resolve));
    assert.ok(thenableHandle);
    assert.equal(thenableHandle.closed, true);
    assert.deepEqual(completions[2], {
      completionToken: "resource-request:4",
      response: {
        status: "error",
        errorReason: "other",
        errorMessage: "thenable rejected",
      },
    });

    /** @type {any} */
    let nulErrorHandle;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:nul-error",
                "custom://nul-error",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        nulErrorHandle = request.handle;
        throw new Error("provider\0rejected");
      },
    );
    assert.ok(nulErrorHandle);
    assert.equal(nulErrorHandle.closed, true);
    assert.deepEqual(completions[3], {
      completionToken: "resource-request:nul-error",
      response: {
        status: "error",
        errorReason: "other",
        errorMessage: "provider\uFFFDrejected",
      },
    });

    /** @type {any} */
    let mutatedHandle;
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:5",
                "custom://mutate",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        mutatedHandle = request.handle;
        /** @type {any} */ (request.handle).closed = true;
        /** @type {any} */ (request.handle).completionToken = "wrong";
        throw new Error("mutated provider boom");
      },
    );
    assert.ok(mutatedHandle);
    assert.equal(mutatedHandle.closed, true);
    assert.equal(mutatedHandle.completionToken, undefined);
    assert.deepEqual(completions[4], {
      completionToken: "resource-request:5",
      response: {
        status: "error",
        errorReason: "other",
        errorMessage: "mutated provider boom",
      },
    });

    /** @type {any} */
    let invalidResponseHandle;
    let invalidResponseAttempts = 0;
    nativeAddon.nativeResourceRequestComplete =
      /** @type {(completionToken: string, response: any) => void} */ (
        (completionToken, response) => {
          invalidResponseAttempts += 1;
          if (invalidResponseAttempts === 1) {
            throw new InvalidArgumentError(1, "invalid response");
          }
          completions.push({ completionToken, response });
        }
      );
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:6",
                "custom://invalid",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        invalidResponseHandle = request.handle;
      },
    );
    assert.ok(invalidResponseHandle);
    assert.throws(
      () => invalidResponseHandle.complete({ status: "not-valid" }),
      InvalidArgumentError,
    );
    assert.equal(invalidResponseHandle.closed, false);
    invalidResponseHandle.complete({ bytes: new Uint8Array([1]) });
    assert.equal(invalidResponseHandle.closed, true);
    assert.deepEqual(completions[5], {
      completionToken: "resource-request:6",
      response: { bytes: new Uint8Array([1]) },
    });

    /** @type {any} */
    let failedCompletionHandle;
    nativeAddon.nativeResourceRequestComplete =
      /** @type {(completionToken: string, response: any) => void} */ (
        () => {
          throw new InvalidStateError(2, "native completion failed");
        }
      );
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:7",
                "custom://fail",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        failedCompletionHandle = request.handle;
      },
    );
    assert.ok(failedCompletionHandle);
    assert.throws(
      () => failedCompletionHandle.complete({ bytes: new Uint8Array([1]) }),
      InvalidStateError,
    );
    assert.equal(failedCompletionHandle.closed, true);
    assert.throws(
      () => failedCompletionHandle.complete({ bytes: new Uint8Array([2]) }),
      InvalidStateError,
    );
    assert.throws(() => failedCompletionHandle.cancelled(), InvalidStateError);

    /** @type {any} */
    let staleHandle;
    nativeAddon.nativeResourceRequestComplete =
      /** @type {(completionToken: string, response: any) => void} */ (
        () => {
          throw new InvalidStateError(null, "ResourceRequestHandle is closed");
        }
      );
    RuntimeHandle.prototype.setResourceProviderRoutes.call(
      {
        native: {
          /** @param {any} _routes @param {(error: any, request: any) => void} callback */
          setResourceProviderRoutes(_routes, callback) {
            callback(
              null,
              fakeResourceProviderRequest(
                "resource-request:8",
                "custom://stale",
              ),
            );
          },
        },
      },
      [{ urlPrefix: "custom://" }],
      (request) => {
        staleHandle = request.handle;
      },
    );
    assert.ok(staleHandle);
    assert.throws(
      () => staleHandle.complete({ bytes: new Uint8Array([1]) }),
      InvalidStateError,
    );
    assert.equal(staleHandle.closed, true);
  } finally {
    nativeAddon.nativeResourceRequestComplete = originalComplete;
    nativeAddon.nativeResourceRequestClose = originalClose;
  }
});

test("replaced and closed providers discard queued requests", () => {
  const originalCreateRuntime = nativeAddon.createNativeRuntimeHandle;
  const originalCloseRequest = nativeAddon.nativeResourceRequestClose;
  /** @type {Function[]} */
  const bridges = [];
  /** @type {string[]} */
  const closedTokens = [];
  nativeAddon.nativeResourceRequestClose = (token) => {
    closedTokens.push(token);
  };
  nativeAddon.createNativeRuntimeHandle =
    /** @type {any} */ (
      () => ({
        closed: false,
        close() {
          this.closed = true;
        },
        /** @param {unknown[]} _routes @param {Function} callback */
        setResourceProviderRoutes(_routes, callback) {
          bridges.push(callback);
        },
      })
    );

  try {
    const runtime = new RuntimeHandle();
    let deliveries = 0;
    runtime.setResourceProviderRoutes([], () => {
      deliveries += 1;
    });
    runtime.setResourceProviderRoutes([], () => {
      deliveries += 1;
    });
    assert.doesNotThrow(() =>
      bridges[1](
        new Error("queued provider delivery failed"),
        fakeResourceProviderRequest("failed-delivery", "error://"),
      ),
    );
    assert.deepEqual(closedTokens, ["failed-delivery"]);
    bridges[0](null, fakeResourceProviderRequest("old-provider", "old://"));
    assert.equal(deliveries, 0);
    assert.deepEqual(closedTokens, ["failed-delivery", "old-provider"]);
    runtime.close();
    bridges[1](null, fakeResourceProviderRequest("closed-runtime", "new://"));
    assert.equal(deliveries, 0);
    assert.deepEqual(closedTokens, [
      "failed-delivery",
      "old-provider",
      "closed-runtime",
    ]);
  } finally {
    nativeAddon.createNativeRuntimeHandle = originalCreateRuntime;
    nativeAddon.nativeResourceRequestClose = originalCloseRequest;
  }
});

test("runtime teardown closes pending resource request wrappers", () => {
  const originalCreateRuntime = nativeAddon.createNativeRuntimeHandle;
  /** @type {undefined | ((error: Error | null, request: any) => void)} */
  let providerBridge;
  nativeAddon.createNativeRuntimeHandle =
    /** @type {any} */ (
      () => ({
        closed: false,
        close() {},
        /**
         * @param {unknown[]} _routes
         * @param {(error: Error | null, request: any) => void} callback
         */
        setResourceProviderRoutes(_routes, callback) {
          providerBridge = callback;
        },
      })
    );

  try {
    const runtime = new RuntimeHandle();
    /** @type {ResourceRequestHandle | undefined} */
    let requestHandle;
    runtime.setResourceProviderRoutes(
      [{ urlPrefix: "custom://" }],
      (request) => {
        requestHandle = request.handle;
      },
    );
    providerBridge?.(
      null,
      fakeResourceProviderRequest(
        "resource-request:runtime-close",
        "custom://pending",
      ),
    );
    assert.ok(requestHandle);
    const pendingHandle = requestHandle;
    assert.equal(pendingHandle.closed, false);

    runtime.close();

    assert.equal(pendingHandle.closed, true);
    assert.throws(() => pendingHandle.cancelled(), InvalidStateError);
  } finally {
    nativeAddon.createNativeRuntimeHandle = originalCreateRuntime;
  }
});

test("runtime handle supports options, resource transform, explicit close, and idempotent disposal", () => {
  const runtime = new RuntimeHandle({ maximumCacheSize: 1n });

  assert.equal(runtime.closed, false);
  assert.equal(
    Object.prototype.propertyIsEnumerable.call(runtime, "native"),
    false,
  );
  assert.equal("native" in runtime, false);
  runtime.runOnce();
  assert.equal(runtime.pollEvent(), null);
  runtime.setResourceTransformRules([
    {
      kind: "source",
      urlPrefix: "http://example.test/",
      replacementUrlPrefix: "https://example.test/",
    },
    {
      url: "custom://style.json",
      replacementUrl: "https://example.test/style.json",
    },
    {
      kind: { kind: "unknown", rawKind: 1000 },
      url: "future://style.json",
      replacementUrl: "https://example.test/future.json",
    },
  ]);
  runtime.clearResourceTransform();
  assert.throws(
    () =>
      runtime.setResourceTransformRules([
        {
          kind: /** @type {any} */ ({ kind: "source", rawKind: 3 }),
          replacementUrl: "https://example.test/mismatch.json",
        },
      ]),
    InvalidArgumentError,
  );
  assert.throws(
    () => runtime.setResourceTransformRules(/** @type {any} */ (null)),
    InvalidArgumentError,
  );
  assert.throws(
    () =>
      runtime.setResourceTransformRules(
        /** @type {any} */ ([
          { urlPrefix: "http://", replacementUrlPrefix: "https://" },
          { replacementUrl: "https://a", replacementUrlPrefix: "https://b" },
        ]),
      ),
    InvalidArgumentError,
  );
  runtime.close();
  assert.equal(runtime.closed, true);
  assert.throws(() => runtime.runOnce(), /handle is closed/);
  runtime.close();
  runtime[Symbol.dispose]();
});

test("map handle retains runtime parent and closes before runtime", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 32, height: 32, scaleFactor: 1 });

  assert.equal(map instanceof MapHandle, true);
  assert.equal(map.closed, false);
  const projection = map.createProjection();
  projection.close();
  assert.throws(
    () =>
      Reflect.construct(/** @type {any} */ (RenderSessionHandle), [
        { closed: false },
        map,
      ]),
    InvalidArgumentError,
  );
  assert.throws(
    () => new /** @type {any} */ (ResourceRequestHandle)("detached"),
    InvalidArgumentError,
  );
  assert.throws(() => runtime.close(), InvalidStateError);
  map.close();
  assert.equal(map.closed, true);
  assert.equal("native" in map, false);
  assert.throws(() => map.requestRepaint(), /handle is closed/);
  map.close();
  runtime.close();
});

test("render session attach descriptors translate native failures", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    assert.throws(
      () =>
        map.attachMetalOwnedTexture({
          extent: { width: 16, height: 16, scaleFactor: 1 },
          context: { device: NativePointer.null },
        }),
      MaplibreError,
    );
    assert.throws(
      () =>
        map.attachMetalBorrowedTexture({
          extent: { width: 16, height: 16, scaleFactor: 1 },
          physicalWidth: 16,
          physicalHeight: 16,
          texture: /** @type {any} */ (0n),
        }),
      InvalidArgumentError,
    );
    assert.equal(
      Object.hasOwn(RenderSessionHandle, "attachMetalOwnedTexture"),
      false,
    );
    assert.equal(
      typeof RenderSessionHandle.prototype.setFeatureState,
      "function",
    );
    assert.equal(
      typeof RenderSessionHandle.prototype.queryRenderedFeatures,
      "function",
    );
    assert.equal(
      typeof RenderSessionHandle.prototype.queryFeatureExtension,
      "function",
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("OpenGL descriptors omit the inactive context provider", () => {
  const originalAttach = nativeAddon.createOpenGLOwnedTextureRenderSession;
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });
  /** @type {any[]} */
  const contexts = [];
  nativeAddon.createOpenGLOwnedTextureRenderSession =
    /** @type {typeof originalAttach} */ (
      (_nativeMap, descriptor) => {
        contexts.push(descriptor.context);
        return /** @type {any} */ ({
          closed: false,
          close() {},
        });
      }
    );

  try {
    const address = NativePointer.unsafeFromAddress(1n);
    map
      .attachOpenGLOwnedTexture({
        extent: { width: 16, height: 16, scaleFactor: 1 },
        context: {
          platform: "egl",
          display: address,
          config: address,
          shareContext: address,
        },
      })
      .close();
    map
      .attachOpenGLOwnedTexture({
        extent: { width: 16, height: 16, scaleFactor: 1 },
        context: {
          platform: "wgl",
          deviceContext: address,
          shareContext: address,
        },
      })
      .close();

    assert.equal(Object.hasOwn(contexts[0], "wgl"), false);
    assert.equal(Object.hasOwn(contexts[0], "egl"), true);
    assert.equal(contexts[0].egl.getProcAddressAddress, undefined);
    assert.equal(Object.hasOwn(contexts[1], "wgl"), true);
    assert.equal(Object.hasOwn(contexts[1], "egl"), false);
    assert.equal(contexts[1].wgl.getProcAddressAddress, undefined);
  } finally {
    nativeAddon.createOpenGLOwnedTextureRenderSession = originalAttach;
    map.close();
    runtime.close();
  }
});

test("map utility methods expose copied booleans and native commands", () => {
  const runtime = new RuntimeHandle();
  const continuousMap = runtime.createMap({ width: 16, height: 16 });

  try {
    assert.equal(typeof continuousMap.isFullyLoaded(), "boolean");
    continuousMap.renderingStatsViewEnabled = true;
    assert.equal(continuousMap.renderingStatsViewEnabled, true);
    continuousMap.renderingStatsViewEnabled = false;
    assert.equal(continuousMap.renderingStatsViewEnabled, false);
    continuousMap.requestRepaint();
    continuousMap.dumpDebugLogs();
  } finally {
    continuousMap.close();
  }

  const staticMap = runtime.createMap({
    width: 16,
    height: 16,
    mapMode: "static",
  });
  try {
    staticMap.requestStillImage();
  } finally {
    staticMap.close();
    runtime.close();
  }
});

test("map viewport and tile options map descriptor fields", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setViewportOptions({
      northOrientation: "right",
      constrainMode: "screen",
      viewportMode: "flippedY",
      frustumOffset: { top: 1, left: 2, bottom: 3, right: 4 },
    });
    const viewport = map.getViewportOptions();
    assert.equal(viewport.northOrientation, "right");
    assert.equal(viewport.northOrientationRaw, 1);
    assert.equal(viewport.constrainMode, "screen");
    assert.equal(viewport.constrainModeRaw, 3);
    assert.equal(viewport.viewportMode, "flippedY");
    assert.equal(viewport.viewportModeRaw, 1);
    assert.deepEqual(viewport.frustumOffset, {
      top: 1,
      left: 2,
      bottom: 3,
      right: 4,
    });
    map.setViewportOptions(
      viewport.copy({
        frustumOffset: { top: 4, left: 3, bottom: 2, right: 1 },
      }),
    );
    assert.deepEqual(map.getViewportOptions().frustumOffset, {
      top: 4,
      left: 3,
      bottom: 2,
      right: 1,
    });

    map.setTileOptions({
      prefetchZoomDelta: 2,
      lodMinRadius: 1,
      lodScale: 1.5,
      lodPitchThreshold: 20,
      lodZoomShift: 0.5,
      lodMode: "distance",
    });
    const tile = map.getTileOptions();
    assert.equal(tile.prefetchZoomDelta, 2);
    assert.equal(tile.lodMode, "distance");
    assert.equal(tile.lodModeRaw, 1);
    map.setTileOptions(tile);
    assert.throws(
      () =>
        map.setViewportOptions({
          northOrientation: /** @type {any} */ ("north"),
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.setViewportOptions(
          /** @type {any} */ ({
            northOrientation: "right",
            northOrientationRaw: 2,
          }),
        ),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.setTileOptions(
          /** @type {any} */ ({ lodMode: "distance", lodModeRaw: 0 }),
        ),
      InvalidArgumentError,
    );
    assert.throws(
      () => map.setTileOptions({ lodMode: /** @type {any} */ ("nearest") }),
      InvalidArgumentError,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("map bounds options copy constraints", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    const bounds = {
      southwest: { latitude: -10, longitude: -20 },
      northeast: { latitude: 10, longitude: 20 },
    };
    map.setBounds({
      bounds,
      minZoom: 1,
      maxZoom: 10,
      minPitch: 0,
      maxPitch: 45,
    });
    const copied = map.getBounds();
    assert.deepEqual(copied.bounds, bounds);
    assert.equal(copied.minZoom, 1);
    assert.equal(copied.maxZoom, 10);
    assert.equal(copied.minPitch, 0);
    assert.equal(copied.maxPitch, 45);
  } finally {
    map.close();
    runtime.close();
  }
});

test("map projection mode maps optional descriptor fields", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setProjectionMode({ axonometric: true, xSkew: 0.2, ySkew: 0.3 });
    const mode = map.getProjectionMode();
    assert.equal(mode.axonometric, true);
    assert.equal(mode.xSkew, 0.2);
    assert.equal(mode.ySkew, 0.3);
  } finally {
    map.close();
    runtime.close();
  }
});

test("map camera fitting helpers copy camera and bounds values", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 256, height: 256 });
  const bounds = {
    southwest: { latitude: -1, longitude: -2 },
    northeast: { latitude: 1, longitude: 2 },
  };

  try {
    const fitOptions = {
      padding: { top: 1, left: 2, bottom: 3, right: 4 },
      bearing: 5,
      pitch: 6,
    };
    const camera = map.cameraForLatLngBounds(bounds, fitOptions);
    assert.equal(typeof camera.zoom, "number");
    assert.equal(typeof camera.center?.latitude, "number");
    const cameraFromCoordinates = map.cameraForLatLngs(
      [bounds.southwest, bounds.northeast],
      fitOptions,
    );
    assert.equal(typeof cameraFromCoordinates.zoom, "number");
    const cameraFromGeometry = map.cameraForGeometry(
      {
        type: "LineString",
        coordinates: [
          [-2, -1],
          [2, 1],
        ],
      },
      fitOptions,
    );
    assert.equal(typeof cameraFromGeometry.zoom, "number");
    const visibleBounds = map.latLngBoundsForCamera(camera);
    const unwrappedBounds = map.latLngBoundsForCameraUnwrapped(camera);
    assert.equal(typeof visibleBounds.southwest.latitude, "number");
    assert.equal(typeof unwrappedBounds.northeast.longitude, "number");
  } finally {
    map.close();
    runtime.close();
  }
});

test("map camera commands copy descriptor values", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.jumpTo({
      center: { latitude: 12.5, longitude: 34.5 },
      zoom: 3,
      bearing: 10,
      pitch: 20,
    });
    const camera = map.getCamera();
    assert.ok(camera.center);
    assert.ok(Math.abs(camera.center.latitude - 12.5) < 1e-9);
    assert.ok(Math.abs(camera.center.longitude - 34.5) < 1e-9);
    assert.equal(camera.zoom, 3);
    assert.equal(camera.bearing, 10);
    assert.equal(camera.pitch, 20);
    map.easeTo(
      { center: { latitude: 13, longitude: 35 }, zoom: 4 },
      { durationMs: 0, easing: { x1: 0, y1: 0, x2: 1, y2: 1 } },
    );
    map.flyTo({ center: { latitude: 14, longitude: 36 }, zoom: 5 }, null);
    map.setFreeCameraOptions({ orientation: { x: 0, y: 0, z: 0, w: 1 } });
    assert.equal(typeof map.getFreeCameraOptions().orientation?.w, "number");
  } finally {
    map.close();
    runtime.close();
  }
});

test("map camera movement commands adapt point descriptors", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 256, height: 256 });

  try {
    map.moveBy(1, 2);
    map.scaleBy(1.1, { x: 10, y: 10 });
    map.scaleBy(1.0);
    map.rotateBy({ x: 10, y: 10 }, { x: 12, y: 12 });
    map.pitchBy(1);
    map.moveByAnimated(1, 2, { durationMs: 0 });
    map.scaleByAnimated(1.1, { x: 10, y: 10 }, { durationMs: 0 });
    map.rotateByAnimated({ x: 10, y: 10 }, { x: 12, y: 12 }, { durationMs: 0 });
    map.pitchByAnimated(1, { durationMs: 0 });
    map.cancelTransitions();
  } finally {
    map.close();
    runtime.close();
  }
});

test("map projection handle snapshots projection state", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 256, height: 256 });
  let projection;

  try {
    map.jumpTo({ center: { latitude: 5, longitude: 6 }, zoom: 2 });
    projection = map.createProjection();
    assert.equal(projection instanceof MapProjectionHandle, true);
    const point = projection.pixelForLatLng({ latitude: 5, longitude: 6 });
    const roundTripped = projection.latLngForPixel(point);
    assert.ok(Math.abs(roundTripped.latitude - 5) < 1e-9);
    assert.ok(Math.abs(roundTripped.longitude - 6) < 1e-9);
    projection.setCamera({ center: { latitude: 7, longitude: 8 }, zoom: 3 });
    assert.ok(projection.getCamera().center);
    projection.setVisibleCoordinates(
      [
        { latitude: -1, longitude: -2 },
        { latitude: 1, longitude: 2 },
      ],
      { top: 0, left: 0, bottom: 0, right: 0 },
    );
    projection.setVisibleGeometry(
      {
        type: "LineString",
        coordinates: [
          [-2, -1],
          [2, 1],
        ],
      },
      { top: 0, left: 0, bottom: 0, right: 0 },
    );
    assert.equal(typeof projection.getCamera().zoom, "number");
    projection.close();
    assert.equal(projection.closed, true);
    projection.close();
  } finally {
    projection?.close();
    map.close();
    runtime.close();
  }
});

test("map screen projection helpers copy point values", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 256, height: 256 });
  const coordinate = { latitude: 10, longitude: 20 };

  try {
    map.jumpTo({ center: coordinate, zoom: 2 });
    const point = map.pixelForLatLng(coordinate);
    const roundTripped = map.latLngForPixel(point);
    const points = map.pixelsForLatLngs([coordinate]);
    const coordinates = map.latLngsForPixels(points);
    assert.equal(typeof point.x, "number");
    assert.equal(typeof point.y, "number");
    assert.equal(points.length, 1);
    assert.equal(coordinates.length, 1);
    assert.ok(Math.abs(roundTripped.latitude - coordinate.latitude) < 1e-9);
    assert.ok(Math.abs(roundTripped.longitude - coordinate.longitude) < 1e-9);
    assert.ok(Math.abs(coordinates[0].latitude - coordinate.latitude) < 1e-9);
    assert.ok(Math.abs(coordinates[0].longitude - coordinate.longitude) < 1e-9);
  } finally {
    map.close();
    runtime.close();
  }
});

test("style JSON helpers serialize JavaScript values and copy booleans", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setStyleJson('{"version":8,"sources":{},"layers":[]}');
    runtime.runOnce();
    map.addStyleSourceJson("empty-geojson", {
      type: "geojson",
      data: { type: "FeatureCollection", features: [] },
    });
    map.setStyleLight({ anchor: "viewport", color: "#ffffff", intensity: 0.5 });
    map.setStyleLightProperty("intensity", 0.75);
    assert.equal(map.getStyleLightProperty("intensity"), 0.75);
    assert.deepEqual(map.getStyleLightProperty("color"), [
      "rgba",
      255,
      255,
      255,
      1,
    ]);

    map.setStyleImage(
      "red-pixel",
      {
        width: 1,
        height: 1,
        pixels: new Uint8Array([255, 0, 0, 255]),
      },
      {
        pixelRatio: 2,
        sdf: true,
      },
    );
    assert.equal(map.styleImageExists("red-pixel"), true);
    assert.deepEqual(map.getStyleImageInfo("red-pixel"), {
      width: 1,
      height: 1,
      stride: 4,
      byteLength: 4,
      pixelRatio: 2,
      sdf: true,
    });
    const copiedImage = map.copyStyleImagePremultipliedRgba8("red-pixel");
    assert.ok(copiedImage);
    assert.equal(copiedImage.byteLength, 4);
    assert.deepEqual([...copiedImage.pixels], [255, 0, 0, 255]);
    assert.equal(map.copyStyleImagePremultipliedRgba8("missing-image"), null);
    assert.equal(map.removeStyleImage("red-pixel"), true);
    assert.equal(map.styleImageExists("red-pixel"), false);

    assert.equal(map.styleSourceExists("empty-geojson"), true);
    assert.equal(map.listStyleSourceIds().includes("empty-geojson"), true);
    assert.deepEqual(map.getStyleSourceType("empty-geojson"), {
      kind: "geojson",
      rawType: 4,
    });
    assert.equal(
      map.getStyleSourceInfo("empty-geojson")?.sourceType,
      "geojson",
    );
    assert.equal(map.getStyleSourceInfo("missing-source"), null);

    const imageCoordinates = [
      { latitude: 1, longitude: 2 },
      { latitude: 1, longitude: 3 },
      { latitude: 0, longitude: 3 },
      { latitude: 0, longitude: 2 },
    ];
    const geojsonData = {
      type: "FeatureCollection",
      features: [
        {
          type: "Feature",
          id: "one",
          properties: { name: "point" },
          geometry: { type: "Point", coordinates: [2, 1] },
        },
      ],
    };
    map.addGeoJsonSourceUrl("geojson-url", "https://example.test/data.geojson");
    assert.deepEqual(map.getStyleSourceType("geojson-url"), {
      kind: "geojson",
      rawType: 4,
    });
    map.setGeoJsonSourceUrl(
      "geojson-url",
      "https://example.test/updated.geojson",
    );
    map.setGeoJsonSourceData("geojson-url", geojsonData);
    map.addGeoJsonSourceData("geojson-data", geojsonData);
    assert.deepEqual(map.getStyleSourceType("geojson-data"), {
      kind: "geojson",
      rawType: 4,
    });
    map.addVectorSourceUrl("vector-url", "https://example.test/vector.json", {
      minZoom: 0,
      maxZoom: 14,
      attribution: "Example",
      scheme: "xyz",
      bounds: {
        southwest: { latitude: -85, longitude: -180 },
        northeast: { latitude: 85, longitude: 180 },
      },
      vectorEncoding: "mvt",
    });
    assert.deepEqual(map.getStyleSourceType("vector-url"), {
      kind: "vector",
      rawType: 1,
    });
    map.addStyleSourceJson("vector-empty-attribution", {
      type: "vector",
      tiles: ["https://example.test/{z}/{x}/{y}.pbf"],
      attribution: "",
    });
    const emptyAttributionInfo = map.getStyleSourceInfo(
      "vector-empty-attribution",
    );
    assert.ok(emptyAttributionInfo);
    assert.equal(emptyAttributionInfo.hasAttribution, true);
    assert.equal(emptyAttributionInfo.attributionSize, 0);
    assert.equal(emptyAttributionInfo.attribution, "");
    map.addRasterSourceUrl("raster-url", "https://example.test/raster.json", {
      tileSize: 256,
    });
    assert.deepEqual(map.getStyleSourceType("raster-url"), {
      kind: "raster",
      rawType: 2,
    });
    map.addRasterDemSourceUrl(
      "raster-dem-url",
      "https://example.test/dem.json",
      { rasterDemEncoding: "mapbox" },
    );
    assert.deepEqual(map.getStyleSourceType("raster-dem-url"), {
      kind: "raster-dem",
      rawType: 3,
    });
    map.addVectorSourceTiles(
      "vector-tiles",
      ["https://example.test/vector/{z}/{x}/{y}.pbf"],
      { scheme: "tms", vectorEncoding: "mlt" },
    );
    assert.deepEqual(map.getStyleSourceType("vector-tiles"), {
      kind: "vector",
      rawType: 1,
    });
    map.addRasterSourceTiles("raster-tiles", [
      "https://example.test/raster/{z}/{x}/{y}.png",
    ]);
    assert.deepEqual(map.getStyleSourceType("raster-tiles"), {
      kind: "raster",
      rawType: 2,
    });
    map.addRasterDemSourceTiles(
      "raster-dem-tiles",
      ["https://example.test/dem/{z}/{x}/{y}.png"],
      { rasterDemEncoding: "terrarium", tileSize: 512 },
    );
    assert.deepEqual(map.getStyleSourceType("raster-dem-tiles"), {
      kind: "raster-dem",
      rawType: 3,
    });
    assert.throws(
      () =>
        map.addVectorSourceTiles(
          "vector-tiles-string",
          /** @type {any} */ ("https://example.test/{z}/{x}/{y}.pbf"),
        ),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.addStyleSourceJson("non-finite", {
          type: "geojson",
          data: { type: "Point", coordinates: [Number.NaN, 0] },
        }),
      /JSON numbers must be finite/,
    );
    map.addCustomGeometrySource("custom-geometry", {
      fetchTile() {},
      cancelTile() {},
      minZoom: 0,
      maxZoom: 14,
      tolerance: 0.375,
      tileSize: 512,
      buffer: 128,
      clip: true,
      wrap: true,
    });
    assert.equal(map.styleSourceExists("custom-geometry"), true);
    assert.throws(
      () =>
        map.addCustomGeometrySource("custom-geometry-invalid", {
          fetchTile: /** @type {any} */ ("nope"),
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.addCustomGeometrySource(
          "custom-geometry-missing-fetch",
          /** @type {any} */ ({}),
        ),
      InvalidArgumentError,
    );
    map.setCustomGeometrySourceTileData(
      "custom-geometry",
      { z: 0, x: 0, y: 0 },
      geojsonData,
    );
    map.invalidateCustomGeometrySourceTile("custom-geometry", {
      z: 0,
      x: 0,
      y: 0,
    });
    map.invalidateCustomGeometrySourceRegion("custom-geometry", {
      southwest: { latitude: -1, longitude: -2 },
      northeast: { latitude: 1, longitude: 2 },
    });
    assert.equal(map.removeStyleSource("custom-geometry"), true);
    map.addHillshadeLayer("hillshade", "raster-dem-url");
    map.addColorReliefLayer("color-relief", "raster-dem-tiles", "hillshade");
    assert.equal(map.getStyleLayerType("hillshade"), "hillshade");
    assert.equal(map.getStyleLayerType("color-relief"), "color-relief");
    assert.equal(map.removeStyleLayer("color-relief"), true);
    assert.equal(map.removeStyleLayer("hillshade"), true);

    const inlineImage = {
      width: 1,
      height: 1,
      pixels: new Uint8Array([0, 255, 0, 255]),
    };
    map.addImageSourceUrl(
      "image-source",
      imageCoordinates,
      "https://example.test/image.png",
    );
    assert.deepEqual(map.getStyleSourceType("image-source"), {
      kind: "image",
      rawType: 5,
    });
    map.setImageSourceUrl("image-source", "https://example.test/updated.png");
    map.setImageSourceImage("image-source", inlineImage);
    map.setImageSourceCoordinates("image-source", imageCoordinates);
    assert.deepEqual(
      map.getImageSourceCoordinates("image-source"),
      imageCoordinates,
    );
    map.addImageSourceImage(
      "inline-image-source",
      imageCoordinates,
      inlineImage,
    );
    assert.deepEqual(map.getStyleSourceType("inline-image-source"), {
      kind: "image",
      rawType: 5,
    });
    assert.equal(map.getImageSourceCoordinates("missing-source"), null);
    assert.equal(map.removeStyleSource("inline-image-source"), true);
    assert.equal(map.removeStyleSource("image-source"), true);
    assert.equal(map.removeStyleSource("raster-dem-tiles"), true);
    assert.equal(map.removeStyleSource("raster-tiles"), true);
    assert.equal(map.removeStyleSource("vector-tiles"), true);
    assert.equal(map.removeStyleSource("raster-dem-url"), true);
    assert.equal(map.removeStyleSource("raster-url"), true);
    assert.equal(map.removeStyleSource("vector-url"), true);
    assert.equal(map.removeStyleSource("geojson-data"), true);
    assert.equal(map.removeStyleSource("geojson-url"), true);
    assert.equal(map.removeStyleSource("empty-geojson"), true);

    map.addLocationIndicatorLayer("location");
    map.setLocationIndicatorLocation(
      "location",
      { latitude: 1, longitude: 2 },
      3,
    );
    map.setLocationIndicatorBearing("location", 45);
    map.setLocationIndicatorAccuracyRadius("location", 12);
    map.setLocationIndicatorImageName("location", "top", "red-pixel");
    assert.equal(map.getStyleLayerType("location"), "location-indicator");
    assert.equal(map.removeStyleLayer("location"), true);
    assert.throws(
      () =>
        map.setLocationIndicatorImageName(
          "location",
          /** @type {any} */ ("halo"),
          "x",
        ),
      InvalidArgumentError,
    );

    map.addStyleLayerJson({
      id: "background",
      type: "background",
      paint: { "background-color": "#000000" },
    });
    map.addStyleLayerJson({ id: "background-2", type: "background" });
    map.moveStyleLayer("background-2", "background");
    map.setLayerProperty("background", "background-color", "#ff0000");
    const backgroundColor = map.getLayerProperty(
      "background",
      "background-color",
    );
    assert.deepEqual(backgroundColor, ["rgba", 255, 0, 0, 1]);
    assert.equal(
      map.getLayerProperty("background", "background-opacity"),
      null,
    );
    assert.equal(map.getLayerFilter("background"), null);
    assert.equal(map.styleLayerExists("background"), true);
    assert.equal(map.listStyleLayerIds().includes("background"), true);
    assert.equal(map.getStyleLayerType("background"), "background");
    const backgroundLayer = map.getStyleLayerJson("background");
    assert.ok(
      backgroundLayer &&
        typeof backgroundLayer === "object" &&
        !Array.isArray(backgroundLayer),
    );
    assert.equal(backgroundLayer.id, "background");
    assert.equal(map.getStyleLayerType("missing-layer"), null);
    assert.equal(map.getStyleLayerJson("missing-layer"), null);
    assert.equal(map.removeStyleLayer("background-2"), true);
    assert.equal(map.removeStyleLayer("background"), true);
    assert.throws(
      () => map.addStyleLayerJson(/** @type {any} */ (undefined)),
      InvalidArgumentError,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("custom geometry callback retention follows current style ownership", async () => {
  const runtime = new RuntimeHandle();
  runtime.setResourceProviderRoutes(
    [{ kind: "style", url: "custom://empty-style.json" }],
    (request) => {
      assert.equal(request.kind, "style");
      assert.throws(
        () =>
          request.handle.complete({
            bytes: new TextEncoder().encode(EMPTY_STYLE_JSON),
            etag: "invalid\0etag",
          }),
        InvalidArgumentError,
      );
      assert.equal(request.handle.closed, false);
      assert.throws(
        () =>
          request.handle.complete({
            status: "error",
            errorReason: "other",
            errorMessage: "invalid\0message",
          }),
        InvalidArgumentError,
      );
      assert.equal(request.handle.closed, false);
      request.handle.complete({
        bytes: new TextEncoder().encode(EMPTY_STYLE_JSON),
      });
    },
  );
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setStyleJson(EMPTY_STYLE_JSON);
    map.addCustomGeometrySource("custom-geometry", {
      fetchTile() {},
    });
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      1,
    );

    /** @type {any} */ (map)._releaseDetachedCustomGeometrySources();
    assert.deepEqual(map.getStyleSourceType("custom-geometry"), {
      kind: "custom-vector",
      rawType: 8,
    });
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      1,
    );

    assert.equal(map.removeStyleSource("custom-geometry"), true);
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      0,
    );

    map.addCustomGeometrySource("custom-geometry", {
      fetchTile() {},
    });
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      1,
    );
    map.setStyleJson(EMPTY_STYLE_JSON);
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      0,
    );

    map.addCustomGeometrySource("custom-geometry", {
      fetchTile() {},
    });
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      1,
    );
    for (let i = 0; i < 5; i += 1) {
      runtime.runOnce();
      while (runtime.pollEvent() != null) {}
    }

    map.setStyleUrl("custom://empty-style.json");
    await eventually(() => {
      runtime.runOnce();
      let event;
      while ((event = runtime.pollEvent()) != null) {
        if (event.eventType === "map-style-loaded") {
          return event;
        }
      }
      return null;
    });
    assert.equal(
      /** @type {any} */ (map)._customGeometrySourceCountForTesting(),
      0,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("style existence and removal probes return copied booleans", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setStyleJson('{"version":8,"sources":{},"layers":[]}');
    runtime.runOnce();
    assert.equal(map.styleSourceExists("missing-source"), false);
    assert.equal(map.removeStyleSource("missing-source"), false);
    assert.equal(map.styleLayerExists("missing-layer"), false);
    assert.equal(map.removeStyleLayer("missing-layer"), false);
  } finally {
    map.close();
    runtime.close();
  }
});

test("map debug options map stable strings to native bitmasks", () => {
  const runtime = new RuntimeHandle();
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setDebugOptions(["tileBorders", "collision"]);
    assert.deepEqual(map.getDebugOptions(), ["tileBorders", "collision"]);
    assert.equal(typeof map.getDebugOptionsRawMask(), "number");
    map.setDebugOptions([]);
    assert.deepEqual(map.getDebugOptions(), []);
    assert.throws(
      () => map.setDebugOptions([/** @type {any} */ ("wireframe")]),
      InvalidArgumentError,
    );
    assert.equal(
      MapHandle.prototype.getDebugOptionsRawMask.call({
        native: {
          closed: false,
          getDebugOptionsRaw() {
            return 0x40000000;
          },
        },
      }),
      0x40000000,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

test("map options reject unknown map modes", () => {
  const runtime = new RuntimeHandle();
  assert.throws(
    () => runtime.createMap({ mapMode: /** @type {any} */ ("globe") }),
    InvalidArgumentError,
  );
  runtime.close();
});

test("runtime options reject invalid bigint values", () => {
  assert.throws(
    () => new RuntimeHandle({ maximumCacheSize: -1n }),
    (error) => {
      if (!(error instanceof InvalidArgumentError)) {
        return false;
      }
      assert.equal(error.nativeStatusCode, null);
      assert.match(error.diagnostic, /maximumCacheSize/);
      return true;
    },
  );
});

test("integer descriptors reject fractional values before native coercion", () => {
  assert.throws(
    () =>
      renderTargetExtentPhysicalSize({
        width: 1.5,
        height: 1,
        scaleFactor: 1,
      }),
    InvalidArgumentError,
  );
  const runtime = new RuntimeHandle();
  assert.throws(
    () => runtime.createMap({ width: 16.5, height: 16 }),
    InvalidArgumentError,
  );
  const map = runtime.createMap({ width: 16, height: 16 });

  try {
    map.setStyleJson(EMPTY_STYLE_JSON);
    assert.throws(
      () =>
        map.setCustomGeometrySourceTileData(
          "custom",
          { z: 0, x: 1.5, y: 0 },
          { type: "FeatureCollection", features: [] },
        ),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.invalidateCustomGeometrySourceTile("custom", {
          z: 0,
          x: 0,
          y: -1,
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.addVectorSourceTiles(
          "fractional-tile-size",
          ["https://example.test/{z}/{x}/{y}"],
          { tileSize: 511.5 },
        ),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.addCustomGeometrySource("fractional-buffer", {
          fetchTile() {},
          buffer: 1.5,
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        map.setStyleImage("fractional-image", {
          width: 1.5,
          height: 1,
          pixels: new Uint8Array(4),
        }),
      InvalidArgumentError,
    );
    assert.throws(
      () => map.setTileOptions({ prefetchZoomDelta: 1.5 }),
      InvalidArgumentError,
    );
    assert.throws(
      () => map.setViewportOptions({ viewportModeRaw: 1.5 }),
      InvalidArgumentError,
    );
    assert.throws(
      () =>
        RenderSessionHandle.prototype.resize.call(
          { native: { closed: false, resize() {} } },
          1.5,
          1,
          1,
        ),
      InvalidArgumentError,
    );
  } finally {
    map.close();
    runtime.close();
  }
});

/** @template T @param {() => T | false | null | undefined} predicate */
async function eventually(predicate) {
  const deadline = Date.now() + 500;
  while (Date.now() < deadline) {
    const value = predicate();
    if (value) {
      return value;
    }
    await new Promise((resolve) => setTimeout(resolve, 10));
  }
  const value = predicate();
  assert.ok(value);
  return value;
}

test("binding-owned validation rejects unknown network status strings", () => {
  assert.throws(
    () => setNetworkStatus(/** @type {any} */ ("airplane")),
    (error) => {
      if (!(error instanceof InvalidArgumentError)) {
        return false;
      }
      assert.equal(error instanceof MaplibreError, true);
      assert.equal(error.status, MaplibreStatus.invalidArgument);
      assert.equal(error.nativeStatusCode, null);
      assert.match(error.diagnostic, /network status/);
      return true;
    },
  );
  assert.throws(
    () => setNetworkStatus(/** @type {any} */ ({ kind: "unknown", raw: -1 })),
    InvalidArgumentError,
  );
  assert.throws(
    () =>
      setNetworkStatus(
        /** @type {any} */ ({ kind: "unknown", raw: 0x1_0000_0000 }),
      ),
    InvalidArgumentError,
  );
  assert.throws(
    () => setNetworkStatus(/** @type {any} */ ({ kind: "offline", raw: 1 })),
    InvalidArgumentError,
  );
});
