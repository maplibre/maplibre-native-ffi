"use strict";

const native = require("./index.js");

const EXPECTED_C_ABI_VERSION = 0;

const MaplibreStatus = Object.freeze({
  invalidArgument: "invalid-argument",
  invalidState: "invalid-state",
  wrongThread: "wrong-thread",
  unsupported: "unsupported",
  nativeError: "native-error",
  abiVersionMismatch: "abi-version-mismatch",
  unknownStatus: "unknown-status",
});

const NATIVE_ERROR_PREFIX = "MaplibreNativeError:";

class MaplibreError extends Error {
  constructor(status, nativeStatusCode, diagnostic, options) {
    const detail =
      diagnostic && diagnostic.trim()
        ? diagnostic
        : "No native diagnostic available.";
    super(
      `${status}${nativeStatusCode == null ? "" : ` (${nativeStatusCode})`}: ${detail}`,
      options,
    );
    this.name = "MaplibreError";
    this.status = status;
    this.nativeStatusCode = nativeStatusCode ?? null;
    this.diagnostic = diagnostic ?? "";
  }
}

class InvalidArgumentError extends MaplibreError {
  constructor(nativeStatusCode, diagnostic, options) {
    super(
      MaplibreStatus.invalidArgument,
      nativeStatusCode,
      diagnostic,
      options,
    );
    this.name = "InvalidArgumentError";
  }
}

class InvalidStateError extends MaplibreError {
  constructor(nativeStatusCode, diagnostic, options) {
    super(MaplibreStatus.invalidState, nativeStatusCode, diagnostic, options);
    this.name = "InvalidStateError";
  }
}

class WrongThreadError extends MaplibreError {
  constructor(nativeStatusCode, diagnostic, options) {
    super(MaplibreStatus.wrongThread, nativeStatusCode, diagnostic, options);
    this.name = "WrongThreadError";
  }
}

class UnsupportedFeatureError extends MaplibreError {
  constructor(nativeStatusCode, diagnostic, options) {
    super(MaplibreStatus.unsupported, nativeStatusCode, diagnostic, options);
    this.name = "UnsupportedFeatureError";
  }
}

class NativeError extends MaplibreError {
  constructor(nativeStatusCode, diagnostic, options) {
    super(MaplibreStatus.nativeError, nativeStatusCode, diagnostic, options);
    this.name = "NativeError";
  }
}

function cVersion() {
  return native.cVersion();
}

function supportedRenderBackends() {
  return native.supportedRenderBackends();
}

function supportedOpenGLContextProviders() {
  return native.supportedOpenGLContextProviders();
}

function renderTargetExtentPhysicalSize(extent) {
  return translateNativeErrors(() =>
    native.renderTargetExtentPhysicalSize(extent),
  );
}

function threadLastErrorMessage() {
  return native.threadLastErrorMessage();
}

function takeNativeLeakReports() {
  return translateNativeErrors(() => native.nativeTakeLeakReports());
}

function networkStatus() {
  return translateNativeErrors(() => native.networkStatus());
}

function setNetworkStatus(status) {
  if (status !== "online" && status !== "offline") {
    throw new InvalidArgumentError(
      null,
      `network status must be 'online' or 'offline', got '${status}'`,
    );
  }
  return translateNativeErrors(() => native.setNetworkStatus(status));
}

function projectedMetersForLatLng(coordinate) {
  return translateNativeErrors(() =>
    native.nativeProjectedMetersForLatLng(coordinate),
  );
}

function latLngForProjectedMeters(meters) {
  return translateNativeErrors(() =>
    native.nativeLatLngForProjectedMeters(meters),
  );
}

function setLogCallback(callback) {
  if (typeof callback !== "function") {
    throw new InvalidArgumentError(null, "log callback must be a function");
  }
  return translateNativeErrors(() =>
    native.nativeSetLogCallback((error, record) => {
      if (error) {
        return;
      }
      try {
        callback(record);
      } catch {
        // User logging callbacks must not escape binding-managed callbacks.
      }
    }),
  );
}

function clearLogCallback() {
  return translateNativeErrors(() => native.nativeClearLogCallback());
}

function setAsyncLogSeverities(severities) {
  let mask = 0;
  for (const severity of severities) {
    mask |= translateNativeErrors(() =>
      native.nativeLogSeverityMaskBit(severity),
    );
  }
  return translateNativeErrors(() =>
    native.nativeSetAsyncLogSeverityMask(mask),
  );
}

function restoreDefaultAsyncLogSeverities() {
  return translateNativeErrors(() =>
    native.nativeSetAsyncLogSeverityMask(
      native.nativeDefaultAsyncLogSeverityMask(),
    ),
  );
}

const MAP_DEBUG_OPTIONS = Object.freeze([
  "tileBorders",
  "parseStatus",
  "timestamps",
  "collision",
  "overdraw",
  "stencilClip",
  "depthBuffer",
]);

function mapDebugOptionMaskBit(option) {
  return translateNativeErrors(() =>
    native.nativeMapDebugOptionMaskBit(option),
  );
}

const CONSTRUCTION_TOKEN = Symbol("constructionToken");

class NativePointer {
  static null = new NativePointer(CONSTRUCTION_TOKEN, 0n);

  static unsafeFromAddress(address) {
    return new NativePointer(CONSTRUCTION_TOKEN, address);
  }

  constructor(token, address, isValid = () => true) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "use NativePointer.unsafeFromAddress() to construct native pointers",
      );
    }
    if (typeof address !== "bigint") {
      throw new InvalidArgumentError(
        null,
        "native pointer address must be a bigint",
      );
    }
    if (address < 0n) {
      throw new InvalidArgumentError(
        null,
        "native pointer address must be non-negative",
      );
    }
    Object.defineProperties(this, {
      _address: { value: address },
      _isValid: { value: isValid },
    });
    Object.freeze(this);
  }

  get address() {
    this.#assertValid();
    return this._address;
  }

  get isNull() {
    return this.address === 0n;
  }

  equals(other) {
    return other instanceof NativePointer && this.address === other.address;
  }

  toString() {
    return `NativePointer[address=0x${this.address.toString(16)}]`;
  }

  #assertValid() {
    if (!this._isValid()) {
      throw new InvalidStateError(null, "native pointer scope is closed");
    }
  }
}

const HANDLE_ENVIRONMENT = Symbol("handleEnvironment");
const ENVIRONMENT_TOKEN = Object.freeze({});
const NATIVE_HANDLES = new WeakMap();
const MAP_NATIVE_ADDRESSES = new WeakMap();

function recordHandleEnvironment(handle) {
  Object.defineProperty(handle, HANDLE_ENVIRONMENT, {
    value: ENVIRONMENT_TOKEN,
  });
}

function assertHandleEnvironment(handle) {
  if (handle?.[HANDLE_ENVIRONMENT] !== ENVIRONMENT_TOKEN) {
    throw new InvalidStateError(
      null,
      "handle belongs to a different N-API environment",
    );
  }
}

function defineCheckedNative(owner, nativeHandle) {
  assertHandleEnvironment(owner);
  NATIVE_HANDLES.set(owner, nativeHandle);
}

function nativeOf(owner) {
  if (NATIVE_HANDLES.has(owner)) {
    assertHandleEnvironment(owner);
    return NATIVE_HANDLES.get(owner);
  }
  if (
    process.env.MAPLIBRE_NATIVE_FFI_NODE_TEST_SEAMS === "1" &&
    owner != null &&
    Object.prototype.hasOwnProperty.call(owner, "native")
  ) {
    return owner.native;
  }
  assertHandleEnvironment(owner);
  throw new InvalidStateError(null, "native handle is not initialized");
}

function liveNativeOf(owner) {
  const nativeHandle = nativeOf(owner);
  if (nativeHandle.closed) {
    throw new InvalidStateError(null, "handle is closed");
  }
  return nativeHandle;
}

class NativeBuffer {
  static allocate(byteLength) {
    return new NativeBuffer(new ArrayBuffer(validateByteLength(byteLength)));
  }

  static from(data) {
    if (data instanceof NativeBuffer) {
      return new NativeBuffer(data.asUint8Array());
    }
    if (data instanceof ArrayBuffer) {
      return new NativeBuffer(data.slice(0));
    }
    if (ArrayBuffer.isView(data)) {
      return new NativeBuffer(data);
    }
    throw new InvalidArgumentError(
      null,
      "native buffer data must be an ArrayBuffer or typed array view",
    );
  }

  constructor(data) {
    if (typeof data === "number") {
      this.buffer = new ArrayBuffer(validateByteLength(data));
    } else if (data instanceof ArrayBuffer) {
      this.buffer = data.slice(0);
    } else if (ArrayBuffer.isView(data)) {
      const copy = new Uint8Array(data.byteLength);
      copy.set(new Uint8Array(data.buffer, data.byteOffset, data.byteLength));
      this.buffer = copy.buffer;
    } else {
      throw new InvalidArgumentError(
        null,
        "native buffer constructor requires a byte length, ArrayBuffer, or typed array view",
      );
    }
  }

  get byteLength() {
    return this.buffer.byteLength;
  }

  asArrayBuffer() {
    return this.buffer;
  }

  asUint8Array() {
    return new Uint8Array(this.buffer);
  }

  get [Symbol.toStringTag]() {
    return "NativeBuffer";
  }
}

class MetalOwnedTextureFrame {
  #active = true;
  #raw;
  #session;

  constructor(token, raw, session) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "texture frames are created by RenderSessionHandle frame acquisition",
      );
    }
    this.#raw = raw;
    this.#session = session;
    Object.freeze(this);
  }

  get generation() {
    return this.#read("generation");
  }

  get width() {
    return this.#read("width");
  }

  get height() {
    return this.#read("height");
  }

  get scaleFactor() {
    return this.#read("scaleFactor");
  }

  get frameId() {
    return this.#read("frameId");
  }

  get texture() {
    return new NativePointer(
      CONSTRUCTION_TOKEN,
      this.#read("textureAddress"),
      () => this.#active,
    );
  }

  get device() {
    return new NativePointer(
      CONSTRUCTION_TOKEN,
      this.#read("deviceAddress"),
      () => this.#active,
    );
  }

  get pixelFormat() {
    return this.#read("pixelFormat");
  }

  #read(field) {
    if (!this.#active) {
      throw new InvalidStateError(null, "texture frame scope is closed");
    }
    return this.#raw[field];
  }

  close() {
    if (!this.#active) {
      return;
    }
    translateNativeErrors(() =>
      liveNativeOf(this.#session).releaseMetalOwnedTextureFrame(this.#raw),
    );
    this.#active = false;
  }

  get closed() {
    return !this.#active;
  }

  [Symbol.dispose]() {
    this.close();
  }
}

class OpenGLOwnedTextureFrame {
  #active = true;
  #raw;
  #session;

  constructor(token, raw, session) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "texture frames are created by RenderSessionHandle frame acquisition",
      );
    }
    this.#raw = raw;
    this.#session = session;
    Object.freeze(this);
  }

  get generation() {
    return this.#read("generation");
  }
  get width() {
    return this.#read("width");
  }
  get height() {
    return this.#read("height");
  }
  get scaleFactor() {
    return this.#read("scaleFactor");
  }
  get frameId() {
    return this.#read("frameId");
  }
  get texture() {
    return this.#read("texture");
  }
  get target() {
    return this.#read("target");
  }
  get internalFormat() {
    return this.#read("internalFormat");
  }
  get format() {
    return this.#read("format");
  }
  get type() {
    return this.#read("type");
  }

  #read(field) {
    if (!this.#active) {
      throw new InvalidStateError(null, "texture frame scope is closed");
    }
    return this.#raw[field];
  }

  close() {
    if (!this.#active) {
      return;
    }
    translateNativeErrors(() =>
      liveNativeOf(this.#session).releaseOpenGLOwnedTextureFrame(this.#raw),
    );
    this.#active = false;
  }

  get closed() {
    return !this.#active;
  }

  [Symbol.dispose]() {
    this.close();
  }
}

class VulkanOwnedTextureFrame {
  #active = true;
  #raw;
  #session;

  constructor(token, raw, session) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "texture frames are created by RenderSessionHandle frame acquisition",
      );
    }
    this.#raw = raw;
    this.#session = session;
    Object.freeze(this);
  }

  get generation() {
    return this.#read("generation");
  }

  get width() {
    return this.#read("width");
  }

  get height() {
    return this.#read("height");
  }

  get scaleFactor() {
    return this.#read("scaleFactor");
  }

  get frameId() {
    return this.#read("frameId");
  }

  get image() {
    return new NativePointer(
      CONSTRUCTION_TOKEN,
      this.#read("imageAddress"),
      () => this.#active,
    );
  }

  get imageView() {
    return new NativePointer(
      CONSTRUCTION_TOKEN,
      this.#read("imageViewAddress"),
      () => this.#active,
    );
  }

  get device() {
    return new NativePointer(
      CONSTRUCTION_TOKEN,
      this.#read("deviceAddress"),
      () => this.#active,
    );
  }

  get format() {
    return this.#read("format");
  }

  get layout() {
    return this.#read("layout");
  }

  #read(field) {
    if (!this.#active) {
      throw new InvalidStateError(null, "texture frame scope is closed");
    }
    return this.#raw[field];
  }

  close() {
    if (!this.#active) {
      return;
    }
    translateNativeErrors(() =>
      liveNativeOf(this.#session).releaseVulkanOwnedTextureFrame(this.#raw),
    );
    this.#active = false;
  }

  get closed() {
    return !this.#active;
  }

  [Symbol.dispose]() {
    this.close();
  }
}

function requireOfflineOperation(
  operation,
  runtime,
  expectedOperationKind,
  expectedResultKind,
) {
  if (operation instanceof OfflineOperationHandle) {
    return operation._requireLive(
      runtime,
      expectedOperationKind,
      expectedResultKind,
    );
  }
  throw new InvalidArgumentError(
    null,
    "offline operation must be an OfflineOperationHandle",
  );
}

function takeOfflineOperation(
  operation,
  runtime,
  expectedOperationKind,
  expectedResultKind,
  take,
) {
  const operationId = requireOfflineOperation(
    operation,
    runtime,
    expectedOperationKind,
    expectedResultKind,
  );
  const result = translateNativeErrors(() => take(operationId));
  operation._markConsumed();
  return translateOfflineOperationResult(result);
}

function translateOfflineOperationResult(result) {
  return result;
}

function validateByteLength(byteLength) {
  if (!Number.isSafeInteger(byteLength) || byteLength < 0) {
    throw new InvalidArgumentError(
      null,
      "native buffer byteLength must be a non-negative safe integer",
    );
  }
  return byteLength;
}

function mutableUint8Array(data, fieldName) {
  if (data instanceof NativeBuffer) {
    return data.asUint8Array();
  }
  if (data instanceof ArrayBuffer) {
    return new Uint8Array(data);
  }
  if (ArrayBuffer.isView(data)) {
    return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
  }
  throw new InvalidArgumentError(
    null,
    `${fieldName} must be a NativeBuffer, ArrayBuffer, or typed array view`,
  );
}

const resourceRequestFinalizer =
  typeof FinalizationRegistry === "function"
    ? new FinalizationRegistry((completionToken) => {
        try {
          native.nativeResourceRequestClose(completionToken);
        } catch {
          // Finalizers are best-effort cleanup only.
        }
      })
    : null;

function resourceProviderErrorResponse(error) {
  const message =
    error && typeof error.message === "string"
      ? error.message
      : "resource provider callback failed";
  return {
    status: "error",
    errorReason: "other",
    errorMessage: message,
  };
}

function completeResourceRequestWithProviderError(handle, error) {
  if (handle.closed) {
    return;
  }
  try {
    handle.complete(resourceProviderErrorResponse(error));
  } catch {
    try {
      handle.close();
    } catch {
      // The request may have been cancelled or completed already.
    }
  }
}

function customGeometryCallback(callback) {
  if (callback == null) {
    return null;
  }
  return (error, tileId) => {
    if (error) {
      return;
    }
    try {
      callback(tileId);
    } catch {
      // Native custom geometry callbacks must not escape into the event loop.
    }
  };
}

class ResourceRequestHandle {
  #completionToken;
  #closed = false;

  constructor(token, completionToken) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "resource request handles are created by resource provider callbacks",
      );
    }
    recordHandleEnvironment(this);
    this.#completionToken = completionToken;
    resourceRequestFinalizer?.register(this, completionToken, this);
    Object.preventExtensions(this);
  }

  get closed() {
    assertHandleEnvironment(this);
    return this.#closed;
  }

  complete(response = {}) {
    assertHandleEnvironment(this);
    if (this.#closed) {
      throw new InvalidStateError(null, "ResourceRequestHandle is closed");
    }
    let consumed = false;
    try {
      const result = translateNativeErrors(() =>
        native.nativeResourceRequestComplete(this.#completionToken, response),
      );
      consumed = true;
      return result;
    } catch (error) {
      if (!(error instanceof InvalidArgumentError)) {
        consumed = true;
      }
      throw error;
    } finally {
      if (consumed) {
        this.#closed = true;
        resourceRequestFinalizer?.unregister(this);
      }
    }
  }

  cancelled() {
    assertHandleEnvironment(this);
    if (this.#closed) {
      throw new InvalidStateError(null, "ResourceRequestHandle is closed");
    }
    return translateNativeErrors(() =>
      native.nativeResourceRequestCancelled(this.#completionToken),
    );
  }

  close() {
    assertHandleEnvironment(this);
    if (this.#closed) {
      return;
    }
    translateNativeErrors(() =>
      native.nativeResourceRequestClose(this.#completionToken),
    );
    this.#closed = true;
    resourceRequestFinalizer?.unregister(this);
  }

  [Symbol.dispose]() {
    this.close();
  }
}

const offlineOperationFinalizer =
  typeof FinalizationRegistry === "function"
    ? new FinalizationRegistry((registration) => {
        if (registration.closed) {
          return;
        }
        try {
          liveNativeOf(registration.runtime).discardOfflineOperation(
            registration.operationId,
          );
          registration.closed = true;
          registration.runtime._unregisterOfflineOperation(registration);
        } catch (error) {
          process.emitWarning(
            `Leaked offline operation ${registration.operationKind} could not be discarded: ${String(error)}`,
          );
        }
      })
    : null;

class OfflineOperationHandle {
  #runtime;
  #operationId;
  #operationKind;
  #resultKind;
  #registration;
  #closed = false;

  constructor(
    token,
    runtime,
    operationId,
    operationKind = "unknown",
    resultKind = "unknown",
  ) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "offline operation handles are created by RuntimeHandle operations",
      );
    }
    recordHandleEnvironment(this);
    if (!(runtime instanceof RuntimeHandle)) {
      throw new InvalidArgumentError(null, "runtime must be a RuntimeHandle");
    }
    if (typeof operationId !== "bigint" || operationId <= 0n) {
      throw new InvalidArgumentError(
        null,
        "offline operation id must be a positive bigint",
      );
    }
    this.#runtime = runtime;
    this.#operationId = operationId;
    this.#operationKind = operationKind;
    this.#resultKind = resultKind;
    this.#registration = {
      runtime,
      operationId,
      operationKind,
      closed: false,
      handle: new WeakRef(this),
    };
    runtime._registerOfflineOperation(this.#registration);
    offlineOperationFinalizer?.register(this, this.#registration, this);
    Object.preventExtensions(this);
  }

  close() {
    assertHandleEnvironment(this);
    if (this.#closed) {
      return;
    }
    translateNativeErrors(() =>
      liveNativeOf(this.#runtime).discardOfflineOperation(this.#operationId),
    );
    this.#markClosed();
  }

  get closed() {
    assertHandleEnvironment(this);
    return this.#closed;
  }

  _requireLive(expectedRuntime, expectedOperationKind, expectedResultKind) {
    assertHandleEnvironment(this);
    if (this.#closed) {
      throw new InvalidStateError(null, "offline operation handle is closed");
    }
    if (this.#runtime !== expectedRuntime) {
      throw new InvalidStateError(
        null,
        "OfflineOperationHandle belongs to a different RuntimeHandle",
      );
    }
    if (
      this.#operationKind !== expectedOperationKind ||
      this.#resultKind !== expectedResultKind
    ) {
      throw new InvalidStateError(
        null,
        `OfflineOperationHandle has kind ${this.#operationKind} and result kind ${this.#resultKind}, expected ${expectedOperationKind} and ${expectedResultKind}`,
      );
    }
    return this.#operationId;
  }

  _markConsumed() {
    assertHandleEnvironment(this);
    this.#markClosed();
  }

  #markClosed() {
    if (!this.#closed) {
      this.#closed = true;
      this.#registration.closed = true;
      this.#runtime._unregisterOfflineOperation(this.#registration);
      offlineOperationFinalizer?.unregister(this);
    }
  }

  [Symbol.dispose]() {
    this.close();
  }
}

class RuntimeHandle {
  #mapsByAddress = new Map();
  #offlineOperations = new Set();

  constructor(options) {
    recordHandleEnvironment(this);
    assertNativeAbiVersion();
    defineCheckedNative(
      this,
      translateNativeErrors(() =>
        native.createNativeRuntimeHandle(options ?? {}),
      ),
    );
  }

  createMap(options) {
    return new MapHandle(CONSTRUCTION_TOKEN, this, options);
  }

  close() {
    if (this.#offlineOperations.size > 0) {
      throw new InvalidStateError(
        null,
        "runtime has live offline operation handles",
      );
    }
    const result = translateNativeErrors(() => nativeOf(this).close());
    this.#mapsByAddress.clear();
    return result;
  }

  get closed() {
    return nativeOf(this).closed;
  }

  runOnce() {
    return translateNativeErrors(() => liveNativeOf(this).runOnce());
  }

  setResourceTransformRules(rules) {
    if (!Array.isArray(rules)) {
      throw new InvalidArgumentError(
        null,
        "resource transform rules must be an array",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).setResourceTransformRules(rules),
    );
  }

  setResourceProviderRoutes(routes, callback) {
    if (!Array.isArray(routes)) {
      throw new InvalidArgumentError(
        null,
        "resource provider routes must be an array",
      );
    }
    if (typeof callback !== "function") {
      throw new InvalidArgumentError(
        null,
        "resource provider callback must be a function",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).setResourceProviderRoutes(routes, (error, request) => {
        if (error) {
          throw error;
        }
        const handle = new ResourceRequestHandle(
          CONSTRUCTION_TOKEN,
          request.completionToken,
        );
        const wrapped = {
          ...request,
          handle,
        };
        delete wrapped.completionToken;
        try {
          const result = callback(wrapped);
          if (result && typeof result.then === "function") {
            Promise.resolve(result).catch((error) => {
              completeResourceRequestWithProviderError(handle, error);
            });
          }
        } catch (error) {
          completeResourceRequestWithProviderError(handle, error);
        }
      }),
    );
  }

  clearResourceTransform() {
    return translateNativeErrors(() =>
      liveNativeOf(this).clearResourceTransform(),
    );
  }

  runAmbientCacheOperation(operation) {
    const start = translateNativeErrors(() =>
      liveNativeOf(this).runAmbientCacheOperation(operation),
    );
    return new OfflineOperationHandle(
      CONSTRUCTION_TOKEN,
      this,
      BigInt(start.operationId),
      "ambientCache",
      "none",
    );
  }

  offlineRegionsList() {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionsList(),
      "regionsList",
      "regionList",
    );
  }

  offlineRegionGet(regionId) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionGet(regionId),
      "regionGet",
      "optionalRegion",
    );
  }

  offlineRegionsMergeDatabase(path) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionsMergeDatabase(path),
      "regionsMergeDatabase",
      "regionList",
    );
  }

  offlineRegionUpdateMetadata(regionId, metadata = null) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionUpdateMetadata(regionId, metadata),
      "regionUpdateMetadata",
      "region",
    );
  }

  offlineRegionGetStatus(regionId) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionGetStatus(regionId),
      "regionGetStatus",
      "regionStatus",
    );
  }

  offlineRegionSetObserved(regionId, observed) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionSetObserved(regionId, observed),
      "regionSetObserved",
      "none",
    );
  }

  offlineRegionSetDownloadState(regionId, state) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionSetDownloadState(regionId, state),
      "regionSetDownloadState",
      "none",
    );
  }

  offlineRegionInvalidate(regionId) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionInvalidate(regionId),
      "regionInvalidate",
      "none",
    );
  }

  offlineRegionDelete(regionId) {
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionDelete(regionId),
      "regionDelete",
      "none",
    );
  }

  offlineRegionCreate(definition, metadata = null) {
    const nativeDefinition = { ...definition };
    if (definition?.geometry == null) {
      delete nativeDefinition.geometry;
    } else {
      nativeDefinition.geometry = stringifyJson(definition.geometry);
    }
    return this.#offlineOperation(
      () => liveNativeOf(this).offlineRegionCreate(nativeDefinition, metadata),
      "regionCreate",
      "region",
    );
  }

  offlineRegionCreateTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionCreate",
      "region",
      (operationId) =>
        liveNativeOf(this).offlineRegionCreateTakeResult(operationId),
    );
  }

  offlineRegionGetTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionGet",
      "optionalRegion",
      (operationId) =>
        liveNativeOf(this).offlineRegionGetTakeResult(operationId),
    );
  }

  offlineRegionsListTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionsList",
      "regionList",
      (operationId) =>
        liveNativeOf(this).offlineRegionsListTakeResult(operationId),
    );
  }

  offlineRegionsMergeDatabaseTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionsMergeDatabase",
      "regionList",
      (operationId) =>
        liveNativeOf(this).offlineRegionsMergeDatabaseTakeResult(operationId),
    );
  }

  offlineRegionUpdateMetadataTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionUpdateMetadata",
      "region",
      (operationId) =>
        liveNativeOf(this).offlineRegionUpdateMetadataTakeResult(operationId),
    );
  }

  offlineRegionGetStatusTakeResult(operation) {
    return takeOfflineOperation(
      operation,
      this,
      "regionGetStatus",
      "regionStatus",
      (operationId) =>
        liveNativeOf(this).offlineRegionGetStatusTakeResult(operationId),
    );
  }

  #offlineOperation(startOperation, operationKind, resultKind) {
    const start = translateNativeErrors(startOperation);
    return new OfflineOperationHandle(
      CONSTRUCTION_TOKEN,
      this,
      BigInt(start.operationId),
      operationKind,
      resultKind,
    );
  }

  pollEvent() {
    const event = translateNativeErrors(() => liveNativeOf(this).pollEvent());
    if (event == null) {
      return null;
    }
    if (event?.eventType === "map-style-loaded" && event.sourceType === "map") {
      this.#mapsByAddress
        .get(event.sourceAddress)
        ?._releaseDetachedCustomGeometrySources();
    }
    event.sourceMap =
      event.sourceType === "map"
        ? (this.#mapsByAddress.get(event.sourceAddress) ?? null)
        : null;
    delete event.sourceAddress;
    const completed = event.payload?.offlineOperationCompleted;
    if (typeof completed?.operationId === "bigint") {
      const registration = [...this.#offlineOperations].find(
        (candidate) => candidate.operationId === completed.operationId,
      );
      completed.operation = registration?.handle.deref() ?? null;
      delete completed.operationId;
    }
    return event;
  }

  _registerMap(map) {
    assertHandleEnvironment(this);
    const nativeAddress = MAP_NATIVE_ADDRESSES.get(map);
    if (nativeAddress != null) {
      this.#mapsByAddress.set(nativeAddress, map);
    }
  }

  _unregisterMap(map) {
    assertHandleEnvironment(this);
    const nativeAddress = MAP_NATIVE_ADDRESSES.get(map);
    if (nativeAddress != null) {
      this.#mapsByAddress.delete(nativeAddress);
    }
  }

  _registerOfflineOperation(registration) {
    assertHandleEnvironment(this);
    this.#offlineOperations.add(registration);
  }

  _unregisterOfflineOperation(registration) {
    assertHandleEnvironment(this);
    this.#offlineOperations.delete(registration);
  }

  [Symbol.dispose]() {
    this.close();
  }
}

class MapProjectionHandle {
  constructor(token, map) {
    recordHandleEnvironment(this);
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "map projections are created by MapHandle.createProjection()",
      );
    }
    if (!(map instanceof MapHandle)) {
      throw new InvalidArgumentError(null, "map must be a MapHandle");
    }
    defineCheckedNative(
      this,
      translateNativeErrors(() =>
        native.createNativeMapProjectionHandle(liveNativeOf(map)),
      ),
    );
  }

  close() {
    return translateNativeErrors(() => nativeOf(this).close());
  }

  get closed() {
    return nativeOf(this).closed;
  }

  getCamera() {
    return translateNativeErrors(() => liveNativeOf(this).getCamera());
  }

  setCamera(camera) {
    return translateNativeErrors(() => liveNativeOf(this).setCamera(camera));
  }

  setVisibleCoordinates(coordinates, padding) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setVisibleCoordinates(coordinates, padding),
    );
  }

  setVisibleGeometry(geometry, padding) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setVisibleGeometry(stringifyJson(geometry), padding),
    );
  }

  pixelForLatLng(coordinate) {
    return translateNativeErrors(() =>
      liveNativeOf(this).pixelForLatLng(coordinate),
    );
  }

  latLngForPixel(point) {
    return translateNativeErrors(() =>
      liveNativeOf(this).latLngForPixel(point),
    );
  }

  [Symbol.dispose]() {
    this.close();
  }
}

function nativePointerAddress(value, fieldName) {
  if (!(value instanceof NativePointer)) {
    throw new InvalidArgumentError(
      null,
      `${fieldName} must be a NativePointer`,
    );
  }
  return value.address;
}

function nullableNativePointerAddress(value, fieldName) {
  return value == null ? null : nativePointerAddress(value, fieldName);
}

function normalizeMetalContext(context = {}) {
  return {
    deviceAddress: nullableNativePointerAddress(context.device, "device"),
  };
}

function normalizeVulkanContext(context) {
  return {
    instanceAddress: nativePointerAddress(context?.instance, "instance"),
    physicalDeviceAddress: nativePointerAddress(
      context?.physicalDevice,
      "physicalDevice",
    ),
    deviceAddress: nativePointerAddress(context?.device, "device"),
    graphicsQueueAddress: nativePointerAddress(
      context?.graphicsQueue,
      "graphicsQueue",
    ),
    graphicsQueueFamilyIndex: context?.graphicsQueueFamilyIndex,
    getInstanceProcAddrAddress: nullableNativePointerAddress(
      context?.getInstanceProcAddr,
      "getInstanceProcAddr",
    ),
    getDeviceProcAddrAddress: nullableNativePointerAddress(
      context?.getDeviceProcAddr,
      "getDeviceProcAddr",
    ),
  };
}

function normalizeOpenGLContext(context) {
  const platform = context?.platform;
  if (platform === "wgl") {
    return {
      platform,
      wgl: {
        deviceContextAddress: nativePointerAddress(
          context.deviceContext,
          "deviceContext",
        ),
        shareContextAddress: nativePointerAddress(
          context.shareContext,
          "shareContext",
        ),
        getProcAddressAddress: nullableNativePointerAddress(
          context.getProcAddress,
          "getProcAddress",
        ),
      },
      egl: null,
    };
  }
  if (platform === "egl") {
    return {
      platform,
      wgl: null,
      egl: {
        displayAddress: nativePointerAddress(context.display, "display"),
        configAddress: nativePointerAddress(context.config, "config"),
        shareContextAddress: nativePointerAddress(
          context.shareContext,
          "shareContext",
        ),
        getProcAddressAddress: nullableNativePointerAddress(
          context.getProcAddress,
          "getProcAddress",
        ),
      },
    };
  }
  throw new InvalidArgumentError(
    null,
    "OpenGL context platform must be 'wgl' or 'egl'",
  );
}

function normalizeMetalOwnedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeMetalContext(descriptor?.context),
  };
}

function normalizeMetalBorrowedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    physicalWidth: descriptor?.physicalWidth,
    physicalHeight: descriptor?.physicalHeight,
    textureAddress: nativePointerAddress(descriptor?.texture, "texture"),
  };
}

function normalizeMetalSurfaceDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeMetalContext(descriptor?.context),
    layerAddress: nativePointerAddress(descriptor?.layer, "layer"),
  };
}

function normalizeVulkanOwnedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeVulkanContext(descriptor?.context),
  };
}

function normalizeVulkanBorrowedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    physicalWidth: descriptor?.physicalWidth,
    physicalHeight: descriptor?.physicalHeight,
    context: normalizeVulkanContext(descriptor?.context),
    imageAddress: nativePointerAddress(descriptor?.image, "image"),
    imageViewAddress: nativePointerAddress(descriptor?.imageView, "imageView"),
    format: descriptor?.format,
    initialLayout: descriptor?.initialLayout,
    finalLayout: descriptor?.finalLayout,
  };
}

function normalizeVulkanSurfaceDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeVulkanContext(descriptor?.context),
    surfaceAddress: nativePointerAddress(descriptor?.surface, "surface"),
  };
}

function normalizeOpenGLOwnedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeOpenGLContext(descriptor?.context),
  };
}

function normalizeOpenGLBorrowedTextureDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    physicalWidth: descriptor?.physicalWidth,
    physicalHeight: descriptor?.physicalHeight,
    context: normalizeOpenGLContext(descriptor?.context),
    texture: descriptor?.texture,
    target: descriptor?.target,
  };
}

function normalizeOpenGLSurfaceDescriptor(descriptor) {
  return {
    extent: descriptor?.extent,
    context: normalizeOpenGLContext(descriptor?.context),
    surfaceAddress: nativePointerAddress(descriptor?.surface, "surface"),
  };
}

class RenderSessionHandle {
  constructor(token, nativeHandle, map) {
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "render sessions are created by MapHandle attach methods",
      );
    }
    recordHandleEnvironment(this);
    defineCheckedNative(this, nativeHandle);
    Object.defineProperty(this, "map", {
      value: map,
      enumerable: true,
      configurable: false,
      writable: false,
    });
  }

  close() {
    return translateNativeErrors(() => nativeOf(this).close());
  }

  get closed() {
    return nativeOf(this).closed;
  }

  resize(width, height, scaleFactor) {
    return translateNativeErrors(() =>
      liveNativeOf(this).resize(width, height, scaleFactor),
    );
  }

  renderUpdate() {
    return translateNativeErrors(() => liveNativeOf(this).renderUpdate());
  }

  detach() {
    return translateNativeErrors(() => liveNativeOf(this).detach());
  }

  reduceMemoryUse() {
    return translateNativeErrors(() => liveNativeOf(this).reduceMemoryUse());
  }

  clearData() {
    return translateNativeErrors(() => liveNativeOf(this).clearData());
  }

  dumpDebugLogs() {
    return translateNativeErrors(() => liveNativeOf(this).dumpDebugLogs());
  }

  setFeatureState(selector, state) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setFeatureState(selector, stringifyJson(state)),
    );
  }

  getFeatureState(selector) {
    return translateNativeErrors(() =>
      JSON.parse(liveNativeOf(this).getFeatureState(selector)),
    );
  }

  removeFeatureState(selector) {
    return translateNativeErrors(() =>
      liveNativeOf(this).removeFeatureState(selector),
    );
  }

  queryRenderedFeatures(geometry, options = null) {
    const nativeOptions =
      options == null
        ? null
        : {
            ...options,
            filter:
              options.filter == null ? null : stringifyJson(options.filter),
          };
    return translateNativeErrors(() =>
      JSON.parse(
        liveNativeOf(this).queryRenderedFeatures(geometry, nativeOptions),
      ),
    );
  }

  querySourceFeatures(sourceId, options = null) {
    const nativeOptions =
      options == null
        ? null
        : {
            ...options,
            filter:
              options.filter == null ? null : stringifyJson(options.filter),
          };
    return translateNativeErrors(() =>
      JSON.parse(
        liveNativeOf(this).querySourceFeatures(sourceId, nativeOptions),
      ),
    );
  }

  queryFeatureExtension(
    sourceId,
    feature,
    extension,
    extensionField,
    args = null,
  ) {
    return translateNativeErrors(() =>
      JSON.parse(
        liveNativeOf(this).queryFeatureExtension(
          sourceId,
          stringifyJson(feature),
          extension,
          extensionField,
          args == null ? null : stringifyJson(args),
        ),
      ),
    );
  }

  acquireMetalOwnedTextureFrame() {
    return new MetalOwnedTextureFrame(
      CONSTRUCTION_TOKEN,
      translateNativeErrors(() =>
        liveNativeOf(this).acquireMetalOwnedTextureFrame(),
      ),
      this,
    );
  }

  acquireVulkanOwnedTextureFrame() {
    return new VulkanOwnedTextureFrame(
      CONSTRUCTION_TOKEN,
      translateNativeErrors(() =>
        liveNativeOf(this).acquireVulkanOwnedTextureFrame(),
      ),
      this,
    );
  }

  acquireOpenGLOwnedTextureFrame() {
    return new OpenGLOwnedTextureFrame(
      CONSTRUCTION_TOKEN,
      translateNativeErrors(() =>
        liveNativeOf(this).acquireOpenGLOwnedTextureFrame(),
      ),
      this,
    );
  }

  readPremultipliedRgba8Into(data) {
    return translateNativeErrors(() =>
      liveNativeOf(this).readPremultipliedRgba8Into(
        mutableUint8Array(data, "readback buffer"),
      ),
    );
  }

  [Symbol.dispose]() {
    this.close();
  }
}

function attachRenderSession(map, attach) {
  if (!(map instanceof MapHandle)) {
    throw new InvalidArgumentError(null, "map must be a MapHandle");
  }
  return new RenderSessionHandle(
    CONSTRUCTION_TOKEN,
    translateNativeErrors(attach),
    map,
  );
}

class MapHandle {
  #runtime;

  constructor(token, runtime, options) {
    recordHandleEnvironment(this);
    if (token !== CONSTRUCTION_TOKEN) {
      throw new InvalidArgumentError(
        null,
        "maps are created by RuntimeHandle.createMap()",
      );
    }
    if (!(runtime instanceof RuntimeHandle)) {
      throw new InvalidArgumentError(null, "runtime must be a RuntimeHandle");
    }
    this.#runtime = runtime;
    defineCheckedNative(
      this,
      translateNativeErrors(() =>
        native.createNativeMapHandle(liveNativeOf(runtime), options ?? {}),
      ),
    );
    MAP_NATIVE_ADDRESSES.set(this, nativeOf(this).nativeAddress);
    runtime._registerMap(this);
  }

  close() {
    const result = translateNativeErrors(() => nativeOf(this).close());
    this.#runtime._unregisterMap(this);
    return result;
  }

  get closed() {
    return nativeOf(this).closed;
  }

  createProjection() {
    return new MapProjectionHandle(CONSTRUCTION_TOKEN, this);
  }

  attachMetalOwnedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createMetalOwnedTextureRenderSession(
        liveNativeOf(this),
        normalizeMetalOwnedTextureDescriptor(descriptor),
      ),
    );
  }

  attachMetalBorrowedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createMetalBorrowedTextureRenderSession(
        liveNativeOf(this),
        normalizeMetalBorrowedTextureDescriptor(descriptor),
      ),
    );
  }

  attachMetalSurface(descriptor) {
    return attachRenderSession(this, () =>
      native.createMetalSurfaceRenderSession(
        liveNativeOf(this),
        normalizeMetalSurfaceDescriptor(descriptor),
      ),
    );
  }

  attachVulkanOwnedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createVulkanOwnedTextureRenderSession(
        liveNativeOf(this),
        normalizeVulkanOwnedTextureDescriptor(descriptor),
      ),
    );
  }

  attachVulkanBorrowedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createVulkanBorrowedTextureRenderSession(
        liveNativeOf(this),
        normalizeVulkanBorrowedTextureDescriptor(descriptor),
      ),
    );
  }

  attachVulkanSurface(descriptor) {
    return attachRenderSession(this, () =>
      native.createVulkanSurfaceRenderSession(
        liveNativeOf(this),
        normalizeVulkanSurfaceDescriptor(descriptor),
      ),
    );
  }

  attachOpenGLOwnedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createOpenGLOwnedTextureRenderSession(
        liveNativeOf(this),
        normalizeOpenGLOwnedTextureDescriptor(descriptor),
      ),
    );
  }

  attachOpenGLBorrowedTexture(descriptor) {
    return attachRenderSession(this, () =>
      native.createOpenGLBorrowedTextureRenderSession(
        liveNativeOf(this),
        normalizeOpenGLBorrowedTextureDescriptor(descriptor),
      ),
    );
  }

  attachOpenGLSurface(descriptor) {
    return attachRenderSession(this, () =>
      native.createOpenGLSurfaceRenderSession(
        liveNativeOf(this),
        normalizeOpenGLSurfaceDescriptor(descriptor),
      ),
    );
  }

  requestRepaint() {
    return translateNativeErrors(() => liveNativeOf(this).requestRepaint());
  }

  requestStillImage() {
    return translateNativeErrors(() => liveNativeOf(this).requestStillImage());
  }

  isFullyLoaded() {
    return translateNativeErrors(() => liveNativeOf(this).isFullyLoaded());
  }

  dumpDebugLogs() {
    return translateNativeErrors(() => liveNativeOf(this).dumpDebugLogs());
  }

  getDebugOptions() {
    const mask = translateNativeErrors(() =>
      liveNativeOf(this).getDebugOptionsRaw(),
    );
    return MAP_DEBUG_OPTIONS.filter((option) =>
      Boolean(mask & mapDebugOptionMaskBit(option)),
    );
  }

  setDebugOptions(options) {
    let mask = 0;
    for (const option of options) {
      mask |= mapDebugOptionMaskBit(option);
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).setDebugOptionsRaw(mask),
    );
  }

  moveBy(deltaX, deltaY) {
    return translateNativeErrors(() =>
      liveNativeOf(this).moveBy(deltaX, deltaY),
    );
  }

  scaleBy(scale, anchor = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).scaleBy(scale, anchor),
    );
  }

  rotateBy(first, second) {
    return translateNativeErrors(() =>
      liveNativeOf(this).rotateBy(first, second),
    );
  }

  pitchBy(pitch) {
    return translateNativeErrors(() => liveNativeOf(this).pitchBy(pitch));
  }

  moveByAnimated(deltaX, deltaY, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).moveByAnimated(deltaX, deltaY, animation),
    );
  }

  scaleByAnimated(scale, anchor = null, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).scaleByAnimated(scale, anchor, animation),
    );
  }

  rotateByAnimated(first, second, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).rotateByAnimated(first, second, animation),
    );
  }

  pitchByAnimated(pitch, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).pitchByAnimated(pitch, animation),
    );
  }

  cancelTransitions() {
    return translateNativeErrors(() => liveNativeOf(this).cancelTransitions());
  }

  getViewportOptions() {
    return translateNativeErrors(() => liveNativeOf(this).getViewportOptions());
  }

  setViewportOptions(options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setViewportOptions(options),
    );
  }

  getTileOptions() {
    return translateNativeErrors(() => liveNativeOf(this).getTileOptions());
  }

  setTileOptions(options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setTileOptions(options),
    );
  }

  getBounds() {
    return translateNativeErrors(() => liveNativeOf(this).getBounds());
  }

  setBounds(options) {
    return translateNativeErrors(() => liveNativeOf(this).setBounds(options));
  }

  getFreeCameraOptions() {
    return translateNativeErrors(() =>
      liveNativeOf(this).getFreeCameraOptions(),
    );
  }

  setFreeCameraOptions(options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setFreeCameraOptions(options),
    );
  }

  getProjectionMode() {
    return translateNativeErrors(() => liveNativeOf(this).getProjectionMode());
  }

  setProjectionMode(mode) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setProjectionMode(mode),
    );
  }

  getCamera() {
    return translateNativeErrors(() => liveNativeOf(this).getCamera());
  }

  jumpTo(camera) {
    return translateNativeErrors(() => liveNativeOf(this).jumpTo(camera));
  }

  easeTo(camera, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).easeTo(camera, animation),
    );
  }

  flyTo(camera, animation = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).flyTo(camera, animation),
    );
  }

  cameraForLatLngBounds(bounds, fitOptions) {
    return translateNativeErrors(() =>
      liveNativeOf(this).cameraForLatLngBounds(bounds, fitOptions ?? null),
    );
  }

  cameraForLatLngs(coordinates, fitOptions) {
    return translateNativeErrors(() =>
      liveNativeOf(this).cameraForLatLngs(coordinates, fitOptions ?? null),
    );
  }

  cameraForGeometry(geometry, fitOptions) {
    return translateNativeErrors(() =>
      liveNativeOf(this).cameraForGeometry(
        stringifyJson(geometry),
        fitOptions ?? null,
      ),
    );
  }

  latLngBoundsForCamera(camera) {
    return translateNativeErrors(() =>
      liveNativeOf(this).latLngBoundsForCamera(camera),
    );
  }

  latLngBoundsForCameraUnwrapped(camera) {
    return translateNativeErrors(() =>
      liveNativeOf(this).latLngBoundsForCameraUnwrapped(camera),
    );
  }

  pixelForLatLng(coordinate) {
    return translateNativeErrors(() =>
      liveNativeOf(this).pixelForLatLng(coordinate),
    );
  }

  latLngForPixel(point) {
    return translateNativeErrors(() =>
      liveNativeOf(this).latLngForPixel(point),
    );
  }

  pixelsForLatLngs(coordinates) {
    return translateNativeErrors(() =>
      liveNativeOf(this).pixelsForLatLngs(coordinates),
    );
  }

  latLngsForPixels(points) {
    return translateNativeErrors(() =>
      liveNativeOf(this).latLngsForPixels(points),
    );
  }

  get renderingStatsViewEnabled() {
    return translateNativeErrors(
      () => liveNativeOf(this).renderingStatsViewEnabled,
    );
  }

  set renderingStatsViewEnabled(enabled) {
    translateNativeErrors(() => {
      liveNativeOf(this).renderingStatsViewEnabled = enabled;
    });
  }

  addStyleSourceJson(sourceId, source) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addStyleSourceJson(sourceId, stringifyJson(source)),
    );
  }

  styleSourceExists(sourceId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).styleSourceExists(sourceId),
    );
  }

  removeStyleSource(sourceId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).removeStyleSource(sourceId),
    );
  }

  listStyleSourceIds() {
    return translateNativeErrors(() => liveNativeOf(this).listStyleSourceIds());
  }

  getStyleSourceType(sourceId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).getStyleSourceType(sourceId),
    );
  }

  getStyleSourceInfo(sourceId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).getStyleSourceInfo(sourceId),
    );
  }

  addGeoJsonSourceUrl(sourceId, url) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addGeoJsonSourceUrl(sourceId, url),
    );
  }

  addGeoJsonSourceData(sourceId, data) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addGeoJsonSourceData(sourceId, stringifyJson(data)),
    );
  }

  setGeoJsonSourceUrl(sourceId, url) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setGeoJsonSourceUrl(sourceId, url),
    );
  }

  setGeoJsonSourceData(sourceId, data) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setGeoJsonSourceData(sourceId, stringifyJson(data)),
    );
  }

  addVectorSourceUrl(sourceId, url, options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addVectorSourceUrl(sourceId, url, options ?? null),
    );
  }

  addRasterSourceUrl(sourceId, url, options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addRasterSourceUrl(sourceId, url, options ?? null),
    );
  }

  addRasterDemSourceUrl(sourceId, url, options) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addRasterDemSourceUrl(sourceId, url, options ?? null),
    );
  }

  addVectorSourceTiles(sourceId, tiles, options) {
    if (!Array.isArray(tiles)) {
      throw new InvalidArgumentError(
        null,
        "vector source tiles must be an array",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).addVectorSourceTiles(
        sourceId,
        Array.from(tiles),
        options ?? null,
      ),
    );
  }

  addRasterSourceTiles(sourceId, tiles, options) {
    if (!Array.isArray(tiles)) {
      throw new InvalidArgumentError(
        null,
        "raster source tiles must be an array",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).addRasterSourceTiles(
        sourceId,
        Array.from(tiles),
        options ?? null,
      ),
    );
  }

  addRasterDemSourceTiles(sourceId, tiles, options) {
    if (!Array.isArray(tiles)) {
      throw new InvalidArgumentError(
        null,
        "raster DEM source tiles must be an array",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).addRasterDemSourceTiles(
        sourceId,
        Array.from(tiles),
        options ?? null,
      ),
    );
  }

  addCustomGeometrySource(sourceId, options = null) {
    const { fetchTile, cancelTile, ...nativeOptions } = options ?? {};
    if (typeof fetchTile !== "function") {
      throw new InvalidArgumentError(
        null,
        "custom geometry fetchTile callback must be a function",
      );
    }
    if (cancelTile != null && typeof cancelTile !== "function") {
      throw new InvalidArgumentError(
        null,
        "custom geometry cancelTile callback must be a function",
      );
    }
    return translateNativeErrors(() =>
      liveNativeOf(this).addCustomGeometrySource(
        sourceId,
        nativeOptions,
        customGeometryCallback(fetchTile),
        customGeometryCallback(cancelTile),
      ),
    );
  }

  _releaseDetachedCustomGeometrySources() {
    if (!this.closed) {
      translateNativeErrors(() =>
        liveNativeOf(this).releaseDetachedCustomGeometrySources(),
      );
    }
  }

  _customGeometrySourceCountForTesting() {
    return translateNativeErrors(() =>
      liveNativeOf(this).customGeometrySourceCountForTesting(),
    );
  }

  setCustomGeometrySourceTileData(sourceId, tileId, data) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setCustomGeometrySourceTileData(
        sourceId,
        tileId,
        stringifyJson(data),
      ),
    );
  }

  invalidateCustomGeometrySourceTile(sourceId, tileId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).invalidateCustomGeometrySourceTile(sourceId, tileId),
    );
  }

  invalidateCustomGeometrySourceRegion(sourceId, bounds) {
    return translateNativeErrors(() =>
      liveNativeOf(this).invalidateCustomGeometrySourceRegion(sourceId, bounds),
    );
  }

  setStyleImage(imageId, image) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setStyleImage(imageId, image),
    );
  }

  styleImageExists(imageId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).styleImageExists(imageId),
    );
  }

  removeStyleImage(imageId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).removeStyleImage(imageId),
    );
  }

  getStyleImageInfo(imageId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).getStyleImageInfo(imageId),
    );
  }

  copyStyleImagePremultipliedRgba8(imageId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).copyStyleImagePremultipliedRgba8(imageId),
    );
  }

  addImageSourceUrl(sourceId, coordinates, url) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addImageSourceUrl(sourceId, coordinates, url),
    );
  }

  addImageSourceImage(sourceId, coordinates, image) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addImageSourceImage(sourceId, coordinates, image),
    );
  }

  setImageSourceUrl(sourceId, url) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setImageSourceUrl(sourceId, url),
    );
  }

  setImageSourceImage(sourceId, image) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setImageSourceImage(sourceId, image),
    );
  }

  setImageSourceCoordinates(sourceId, coordinates) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setImageSourceCoordinates(sourceId, coordinates),
    );
  }

  getImageSourceCoordinates(sourceId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).getImageSourceCoordinates(sourceId),
    );
  }

  addHillshadeLayer(layerId, sourceId, beforeLayerId = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addHillshadeLayer(layerId, sourceId, beforeLayerId),
    );
  }

  addColorReliefLayer(layerId, sourceId, beforeLayerId = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addColorReliefLayer(layerId, sourceId, beforeLayerId),
    );
  }

  addLocationIndicatorLayer(layerId, beforeLayerId = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addLocationIndicatorLayer(layerId, beforeLayerId),
    );
  }

  setLocationIndicatorLocation(layerId, coordinate, altitude = 0) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLocationIndicatorLocation(
        layerId,
        coordinate,
        altitude,
      ),
    );
  }

  setLocationIndicatorBearing(layerId, bearing) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLocationIndicatorBearing(layerId, bearing),
    );
  }

  setLocationIndicatorAccuracyRadius(layerId, radius) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLocationIndicatorAccuracyRadius(layerId, radius),
    );
  }

  setLocationIndicatorImageName(layerId, imageKind, imageId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLocationIndicatorImageName(
        layerId,
        imageKind,
        imageId,
      ),
    );
  }

  addStyleLayerJson(layer, beforeLayerId = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).addStyleLayerJson(stringifyJson(layer), beforeLayerId),
    );
  }

  styleLayerExists(layerId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).styleLayerExists(layerId),
    );
  }

  removeStyleLayer(layerId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).removeStyleLayer(layerId),
    );
  }

  listStyleLayerIds() {
    return translateNativeErrors(() => liveNativeOf(this).listStyleLayerIds());
  }

  getStyleLayerType(layerId) {
    return translateNativeErrors(() =>
      liveNativeOf(this).getStyleLayerType(layerId),
    );
  }

  getStyleLayerJson(layerId) {
    const json = translateNativeErrors(() =>
      liveNativeOf(this).getStyleLayerJson(layerId),
    );
    return json === null ? null : JSON.parse(json);
  }

  moveStyleLayer(layerId, beforeLayerId = null) {
    return translateNativeErrors(() =>
      liveNativeOf(this).moveStyleLayer(layerId, beforeLayerId),
    );
  }

  setLayerProperty(layerId, propertyName, value) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLayerPropertyJson(
        layerId,
        propertyName,
        stringifyJson(value),
      ),
    );
  }

  getLayerProperty(layerId, propertyName) {
    const json = translateNativeErrors(() =>
      liveNativeOf(this).getLayerPropertyJson(layerId, propertyName),
    );
    return json === null ? null : JSON.parse(json);
  }

  setLayerFilter(layerId, filter) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setLayerFilterJson(
        layerId,
        filter === null ? null : stringifyJson(filter),
      ),
    );
  }

  getLayerFilter(layerId) {
    const json = translateNativeErrors(() =>
      liveNativeOf(this).getLayerFilterJson(layerId),
    );
    return json === null ? null : JSON.parse(json);
  }

  setStyleLight(light) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setStyleLightJson(stringifyJson(light)),
    );
  }

  setStyleLightProperty(propertyName, value) {
    return translateNativeErrors(() =>
      liveNativeOf(this).setStyleLightPropertyJson(
        propertyName,
        stringifyJson(value),
      ),
    );
  }

  getStyleLightProperty(propertyName) {
    const json = translateNativeErrors(() =>
      liveNativeOf(this).getStyleLightPropertyJson(propertyName),
    );
    return json === null ? null : JSON.parse(json);
  }

  setStyleJson(json) {
    return translateNativeErrors(() => liveNativeOf(this).setStyleJson(json));
  }

  setStyleUrl(url) {
    return translateNativeErrors(() => liveNativeOf(this).setStyleUrl(url));
  }

  [Symbol.dispose]() {
    this.close();
  }
}

function stringifyJson(value) {
  let json;
  try {
    json = JSON.stringify(value, (_key, item) => {
      if (typeof item === "number" && !Number.isFinite(item)) {
        throw new InvalidArgumentError(null, "JSON numbers must be finite");
      }
      return item;
    });
  } catch (error) {
    if (error instanceof InvalidArgumentError) {
      throw error;
    }
    throw new InvalidArgumentError(
      null,
      `JSON value is not serializable: ${error.message}`,
    );
  }
  if (json === undefined) {
    throw new InvalidArgumentError(
      null,
      "JSON value must be serializable as an object, array, string, number, boolean, or null",
    );
  }
  return json;
}

function assertNativeAbiVersion() {
  const actual = native.cVersion();
  if (actual !== EXPECTED_C_ABI_VERSION) {
    throw new MaplibreError(
      MaplibreStatus.abiVersionMismatch,
      null,
      `maplibre-native-c ABI version ${actual} does not match binding ABI version ${EXPECTED_C_ABI_VERSION}`,
    );
  }
}

function parseJson(value) {
  try {
    return JSON.parse(value);
  } catch (error) {
    throw new InvalidArgumentError(
      null,
      `JSON value from native is invalid: ${error.message}`,
    );
  }
}

function translateNativeErrors(callback) {
  try {
    return callback();
  } catch (error) {
    throw mapNativeError(error);
  }
}

function mapNativeError(error) {
  if (!(error instanceof Error)) {
    return error;
  }

  const payload = parseNativePayload(error.message);
  if (!payload) {
    return error;
  }

  const options = { cause: error };
  switch (payload.kind) {
    case "InvalidArgument":
      return new InvalidArgumentError(
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    case "InvalidState":
      return new InvalidStateError(
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    case "WrongThread":
      return new WrongThreadError(
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    case "Unsupported":
      return new UnsupportedFeatureError(
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    case "NativeError":
      return new NativeError(
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    case "AbiVersionMismatch":
      return new MaplibreError(
        MaplibreStatus.abiVersionMismatch,
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
    default:
      return new MaplibreError(
        MaplibreStatus.unknownStatus,
        payload.nativeStatusCode,
        payload.diagnostic,
        options,
      );
  }
}

function parseNativePayload(message) {
  if (typeof message !== "string" || !message.startsWith(NATIVE_ERROR_PREFIX)) {
    return null;
  }

  try {
    const payload = JSON.parse(message.slice(NATIVE_ERROR_PREFIX.length));
    if (
      typeof payload.kind !== "string" ||
      typeof payload.diagnostic !== "string"
    ) {
      return null;
    }
    return {
      kind: payload.kind,
      nativeStatusCode:
        typeof payload.nativeStatusCode === "number"
          ? payload.nativeStatusCode
          : null,
      diagnostic: payload.diagnostic,
    };
  } catch {
    return null;
  }
}

module.exports = {
  MaplibreError,
  InvalidArgumentError,
  InvalidStateError,
  WrongThreadError,
  UnsupportedFeatureError,
  NativeError,
  MaplibreStatus,
  RuntimeHandle,
  ResourceRequestHandle,
  OfflineOperationHandle,
  MapHandle,
  MapProjectionHandle,
  RenderSessionHandle,
  MetalOwnedTextureFrame,
  VulkanOwnedTextureFrame,
  OpenGLOwnedTextureFrame,
  NativePointer,
  NativeBuffer,
  cVersion,
  supportedRenderBackends,
  supportedOpenGLContextProviders,
  renderTargetExtentPhysicalSize,
  threadLastErrorMessage,
  takeNativeLeakReports,
  networkStatus,
  setNetworkStatus,
  projectedMetersForLatLng,
  latLngForProjectedMeters,
  setLogCallback,
  clearLogCallback,
  setAsyncLogSeverities,
  restoreDefaultAsyncLogSeverities,
};

module.exports.MaplibreError = MaplibreError;
module.exports.InvalidArgumentError = InvalidArgumentError;
module.exports.InvalidStateError = InvalidStateError;
module.exports.WrongThreadError = WrongThreadError;
module.exports.UnsupportedFeatureError = UnsupportedFeatureError;
module.exports.NativeError = NativeError;
module.exports.MaplibreStatus = MaplibreStatus;
module.exports.RuntimeHandle = RuntimeHandle;
module.exports.ResourceRequestHandle = ResourceRequestHandle;
module.exports.OfflineOperationHandle = OfflineOperationHandle;
module.exports.MapHandle = MapHandle;
module.exports.MapProjectionHandle = MapProjectionHandle;
module.exports.RenderSessionHandle = RenderSessionHandle;
module.exports.MetalOwnedTextureFrame = MetalOwnedTextureFrame;
module.exports.VulkanOwnedTextureFrame = VulkanOwnedTextureFrame;
module.exports.OpenGLOwnedTextureFrame = OpenGLOwnedTextureFrame;
module.exports.NativePointer = NativePointer;
module.exports.NativeBuffer = NativeBuffer;
module.exports.cVersion = cVersion;
module.exports.supportedRenderBackends = supportedRenderBackends;
module.exports.supportedOpenGLContextProviders =
  supportedOpenGLContextProviders;
module.exports.renderTargetExtentPhysicalSize = renderTargetExtentPhysicalSize;
module.exports.threadLastErrorMessage = threadLastErrorMessage;
module.exports.takeNativeLeakReports = takeNativeLeakReports;
module.exports.networkStatus = networkStatus;
module.exports.setNetworkStatus = setNetworkStatus;
module.exports.projectedMetersForLatLng = projectedMetersForLatLng;
module.exports.latLngForProjectedMeters = latLngForProjectedMeters;
module.exports.setLogCallback = setLogCallback;
module.exports.clearLogCallback = clearLogCallback;
module.exports.setAsyncLogSeverities = setAsyncLogSeverities;
module.exports.restoreDefaultAsyncLogSeverities =
  restoreDefaultAsyncLogSeverities;
