// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  AndroidHardwareBufferFrame,
  AndroidHardwareBufferRingDescriptor,
  AndroidSurfaceDescriptor,
  AndroidSurfaceReplacement,
  BoundOptions,
  CacheDatabaseOperation,
  CameraDelta,
  CameraFitOptions,
  CameraFitUpdate,
  CameraOptions,
  CameraQueryResult,
  CameraTransitionHandler,
  CameraUpdate,
  CanonicalTileId,
  CustomGeometrySourceOptions,
  CustomVectorSourceOptions,
  DmaBufFrame,
  DmaBufRingDescriptor,
  DxgiSharedTextureFrame,
  DxgiSharedTextureRingDescriptor,
  FeatureStateSelector,
  FrameDemand,
  FramePacerOptions,
  FreeCameraOptions,
  GeoJsonSourceOptions,
  GpuSync,
  HttpHeaderTransform,
  IoSurfaceFrame,
  IoSurfaceRingDescriptor,
  LatLng,
  LatLngBounds,
  LogHandler,
  LogSeverityFlag,
  LogicalExtent,
  LogicalSize,
  MapDebugFlag,
  MapOptions,
  MapProjectionCameraResult,
  MapSnapshot,
  MapTileOptions,
  MapView,
  MapViewportOptions,
  MetalBorrowedTextureDescriptor,
  MetalBorrowedTextureReplacement,
  MetalSurfaceDescriptor,
  MetalSurfaceReplacement,
  MetalTextureFrame,
  NetworkStatus,
  OfflineRegionDefinition,
  OfflineRegionDownloadState,
  OfflineRegionInfo,
  OfflineRegionStatus,
  OffscreenDescriptor,
  OffscreenDeviceOptions,
  OhosSurfaceDescriptor,
  OhosSurfaceReplacement,
  OpenGlBorrowedTextureDescriptor,
  OpenGlBorrowedTextureReplacement,
  OpenGlContextProviderFlag,
  OpenGlSurfaceDescriptor,
  OpenGlSurfaceReplacement,
  OpenGlTextureFrame,
  PremultipliedRgba8Image,
  ProjectedMeters,
  ProjectionModeOptions,
  RenderAbandonResult,
  RenderBackendFlag,
  RenderFrameResult,
  RenderSessionAttachOptions,
  RenderSessionCapabilities,
  RenderSessionSnapshot,
  RenderedFeature,
  RenderedFeatureQueryOptions,
  RenderedQueryGeometry,
  ResourceProvider,
  ResourceRequestCancelHandler,
  ResourceResponse,
  ResourceTransform,
  ResourceTransformError,
  RuntimeEventFlag,
  RuntimeEventHandler,
  RuntimeOptions,
  ScreenPoint,
  SourceFeature,
  SourceFeatureQueryOptions,
  StyleExpressionInfo,
  StyleExpressionInputs,
  StyleExpressionOptions,
  StyleImageInfo,
  StyleImageOptions,
  StyleLayerInfo,
  StyleSourceInfo,
  StyleTransitionOptions,
  StyleVectorTileEncoding,
  TextureReadbackResult,
  VulkanBorrowedTextureDescriptor,
  VulkanBorrowedTextureReplacement,
  VulkanDeviceContextResult,
  VulkanDeviceOptions,
  VulkanSurfaceDescriptor,
  VulkanSurfaceReplacement,
  VulkanTextureFrame,
  Wake,
  WaylandSurfaceDescriptor,
  WaylandSurfaceReplacement,
  WebGpuBorrowedTextureDescriptor,
  WebGpuBorrowedTextureReplacement,
  WebGpuSurfaceDescriptor,
  WebGpuSurfaceReplacement,
  WebGpuTextureFrame,
  Win32WindowDescriptor,
  Win32WindowReplacement,
} from "./generated_values.ts";
import { native } from "./internal/addon.ts";
import {
  type CommandCompletion,
  attach,
  command,
  query,
  run,
  submitRun,
  threadStart,
} from "./internal/completions.ts";
import { type NativeRegistration } from "./internal/native_registrations.ts";
import {
  ADOPT,
  BORROW,
  type AdoptKey,
  Owner,
  ViewScope,
  adopt,
  answerSync,
  checkKey,
  closeAsync,
  closeSync,
  defineDisposers,
  rootsOf,
} from "./internal/owners.ts";
import {
  type Registrations,
  defineCallbacks,
  withRegistrations,
} from "./internal/realm.ts";

/**
 * Creates a frame pacer that follows the display through the calling thread's
 * `AChoreographer`.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread, and
 * keep that thread running its looper while sessions use the pacer.
 *
 * `AChoreographer` can stop while the phone's display is off, even while a
 * Surface on another display, such as an Android Auto projection, still shows
 * the map. The pacer then keeps demanding frames from a timer on the looper,
 * within the frame rate limit, until vsyncs arrive again.
 *
 * See `mln_android_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.invalidState` when the calling thread has no `ALooper`.
 */
export function androidFramePacerCreate(
  options?: FramePacerOptions,
): FramePacerHandle {
  const handle = native.mln_android_frame_pacer_create(options);
  return adopt(
    handle,
    () => new FramePacerHandle(ADOPT, handle),
    (disposed) => native.mln_frame_pacer_release(disposed, undefined),
  );
}

/**
 * Creates a frame pacer that follows the display through a `CADisplayLink`.
 *
 * You can call it on any thread. The pacer needs no run loop of yours. On
 * macOS, it follows the main display, the one with the menu bar.
 *
 * With `FramePacerOptions.maximumFramesPerSecond`, the pacer also asks the
 * system for that refresh rate. On iPhone, a rate above 60 needs
 * `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.
 *
 * See `mln_apple_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * Errors:
 *
 * - `Status.nativeError` when the system has no display to follow.
 */
export function appleFramePacerCreate(
  options?: FramePacerOptions,
): FramePacerHandle {
  const handle = native.mln_apple_frame_pacer_create(options);
  return adopt(
    handle,
    () => new FramePacerHandle(ADOPT, handle),
    (disposed) => native.mln_frame_pacer_release(disposed, undefined),
  );
}

/**
 * Returns the API epoch of the library.
 *
 * Headers and a library with different epochs aren't compatible.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function cVersion(): number {
  return native.mln_c_version();
}

/**
 * Computes the camera that shows exactly one tile on a square map.
 *
 * Jump a map in `MapMode.tile` to the camera to render `tile`, where
 * `logicalSize` is the map's size. The camera centers the tile, zooms until the
 * tile fills the map, and looks straight down with north up and no padding. The
 * map keeps its camera within its zoom limits, so for a deep tile, raise the
 * maximum zoom past the camera's with `MapHandle.setBounds()`.
 *
 * See `mln_camera_for_tile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Errors:
 *
 * - `Status.invalidArgument` when `logicalSize` isn't square, or is wider than
 *   512 pixels. For denser tiles, raise the map's scale factor instead.
 * - `Status.invalidArgument` when `tile.x` or `tile.y` isn't less than 2 to the
 *   power `tile.z`, or when the tile can't fill the map at a zoom from 0 to
 *   25.5, as for tile 0/0/0 on a 256-pixel map.
 */
export function cameraForTile(
  tile: CanonicalTileId,
  logicalSize: LogicalSize,
): CameraOptions {
  return native.mln_camera_for_tile(tile, logicalSize);
}

/**
 * Sends log messages to the platform's log again, instead of a callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function clearLogCallback(): void {
  native.mln_log_clear_callback();
}

/**
 * Reads whether the library may use the network.
 *
 * See `mln_network_get_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function getNetworkStatus(): NetworkStatus {
  return native.mln_network_get_status();
}

/**
 * Returns the function that registers a plugin's style layer types.
 *
 * Pass the function to the plugin's registration entry point before loading a
 * style that uses the plugin's layer types. Plugin callbacks run on library
 * threads and can keep running while the process exits, so they must not use
 * state that the process destroys at exit.
 *
 * See `mln_plugin_get_register_function_v1` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
 */
export function getPluginRegisterFunctionV1(): bigint {
  return native.mln_plugin_get_register_function_v1();
}

/**
 * Gives the library the Java VM and the app context that it needs on Android.
 *
 * Call it before creating a runtime. `jniEnv` is the calling thread's
 * `JNIEnv*`, and `context` is an Android `Context`, such as the application.
 *
 * See `mln_android_init` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 */
export function initAndroid(jniEnv: bigint, context: bigint): void {
  native.mln_android_init(jniEnv, context);
}

/**
 * Converts meters in the Web Mercator projection to a coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
  return native.mln_lat_lng_for_projected_meters(meters);
}

/**
 * Computes the size in pixels of a target with a logical size and scale factor.
 *
 * See `mln_logical_extent_physical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function logicalExtentPhysicalSize(extent: LogicalExtent): {
  readonly width: number;
  readonly height: number;
} {
  return native.mln_logical_extent_physical_size(extent);
}

/**
 * Creates a frame pacer that follows the display through OpenHarmony's
 * `OH_NativeVSync`.
 *
 * You can call it on any thread. The pacer needs no event loop of yours.
 *
 * See `mln_ohos_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
 *
 * Errors:
 *
 * - `Status.nativeError` when the system can't report the display's refreshes
 *   to the process.
 */
export function ohosFramePacerCreate(
  options?: FramePacerOptions,
): FramePacerHandle {
  const handle = native.mln_ohos_frame_pacer_create(options);
  return adopt(
    handle,
    () => new FramePacerHandle(ADOPT, handle),
    (disposed) => native.mln_frame_pacer_release(disposed, undefined),
  );
}

/**
 * Reports the OpenGL context platforms that this build of the library has.
 *
 * See `mln_open_gl_supported_context_provider_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function openGlSupportedContextProviderMask(): OpenGlContextProviderFlag {
  return native.mln_open_gl_supported_context_provider_mask();
}

/**
 * Converts a coordinate to meters in the Web Mercator projection.
 *
 * See `mln_projected_meters_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
  return native.mln_projected_meters_for_lat_lng(coordinate);
}

/**
 * Selects the severities that the library logs on a background thread.
 *
 * The library logs other severities on the thread that logged them. `mask`
 * applies to every runtime in the process.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function setLogAsyncSeverityMask(mask: LogSeverityFlag): void {
  native.mln_log_set_async_severity_mask(mask);
}

/**
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 *
 * The callback runs asynchronously on the JavaScript thread that registered it,
 * after native queues the call.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function setLogCallback(handler: LogHandler): void {
  return withRegistrations((registrations) => {
    native.mln_log_set_callback(handler);
    registrations.commit();
  });
}

/**
 * Tells the library whether it may use the network.
 *
 * While the status is `NetworkStatus.offline`, the library starts no network
 * requests. The requests that wait start when the status returns to
 * `NetworkStatus.online`. The status starts as online and applies to every
 * runtime in the process.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function setNetworkStatus(status: NetworkStatus): void {
  native.mln_network_set_status(status);
}

/**
 * Returns the render backends that this build of the library draws with.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function supportedRenderBackendMask(): RenderBackendFlag {
  return native.mln_supported_render_backend_mask();
}

/**
 * A rendered frame that the caller holds to read or show.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class AcquiredFrameHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: RenderSessionHandle,
    registrations?: Registrations,
  ) {
    super(key, handle, "AcquiredFrameHandle", { parent, registrations });
  }

  /**
   * Gives up an acquired frame without saying when your GPU stops reading it.
   *
   * Because your GPU may still read the frame's texture, the session never
   * renders into that texture again.
   *
   * See `mln_acquired_frame_discard` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  discard(): void {
    closeSync(this, (handle, token) =>
      native.mln_acquired_frame_discard(handle, token),
    );
  }

  /**
   * Reads the frame result of an acquired frame.
   *
   * It succeeds until you release or discard the frame, whatever the session's
   * state.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getResult(): RenderFrameResult {
    return native.mln_acquired_frame_get_result(this.id);
  }

  /**
   * Gives an acquired frame back to the session once you're done reading it.
   *
   * `consumerCompletion` says when your GPU work that reads the texture
   * finishes. Pass `GpuSyncKind.cpuComplete` when that work has already
   * finished. The session waits for that point before it renders into the
   * texture again. If that wait fails, such as for an OpenGL fence outside the
   * session's share group, the session fails.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `RenderSessionCapabilities.consumerSyncKinds`
   *   lacks the kind of `consumerCompletion`.
   */
  release(consumerCompletion: GpuSync): void {
    closeSync(this, (handle, token) =>
      native.mln_acquired_frame_release(handle, token, consumerCompletion),
    );
  }

  /**
   * Gets the Android hardware buffer of an acquired frame.
   *
   * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the
   * buffer. Once you release or discard the frame, the session may render into
   * the buffer again. A reference that you take with `AHardwareBuffer_acquire`
   * keeps the buffer's memory, but not its contents.
   *
   * See `mln_acquired_frame_get_android_hardware_buffer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withAndroidHardwareBuffer<T>(
    use: (view: ScopedAndroidHardwareBufferFrame) => T,
  ): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedAndroidHardwareBufferFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_android_hardware_buffer(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the dmabuf of an acquired frame.
   *
   * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the
   * buffer. Once you release or discard the frame, the session may render into
   * the buffer again. You can map a buffer with DRM_FORMAT_MOD_LINEAR, which
   * has one plane, with `mmap`, and bracket your reads with DMA_BUF_IOCTL_SYNC.
   *
   * The session owns the planes' file descriptors, so don't close them. To keep
   * the buffer after you release the frame, duplicate each file descriptor with
   * `dup`. A duplicate keeps the buffer's memory, but not its contents.
   *
   * See `mln_acquired_frame_get_dma_buf` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withDmaBuf<T>(use: (view: ScopedDmaBufFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedDmaBufFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_dma_buf(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the DXGI shared texture of an acquired frame.
   *
   * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the
   * texture. Once you release or discard the frame, the session may render into
   * the texture again. Open the handle with
   * `ID3D11Device1::OpenSharedResource1` or `ID3D12Device::OpenSharedHandle`. A
   * texture that you open keeps its memory, but not its contents.
   *
   * The session owns the handle, so don't close it. To keep the handle after
   * you release the frame, duplicate it with `DuplicateHandle`.
   *
   * See `mln_acquired_frame_get_dxgi_shared_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withDxgiSharedTexture<T>(use: (view: ScopedDxgiSharedTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedDxgiSharedTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_dxgi_shared_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the IOSurface of an acquired frame.
   *
   * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the
   * surface. Once you release or discard the frame, the session may render into
   * the surface again. For an API that takes a `CVPixelBuffer`, wrap the
   * surface with `CVPixelBufferCreateWithIOSurface`. A reference that you take
   * with `CFRetain`, or a pixel buffer that wraps the surface, keeps the
   * surface's memory, but not its contents.
   *
   * See `mln_acquired_frame_get_io_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withIoSurface<T>(use: (view: ScopedIoSurfaceFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedIoSurfaceFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_io_surface(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the Metal texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_metal_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withMetalTexture<T>(use: (view: ScopedMetalTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedMetalTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_metal_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the OpenGL texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_open_gl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withOpenGlTexture<T>(use: (view: ScopedOpenGlTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedOpenGlTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_open_gl_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Reads what to wait for before you read an acquired frame's texture.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withProducerSync<T>(use: (view: ScopedGpuSync) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedGpuSync(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_producer_sync(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the Vulkan image of an acquired frame.
   *
   * See `mln_acquired_frame_get_vulkan_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withVulkanTexture<T>(use: (view: ScopedVulkanTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedVulkanTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_vulkan_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the WebGPU texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_web_gpu_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed, or you detached, abandoned,
   *   or disposed it.
   */
  withWebGpuTexture<T>(use: (view: ScopedWebGpuTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedWebGpuTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_web_gpu_texture(this.id),
        ),
      ),
    );
  }
}
defineDisposers(AcquiredFrameHandle, undefined, undefined);

/**
 * Demands render sessions' frames at a display's refreshes.
 *
 * See `mln_frame_pacer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class FramePacerHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "FramePacerHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a frame pacer.
   *
   * Render sessions that use the pacer stay paced until you release them or set
   * another pacer.
   *
   * See `mln_frame_pacer_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_frame_pacer_release(handle, token),
    );
  }
}
defineDisposers(FramePacerHandle, "release", "release");

/**
 * A map, with its style and camera.
 *
 * See `mln_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class MapHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: RuntimeHandle | undefined,
    ready?: Promise<void>,
    registrations?: Registrations,
  ) {
    super(key, handle, "MapHandle", { parent, registrations });
    this.ready = ready ?? Promise.resolve();
  }

  /**
   * Settles once the map is ready, and rejects with a `MaplibreError` when it
   * fails to become ready. Asynchronous calls on the map before then wait for
   * it. A rejection that nothing awaits isn't reported as unhandled.
   */
  readonly ready: Promise<void>;

  /**
   * The native handle value, which runtime events report as their
   * source. It stays readable after release, so events drained later
   * still match this handle.
   */
  declare readonly id: bigint;

  /** The commands of this owner that discard their outcomes. */
  get discarding(): MapHandleDiscarding {
    return new MapHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Adds a source whose GeoJSON tiles come from callbacks.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.alreadyExists` when the style already has a source named
   *   `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addCustomGeometrySource(
    sourceId: string,
    options: CustomGeometrySourceOptions,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_add_custom_geometry_source(
          this.id,
          sourceId,
          options,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Adds a source whose vector tiles come from callbacks.
   *
   * See `mln_map_add_custom_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.alreadyExists` when the style already has a source named
   *   `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addCustomVectorSource(
    sourceId: string,
    options: CustomVectorSourceOptions,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_add_custom_vector_source(
          this.id,
          sourceId,
          options,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Adds a GeoJSON source with no data.
   *
   * Give it data with `MapHandle.setGeoJsonSourceData()` or
   * `MapHandle.setGeoJsonSourceUrl()`. Its options stay fixed for as long as
   * the style holds it.
   *
   * `attribution` is the source's
   * &#91;attribution&#93;(https://maplibre.org/maplibre-style-spec/sources/#attribution),
   * or empty for none.
   *
   * See `mln_map_add_geo_json_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when an option in `options` is out of range.
   * - `Status.alreadyExists` when the style already has a source named
   *   `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addGeoJsonSource(
    sourceId: string,
    options?: GeoJsonSourceOptions,
    attribution?: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_geo_json_source(
        this.id,
        sourceId,
        options,
        attribution,
        completion,
      ),
    );
  }

  /**
   * Adds an image source that shows `image`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.alreadyExists` when the style already has a source named
   *   `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_image_source_image(
        this.id,
        sourceId,
        coordinates,
        image,
        completion,
      ),
    );
  }

  /**
   * Adds a layer to the map's style.
   *
   * `layerJson` is a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   * The layer goes below the layer named `beforeLayerId`, or above every layer
   * when `beforeLayerId` is empty.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `beforeLayerId`, or
   *   no source with the ID that the layer uses.
   * - `Status.alreadyExists` when the style already has a layer with the new
   *   layer's ID.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addStyleLayerJson(
    layerJson: unknown,
    beforeLayerId?: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_style_layer_json(
        this.id,
        layerJson,
        beforeLayerId,
        completion,
      ),
    );
  }

  /**
   * Adds a source to the map's style.
   *
   * `sourceJson` is a
   * &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/) object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.alreadyExists` when the style already has a source named
   *   `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  addStyleSourceJson(
    sourceId: string,
    sourceJson: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_style_source_json(
        this.id,
        sourceId,
        sourceJson,
        completion,
      ),
    );
  }

  /**
   * Moves the camera relative to where it is, as a gesture does.
   *
   * `delta` can pan the map's content by an offset on the screen, scale the
   * zoom, and add to the bearing and pitch. An anchor keeps one screen point in
   * place while the scale, bearing, or pitch changes. An anchor needs one of
   * those changes, and combines with an offset only when the delta has no
   * animation. The function finishes when the map applies the delta, before an
   * animation ends.
   *
   * A delta that replaces an animated one starts from where that animation left
   * the camera, so the rest of the replaced change never happens. To reach an
   * exact camera, use `MapHandle.updateCamera()`.
   *
   * A delta in the end phase can carry momentum, which keeps the camera moving
   * from the gesture's release velocities and slows it on each rendered frame.
   * The gesture lasts until the momentum ends: the snapshot keeps
   * `gesture_in_progress` set, and camera events report
   * `CameraChangeSource.momentum`. With momentum, `animation` sets only
   * `transition_id` and the end handler, which runs when the gesture ends.
   *
   * A delta in the begin phase, `MapHandle.cancelCameraTransitions()`, or a
   * cancellation by that ID stops the momentum. A later command that sets a
   * field stops only the parts of the momentum that move that field. On a
   * tilted map, the pan moves the map's content the same distance on the screen
   * in every direction.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  applyCameraDelta(delta: CameraDelta): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_apply_camera_delta(this.id, delta, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Creates a session that renders the map into a ring of Android hardware
   * buffers that the library allocates.
   *
   * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its
   * buffer with `AcquiredFrameHandle.withAndroidHardwareBuffer()`.
   * `RenderSessionHandle.resize()` gives the ring new buffers of the new size.
   *
   * A Vulkan build renders with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses an EGL context on the
   * default display, and `device` must be null. Attach with
   * `RenderDriverKind.coreWorker`.
   *
   * To show the buffers through Android's UI, such as with
   * `Bitmap.wrapHardwareBuffer`, attach without `GpuSyncKindFlag.syncFd` in
   * `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes
   * before you acquire it. Keep each frame acquired until the next one replaces
   * it on screen, then release it with `GpuSyncKind.cpuComplete`. Give the ring
   * 3 buffers, so the session can render while you hold two frames.
   *
   * See `mln_map_attach_android_hardware_buffer_ring` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when Android is older than 8.0, API level 26, or
   *   when the graphics context can't render into hardware buffers: the Vulkan
   *   device lacks VK_ANDROID_external_memory_android_hardware_buffer, or the
   *   EGL display lacks EGL_ANDROID_get_native_client_buffer or
   *   EGL_ANDROID_image_native_buffer.
   * - `Status.nativeError` when the library can't create its Vulkan device or
   *   initialize the default EGL display.
   */
  attachAndroidHardwareBufferRing(
    descriptor: AndroidHardwareBufferRingDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_android_hardware_buffer_ring(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that presents the map to an Android window, with a
   * graphics context that the library creates.
   *
   * A Vulkan build presents with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses an EGL context on the
   * default display, and `device` must be null. Attach with
   * `RenderDriverKind.coreWorker`.
   *
   * When the platform destroys the window, such as in
   * `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
   * `RenderSessionHandle.releaseTarget()`, or give the session a new window
   * with `RenderSessionHandle.setAndroidSurfaceTarget()`, without waiting for
   * that to complete. The session keeps its own reference to the window and
   * stays usable for the next window. A frame to the destroyed window finishes
   * with `RenderResult.targetNotReady`, or with `RenderResult.rendered` if the
   * window went away while the frame was presenting.
   *
   * See `mln_map_attach_android_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `surface` has no window, such as after the
   *   platform destroyed it.
   * - `Status.unsupported` when you created `device` without
   *   `VulkanDeviceOptions.presentation`.
   * - `Status.unsupported` when the GPU or the Vulkan loader can't present.
   * - `Status.nativeError` when the library can't create the graphics context
   *   or the window's surface.
   */
  attachAndroidSurface(
    descriptor: AndroidSurfaceDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_android_surface(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of dmabufs that the
   * library allocates.
   *
   * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its
   * buffer with `AcquiredFrameHandle.withDmaBuf()`.
   * `RenderSessionHandle.resize()` gives the ring new buffers of the new size.
   *
   * A Vulkan build renders with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses an EGL context of its
   * own, and `device` must be null. Attach with `RenderDriverKind.coreWorker`.
   *
   * For a reader that waits on no sync file, such as a Wayland compositor
   * without explicit synchronization, attach without `GpuSyncKindFlag.syncFd`
   * in `RenderSessionAttachOptions.producerSyncKinds`, so that each frame
   * finishes before you acquire it. Keep each frame acquired until its reader
   * is done with the buffer. Give the ring 3 buffers, so the session can render
   * while you hold two frames.
   *
   * See `mln_map_attach_dma_buf_ring` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the graphics context can't render into a dmabuf
   *   of any of `formats`.
   * - `Status.nativeError` when the library can't create its Vulkan device or
   *   initialize an EGL display.
   */
  attachDmaBufRing(
    descriptor: DmaBufRingDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_dma_buf_ring(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of DXGI shared textures
   * that the library allocates.
   *
   * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its
   * texture with `AcquiredFrameHandle.withDxgiSharedTexture()`.
   * `RenderSessionHandle.resize()` gives the ring new textures of the new size.
   *
   * The session renders with the Vulkan device `device`, or, when `device` is
   * null, with one that the library creates. The textures belong to the
   * device's GPU, so open them on a D3D11 or D3D12 device of the same adapter.
   * To pick the GPU, create `device` with `VulkanDeviceOptions.deviceLuid` set
   * to your adapter's LUID. Attach with `RenderDriverKind.coreWorker`.
   *
   * For a reader that waits on no fence, such as Flutter's texture registrar,
   * attach without `GpuSyncKindFlag.d3d12Fence` in
   * `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes
   * before you acquire it. Release a frame with `GpuSyncKind.cpuComplete` only
   * once the reader has finished with its texture.
   *
   * See `mln_map_attach_dxgi_shared_texture_ring` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` in an OpenGL build, or when the GPU can't share the
   *   textures: D3D11 has no adapter with the Vulkan device's LUID or can't
   *   share textures through NT handles, or the device lacks
   *   VK_KHR_external_memory_win32 or can't import D3D11 textures.
   * - `Status.nativeError` when the library can't create its Vulkan device or a
   *   D3D11 device on the adapter.
   */
  attachDxgiSharedTextureRing(
    descriptor: DxgiSharedTextureRingDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_dxgi_shared_texture_ring(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of IOSurfaces that the
   * library allocates.
   *
   * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its
   * surface with `AcquiredFrameHandle.withIoSurface()`.
   * `RenderSessionHandle.resize()` gives the ring new surfaces of the new size.
   * Attach with `RenderDriverKind.coreWorker`.
   *
   * To hand frames to a reader that can't wait on an event, such as Flutter's
   * `copyPixelBuffer`, attach without `GpuSyncKindFlag.metalSharedEvent` in
   * `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes
   * before you acquire it. Release a frame with `GpuSyncKind.cpuComplete` only
   * once the reader has finished with its surface.
   *
   * See `mln_map_attach_io_surface_ring` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`.
   * - `Status.nativeError` when `context` names no device and the system has no
   *   Metal device.
   */
  attachIoSurfaceRing(
    descriptor: IoSurfaceRingDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_io_surface_ring(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of your Metal textures.
   *
   * See `mln_map_attach_metal_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_metal_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map to a Metal layer.
   *
   * See `mln_map_attach_metal_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`.
   */
  attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_metal_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Attaches a render session that renders the map offscreen.
   *
   * Read each image's pixels with `RenderSessionHandle.readTexture()`.
   *
   * The session renders on `device`. When `device` is null, the session creates
   * a device with the default options, and releases it when the session ends.
   *
   * See `mln_map_attach_offscreen` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the map already has a render session.
   * - `Status.nativeError` when `device` is null and the platform can't create
   *   a device.
   */
  attachOffscreen(
    descriptor: OffscreenDescriptor,
    options: RenderSessionAttachOptions,
    device?: OffscreenDeviceHandle,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_offscreen(
            this.id,
            descriptor,
            options,
            device?.id,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that presents the map to an OpenHarmony window, with a
   * graphics context that the library creates.
   *
   * A Vulkan build presents with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses an EGL context on the
   * default display, and `device` must be null. Attach with
   * `RenderDriverKind.coreWorker`.
   *
   * When the platform destroys the window, such as in an `XComponent`'s
   * `onSurfaceDestroyed` callback, release the target with
   * `RenderSessionHandle.releaseTarget()`, or give the session a new window
   * with `RenderSessionHandle.setOhosSurfaceTarget()`, without waiting for that
   * to complete. The session keeps its own reference to the window and stays
   * usable for the next window. A frame to the destroyed window finishes with
   * `RenderResult.targetNotReady`, or with `RenderResult.rendered` if the
   * window went away while the frame was presenting.
   *
   * See `mln_map_attach_ohos_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when no surface in this process has
   *   `OhosSurfaceDescriptor.surfaceId`, such as after the platform destroyed
   *   it.
   * - `Status.unsupported` when you created `device` without
   *   `VulkanDeviceOptions.presentation`.
   * - `Status.unsupported` when the GPU or the Vulkan loader can't present.
   * - `Status.nativeError` when the library can't create the graphics context
   *   or the window's surface.
   */
  attachOhosSurface(
    descriptor: OhosSurfaceDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_ohos_surface(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of your OpenGL textures.
   *
   * See `mln_map_attach_open_gl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
   *   the context's platform, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachOpenGlBorrowedTexture(
    descriptor: OpenGlBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_open_gl_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map to an OpenGL surface.
   *
   * See `mln_map_attach_open_gl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
   *   the context's platform.
   */
  attachOpenGlSurface(
    descriptor: OpenGlSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_open_gl_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of your Vulkan images.
   *
   * See `mln_map_attach_vulkan_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_vulkan_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map to a Vulkan surface.
   *
   * See `mln_map_attach_vulkan_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`.
   */
  attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_vulkan_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that presents the map to a Wayland surface, with a
   * graphics context that the library creates.
   *
   * A Vulkan build presents with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses an EGL context on the
   * display, and `device` must be null. Attach with
   * `RenderDriverKind.coreWorker`.
   *
   * The session presents without waiting for the compositor, so demand at most
   * one frame per display refresh.
   *
   * Keep the surface until a release or replacement of the target, or a detach
   * of the session, completes. Keep the display connected until the session's
   * detach completes. In an OpenGL build, if you also use EGL on the display,
   * initialize it before you attach.
   *
   * See `mln_map_attach_wayland_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when you created `device` without
   *   `VulkanDeviceOptions.presentation`.
   * - `Status.unsupported` when the GPU or the Vulkan loader can't present to
   *   Wayland.
   * - `Status.nativeError` when the library can't create the graphics context
   *   or present to the surface.
   */
  attachWaylandSurface(
    descriptor: WaylandSurfaceDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_wayland_surface(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map into a ring of your WebGPU textures.
   *
   * See `mln_map_attach_web_gpu_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.webGpu`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachWebGpuBorrowedTexture(
    descriptor: WebGpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_web_gpu_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that renders the map to a WebGPU surface.
   *
   * See `mln_map_attach_web_gpu_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.webGpu`.
   */
  attachWebGpuSurface(
    descriptor: WebGpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_web_gpu_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Creates a session that presents the map to a Win32 window, with a graphics
   * context that the library creates.
   *
   * A Vulkan build presents with `device`, or, when `device` is null, with a
   * device that the library creates. An OpenGL build uses a WGL context, and
   * `device` must be null. Attach with `RenderDriverKind.coreWorker`.
   *
   * An OpenGL build sets the window's pixel format, so don't set one yourself.
   *
   * The session presents without waiting for the display, so demand at most one
   * frame per display refresh. While the window is minimized, frame demands
   * finish with `RenderResult.targetNotReady`.
   *
   * Keep the window until a release or replacement of the target, or a detach
   * of the session, completes.
   *
   * See `mln_map_attach_win32_window` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `window` isn't a window, or, in an OpenGL
   *   build, already has another pixel format.
   * - `Status.invalidState` when `window` is minimized or has no client area.
   * - `Status.unsupported` when you created `device` without
   *   `VulkanDeviceOptions.presentation`.
   * - `Status.invalidState` when `window` was minimized before the attach ran.
   * - `Status.unsupported` when the GPU or the Vulkan loader can't present.
   * - `Status.nativeError` when the library can't create the graphics context
   *   or present to the window.
   */
  attachWin32Window(
    descriptor: Win32WindowDescriptor,
    device: VulkanDeviceHandle | undefined,
    options: RenderSessionAttachOptions,
  ): RenderSessionHandle {
    return withRegistrations((registrations) =>
      attach(
        (completion) =>
          native.mln_map_attach_win32_window(
            this.id,
            descriptor,
            device?.id,
            options,
            completion,
          ),
        (handle, ready) =>
          adopt(
            handle,
            () =>
              new RenderSessionHandle(
                ADOPT,
                handle,
                this,
                ready,
                registrations,
              ),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined),
          ),
      ),
    );
  }

  /**
   * Holds back the map's changes from rendering until
   * `MapHandle.endCommandGroup()`, so they show in the same frame.
   *
   * Groups nest, and the changes render when the outermost group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  beginCommandGroup(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_begin_command_group(this.id, completion),
    );
  }

  /**
   * Computes a camera that fits a GeoJSON geometry in the map's view.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
   * feature. The camera keeps the map's bearing and pitch unless `fitOptions`
   * sets them, and its zoom stays within the map's zoom limits.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
   *   for the area.
   */
  cameraForGeometry(
    geometry: unknown,
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_geometry(
        this.id,
        geometry,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Computes a camera that fits `bounds` in the map's view.
   *
   * The camera keeps the map's bearing and pitch unless `fitOptions` sets them,
   * and its zoom stays within the map's zoom limits.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
   *   for the area.
   */
  cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_lat_lng_bounds(
        this.id,
        bounds,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Computes a camera that fits `coordinates` in the map's view.
   *
   * `coordinates` must not be empty. The camera keeps the map's bearing and
   * pitch unless `fitOptions` sets them, and its zoom stays within the map's
   * zoom limits.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `fitOptions` leaves no room
   *   for the area.
   */
  cameraForLatLngs(
    coordinates: readonly LatLng[],
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_lat_lngs(
        this.id,
        coordinates,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Stops every running camera animation where it is.
   *
   * The end handler of each stopped animation reports
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransitions(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_cancel_camera_transitions(this.id, completion),
    );
  }

  /**
   * Stops the running camera animations whose animation options set
   * `transitionId`.
   *
   * Other animations keep running. The end handler of each stopped animation
   * reports `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transitions_with_id` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransitionsWithId(
    transitionId: bigint,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_cancel_camera_transitions_with_id(
        this.id,
        transitionId,
        completion,
      ),
    );
  }

  /**
   * Releases the map in any state, and finishes in the background.
   *
   * Use it from destructors and finalizers. Otherwise, use
   * `MapHandle.release()`.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) => native.mln_map_dispose(handle, token));
  }

  /**
   * Logs debugging information about the map's style and sources.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  dumpDebugLogs(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_dump_debug_logs(this.id, completion),
    );
  }

  /**
   * Ends the group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  endCommandGroup(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_end_command_group(this.id, completion),
    );
  }

  /**
   * Moves the camera to fit a GeoJSON geometry in the map's view, at once or
   * with an animation.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
   * feature. The camera is the one that `MapHandle.cameraForGeometry()`
   * computes after the commands sent before this one apply. The function
   * finishes when the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `update` leaves no room for
   *   the area. The camera doesn't move.
   */
  fitCameraToGeometry(
    geometry: unknown,
    update: CameraFitUpdate,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_fit_camera_to_geometry(
          this.id,
          geometry,
          update,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Moves the camera to fit `bounds` in the map's view, at once or with an
   * animation.
   *
   * The camera is the one that `MapHandle.cameraForLatLngBounds()` computes
   * after the commands sent before this one apply. The function finishes when
   * the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `update` leaves no room for
   *   the area. The camera doesn't move.
   */
  fitCameraToLatLngBounds(
    bounds: LatLngBounds,
    update: CameraFitUpdate,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_fit_camera_to_lat_lng_bounds(
          this.id,
          bounds,
          update,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Moves the camera to fit `coordinates` in the map's view, at once or with an
   * animation.
   *
   * `coordinates` must not be empty. The camera is the one that
   * `MapHandle.cameraForLatLngs()` computes after the commands sent before this
   * one apply. The function finishes when the map applies the change, before an
   * animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the padding in `update` leaves no room for
   *   the area. The camera doesn't move.
   */
  fitCameraToLatLngs(
    coordinates: readonly LatLng[],
    update: CameraFitUpdate,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_fit_camera_to_lat_lngs(
          this.id,
          coordinates,
          update,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Reads the map's camera.
   *
   * The query reads the camera after the map applies the commands sent before
   * it.
   *
   * See `mln_map_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  getCamera(): Promise<CameraQueryResult> {
    return query<CameraQueryResult>((completion) =>
      native.mln_map_get_camera(this.id, completion),
    );
  }

  /**
   * Reads the state of a feature, as a JSON object.
   *
   * The object is empty when the feature has no state, including after its
   * source leaves the style or a style loads, as `MapHandle.setFeatureState()`
   * describes.
   *
   * See `mln_map_get_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getFeatureState(selector: FeatureStateSelector): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_feature_state(this.id, selector, completion),
    );
  }

  /**
   * Reads the global state of the map's style as a JSON object.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getGlobalState(): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_global_state(this.id, completion),
    );
  }

  /**
   * Reads the corners of an image source's image, clockwise from the top left.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   */
  getImageSourceCoordinates(sourceId: string): Promise<readonly LatLng[]> {
    return query<readonly LatLng[]>((completion) =>
      native.mln_map_get_image_source_coordinates(
        this.id,
        sourceId,
        completion,
      ),
    );
  }

  /**
   * Reads the JSON of the style that the map loaded.
   *
   * The JSON doesn't include the changes made to the style since it loaded.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getLoadedStyleJson(): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_loaded_style_json(this.id, completion),
    );
  }

  /**
   * Copies the map's state.
   *
   * The copy doesn't wait for pending commands. To read the camera after they
   * apply, use `MapHandle.getCamera()`.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` until `RuntimeHandle.createMap()` completes with
   *   `Status.ok`.
   */
  getSnapshot(): MapSnapshot {
    return native.mln_map_get_snapshot(this.id);
  }

  /**
   * Reads an image of the map's style.
   *
   * Completes with no value when the style has no image named `imageId`.
   *
   * See `mln_map_get_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleImage(imageId: string): Promise<StyleImageInfo | undefined> {
    return query<StyleImageInfo | undefined>((completion) =>
      native.mln_map_get_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Reads a layer of the map's style.
   *
   * Completes with no value when the style has no layer named `layerId`.
   *
   * See `mln_map_get_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayer(layerId: string): Promise<StyleLayerInfo | undefined> {
    return query<StyleLayerInfo | undefined>((completion) =>
      native.mln_map_get_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Reads a layer of the map's style as a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   *
   * Completes with no value when the style has no layer named `layerId`.
   *
   * See `mln_map_get_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayerJson(layerId: string): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_layer_json(this.id, layerId, completion),
    );
  }

  /**
   * Reads a property of a layer in the map's style.
   *
   * The value can differ in form from the value that was set: a color set as
   * `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.
   *
   * Completes with no value when the layer doesn't set the property.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   */
  getStyleLayerProperty(
    layerId: string,
    propertyName: string,
  ): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_layer_property(
        this.id,
        layerId,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Reads a property of the light of the map's style.
   *
   * The value can differ in form from the value that was set: a color set as
   * `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.
   *
   * Completes with no value when the light doesn't set the property.
   *
   * See `mln_map_get_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLightProperty(propertyName: string): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_light_property(
        this.id,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Reads a source of the map's style.
   *
   * A tile source with a `url` has an empty attribution until its TileJSON
   * loads, which raises `RuntimeEventType.mapSourceChanged`.
   *
   * Completes with no value when the style has no source named `sourceId`.
   *
   * See `mln_map_get_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleSource(sourceId: string): Promise<StyleSourceInfo | undefined> {
    return query<StyleSourceInfo | undefined>((completion) =>
      native.mln_map_get_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Reads the default transition of the map's style.
   *
   * See `mln_map_get_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleTransitionOptions(): Promise<StyleTransitionOptions> {
    return query<StyleTransitionOptions>((completion) =>
      native.mln_map_get_style_transition_options(this.id, completion),
    );
  }

  /**
   * Reads the URL of the map's style.
   *
   * The URL is empty when the style came from `MapHandle.setStyleJson()`.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getStyleUrl(): Promise<string> {
    return query<string>((completion) =>
      native.mln_map_get_style_url(this.id, completion),
    );
  }

  /**
   * Makes a custom geometry source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  invalidateCustomGeometrySourceRegion(
    sourceId: string,
    bounds: LatLngBounds,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_geometry_source_region(
        this.id,
        sourceId,
        bounds,
        completion,
      ),
    );
  }

  /**
   * Makes a custom geometry source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  invalidateCustomGeometrySourceTile(
    sourceId: string,
    tileId: CanonicalTileId,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_geometry_source_tile(
        this.id,
        sourceId,
        tileId,
        completion,
      ),
    );
  }

  /**
   * Makes a custom vector source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_vector_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  invalidateCustomVectorSourceRegion(
    sourceId: string,
    bounds: LatLngBounds,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_vector_source_region(
        this.id,
        sourceId,
        bounds,
        completion,
      ),
    );
  }

  /**
   * Makes a custom vector source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  invalidateCustomVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_vector_source_tile(
        this.id,
        sourceId,
        tileId,
        completion,
      ),
    );
  }

  /**
   * Computes the area that the map would show with `camera`.
   *
   * Camera fields that `camera` doesn't set come from the map's camera. The
   * longitudes can fall outside -180 to 180, so a view across the antimeridian
   * gives one continuous range.
   *
   * See `mln_map_lat_lng_bounds_for_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  latLngBoundsForCamera(camera: CameraOptions): Promise<LatLngBounds> {
    return query<LatLngBounds>((completion) =>
      native.mln_map_lat_lng_bounds_for_camera(this.id, camera, completion),
    );
  }

  /**
   * Lists the layers of the map's style, from bottom to top.
   *
   * See `mln_map_list_style_layers` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleLayers(): Promise<readonly StyleLayerInfo[]> {
    return query<readonly StyleLayerInfo[]>((completion) =>
      native.mln_map_list_style_layers(this.id, completion),
    );
  }

  /**
   * Lists the sources of the map's style.
   *
   * See `mln_map_list_style_sources` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleSources(): Promise<readonly StyleSourceInfo[]> {
    return query<readonly StyleSourceInfo[]>((completion) =>
      native.mln_map_list_style_sources(this.id, completion),
    );
  }

  /**
   * Moves a layer of the map's style below the layer named `beforeLayerId`, or
   * above every layer when `beforeLayerId` is empty.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId` or
   *   `beforeLayerId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  moveStyleLayer(
    layerId: string,
    beforeLayerId?: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_move_style_layer(
        this.id,
        layerId,
        beforeLayerId,
        completion,
      ),
    );
  }

  /**
   * Releases the map.
   *
   * Detach the map's render session first.
   *
   * See `mln_map_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  release(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_map_release(handle, token, completion),
      ),
    );
  }

  /**
   * Removes the feature state that `selector` selects.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  removeFeatureState(
    selector: FeatureStateSelector,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_feature_state(this.id, selector, completion),
    );
  }

  /**
   * Removes an image from the map's style.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no image named `imageId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  removeStyleImage(imageId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Removes a layer from the map's style.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  removeStyleLayer(layerId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Removes a source from the map's style.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidState` when a layer still uses the source, or while the
   *   style isn't loaded, as `MapHandle.setStyleUrl()` describes.
   */
  removeStyleSource(sourceId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Renders a new frame of a continuous map, even when nothing changed.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestRepaint(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_request_repaint(this.id, completion),
    );
  }

  /**
   * Renders a still image of a static or tile map.
   *
   * The map's render session draws the image once every resource that the image
   * needs has loaded.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.cancelled` when the map is released before the image is done.
   * - `Status.nativeError` when the style or a resource that the image needs
   *   fails to load.
   */
  requestStillImage(): Promise<void> {
    return run((completion) =>
      native.mln_map_request_still_image(this.id, completion),
    );
  }

  /**
   * Sets the map's size.
   *
   * A map with a render session takes its size from the session, so resize the
   * session with `RenderSessionHandle.resize()` instead.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the map has a render session.
   * - `Status.invalidState` when a render session attached to the map before
   *   the resize applied.
   */
  resize(logicalSize: LogicalSize): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_resize(this.id, logicalSize, completion),
    );
  }

  /**
   * Limits the camera's center to an area, and its zoom and pitch to ranges.
   *
   * Limits that `options` doesn't set keep their current values. When the
   * camera is outside the new limits, it moves inside them.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setBounds(options: BoundOptions): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_bounds(this.id, options, completion),
    );
  }

  /**
   * Sets the data of a tile in a custom geometry source.
   *
   * `data` is a GeoJSON feature collection, feature, or geometry. The source
   * ignores data for a tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_geometry_source_tile_data(
        this.id,
        sourceId,
        tileId,
        data,
        completion,
      ),
    );
  }

  /**
   * Reports that a tile of a custom geometry source failed to load.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  setCustomGeometrySourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_geometry_source_tile_error(
        this.id,
        sourceId,
        tileId,
        message,
        completion,
      ),
    );
  }

  /**
   * Sets the data of a tile in a custom vector source.
   *
   * `data` is the tile, encoded as `encoding`. The source ignores data for a
   * tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  setCustomVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: Uint8Array,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_vector_source_tile_data(
        this.id,
        sourceId,
        tileId,
        encoding,
        data,
        completion,
      ),
    );
  }

  /**
   * Reports that a tile of a custom vector source failed to load.
   *
   * See `mln_map_set_custom_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  setCustomVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_vector_source_tile_error(
        this.id,
        sourceId,
        tileId,
        message,
        completion,
      ),
    );
  }

  /**
   * Draws debugging overlays, such as tile borders, on the map.
   *
   * `flags` replaces the overlays set before.
   *
   * See `mln_map_set_debug_flags` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugFlags(flags: MapDebugFlag): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_debug_flags(this.id, flags, completion),
    );
  }

  /**
   * Selects the events that the map reports.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventFlag): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_event_mask(this.id, mask, completion),
    );
  }

  /**
   * Merges `state` into the state of a feature.
   *
   * `state` is a JSON object. Its keys replace the same keys in the feature's
   * state, and the other keys stay.
   *
   * The map keeps the state until it is removed, its source leaves the style,
   * or a style loads. `MapHandle.removeStyleSource()` removes the state of the
   * source's features, and a style that loads removes the state of every
   * source, so a later source with the same ID starts with no state. To set
   * state for a new style's sources, wait until `MapHandle.setStyleUrl()` or
   * `MapHandle.setStyleJson()` completes. The style load removes state set
   * before then.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(
    selector: FeatureStateSelector,
    state: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_feature_state(this.id, selector, state, completion),
    );
  }

  /**
   * Places the camera at a position and orientation in space.
   *
   * The map adjusts them to a camera that it can show. It stops running camera
   * animations first, and their end handlers report
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setFreeCameraOptions(options: FreeCameraOptions): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_free_camera_options(this.id, options, completion),
    );
  }

  /**
   * Replaces the data of a GeoJSON source.
   *
   * `data` is a GeoJSON feature collection, feature, or geometry. The library
   * prepares it for the source with the source's options, off the threads that
   * the caller and the runtime use, and the command completes once the source
   * holds it. Later commands don't wait for it. While the source is still
   * preparing earlier data, only the newest data waits to follow it.
   *
   * With synchronous tiling on, as
   * `MapHandle.setGeoJsonSourceSynchronousTiling()` describes, the command
   * prepares `data` before later commands run, so the data shows in the next
   * frame.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source, when `data` isn't valid GeoJSON, or when the source clusters and
   *   `data` isn't a feature collection of points. The source keeps its data.
   * - `Status.cancelled` when newer data for the source replaces `data` while
   *   it waits, or when the source's URL, its removal, a style load, or the
   *   map's release replaces the source's data before `data` is ready.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setGeoJsonSourceData(
    sourceId: string,
    data: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_data(
        this.id,
        sourceId,
        data,
        completion,
      ),
    );
  }

  /**
   * Sets whether a GeoJSON source builds its tiles while the map renders.
   *
   * Building tiles while the map renders, instead of in the background, makes
   * new data show in the next frame. The setting replaces the
   * `synchronousUpdate` member of the source's JSON, which defaults to false.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setGeoJsonSourceSynchronousTiling(
    sourceId: string,
    enabled: boolean,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_synchronous_tiling(
        this.id,
        sourceId,
        enabled,
        completion,
      ),
    );
  }

  /**
   * Makes a GeoJSON source load its data from `url`.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setGeoJsonSourceUrl(
    sourceId: string,
    url: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_url(
        this.id,
        sourceId,
        url,
        completion,
      ),
    );
  }

  /**
   * Sets a value that the style's `global-state`
   * &#91;expressions&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * read.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setGlobalStateProperty(
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_global_state_property(
        this.id,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Moves the image of an image source.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setImageSourceCoordinates(
    sourceId: string,
    coordinates: readonly LatLng[],
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_coordinates(
        this.id,
        sourceId,
        coordinates,
        completion,
      ),
    );
  }

  /**
   * Replaces the image of an image source.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setImageSourceImage(
    sourceId: string,
    image: PremultipliedRgba8Image,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_image(
        this.id,
        sourceId,
        image,
        completion,
      ),
    );
  }

  /**
   * Makes an image source load its image from `url`.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setImageSourceUrl(sourceId: string, url: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_url(this.id, sourceId, url, completion),
    );
  }

  /**
   * Sets whether the map draws in perspective or axonometric projection, and
   * how much it skews the view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setProjectionMode(
    options: ProjectionModeOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_projection_mode(this.id, options, completion),
    );
  }

  /**
   * Shows or hides an overlay of rendering statistics on the map.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setRenderingStatsViewEnabled(enabled: boolean): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_rendering_stats_view_enabled(
        this.id,
        enabled,
        completion,
      ),
    );
  }

  /**
   * Adds an image to the map's style, or replaces the image named `imageId`.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image,
    options?: StyleImageOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_image(
        this.id,
        imageId,
        image,
        options,
        completion,
      ),
    );
  }

  /**
   * Loads the map's style from `json`.
   *
   * The command completes once the style has parsed, and the map's style is
   * then loaded, as `MapHandle.setStyleUrl()` describes. The style's sources,
   * sprites, and glyphs load in the background.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `json` isn't a valid style. The map keeps
   *   its current style.
   */
  setStyleJson(json: unknown): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_json(this.id, json, completion),
    );
  }

  /**
   * Sets a property of a layer in the map's style.
   *
   * `propertyName` is one of the layer's layout or paint
   * &#91;properties&#93;(https://maplibre.org/maplibre-style-spec/layers/), or
   * `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value`
   * resets any of these but `source` and `source-layer` to its default.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_layer_property(
        this.id,
        layerId,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Replaces the
   * &#91;light&#93;(https://maplibre.org/maplibre-style-spec/light/) of the
   * map's style.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleLightJson(lightJson: unknown): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_light_json(this.id, lightJson, completion),
    );
  }

  /**
   * Sets a property of the light of the map's style.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleLightProperty(
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_light_property(
        this.id,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Sets whether a source is volatile.
   *
   * The map doesn't store the tiles of a volatile source in the persistent
   * cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleSourceVolatile(
    sourceId: string,
    isVolatile: boolean,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_source_volatile(
        this.id,
        sourceId,
        isVolatile,
        completion,
      ),
    );
  }

  /**
   * Sets the default transition of the map's style.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while the style isn't loaded, as
   *   `MapHandle.setStyleUrl()` describes.
   */
  setStyleTransitionOptions(
    options: StyleTransitionOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_transition_options(this.id, options, completion),
    );
  }

  /**
   * Loads the map's style from `url`.
   *
   * The command completes once the style document has loaded and parsed. The
   * style's sources, sprites, and glyphs then load in the background.
   *
   * The map's style is loaded from the time a style document parses until the
   * next call to this function. While the style isn't loaded, the commands that
   * change it complete with `Status.invalidState`. To change a new map's style
   * without a style document, first load an empty style, such as
   * `{"version":8,"sources":{},"layers":[]}`.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.nativeError` when the style document fails to load or isn't a
   *   valid style.
   * - `Status.cancelled` when the map is released, or this function or
   *   `MapHandle.setStyleJson()` loads another style, before the style document
   *   parses.
   */
  setStyleUrl(url: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_url(this.id, url, completion),
    );
  }

  /**
   * Sets how the map chooses which tiles to load, and at what detail.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setTileOptions(options: MapTileOptions): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_tile_options(this.id, options, completion),
    );
  }

  /**
   * Sets how the map orients and constrains its view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setViewportOptions(options: MapViewportOptions): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_viewport_options(this.id, options, completion),
    );
  }

  /**
   * Moves the camera, at once or with an animation.
   *
   * Camera fields that `update` doesn't set keep their values, and the update
   * stops any running animation of the fields it sets. The function finishes
   * when the map applies the update, before an animation ends. To learn when an
   * animation ends, set an `CameraTransitionHandler` in its animation options.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  updateCamera(update: CameraUpdate): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_update_camera(this.id, update, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }
}
defineDisposers(MapHandle, "release", "dispose");

/**
 * The commands of a {@link MapHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class MapHandleDiscarding {
  readonly #owner: MapHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: MapHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Adds a source whose GeoJSON tiles come from callbacks.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomGeometrySource(
    sourceId: string,
    options: CustomGeometrySourceOptions,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_add_custom_geometry_source(
        this.#owner.id,
        sourceId,
        options,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds a source whose vector tiles come from callbacks.
   *
   * See `mln_map_add_custom_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomVectorSource(
    sourceId: string,
    options: CustomVectorSourceOptions,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_add_custom_vector_source(
        this.#owner.id,
        sourceId,
        options,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds a GeoJSON source with no data.
   *
   * Give it data with `MapHandle.setGeoJsonSourceData()` or
   * `MapHandle.setGeoJsonSourceUrl()`. Its options stay fixed for as long as
   * the style holds it.
   *
   * `attribution` is the source's
   * &#91;attribution&#93;(https://maplibre.org/maplibre-style-spec/sources/#attribution),
   * or empty for none.
   *
   * See `mln_map_add_geo_json_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when an option in `options` is out of range.
   */
  addGeoJsonSource(
    sourceId: string,
    options?: GeoJsonSourceOptions,
    attribution?: string,
  ): void {
    native.mln_map_add_geo_json_source(
      this.#owner.id,
      sourceId,
      options,
      attribution,
      undefined,
    );
  }

  /**
   * Adds an image source that shows `image`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image,
  ): void {
    native.mln_map_add_image_source_image(
      this.#owner.id,
      sourceId,
      coordinates,
      image,
      undefined,
    );
  }

  /**
   * Adds a layer to the map's style.
   *
   * `layerJson` is a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   * The layer goes below the layer named `beforeLayerId`, or above every layer
   * when `beforeLayerId` is empty.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleLayerJson(layerJson: unknown, beforeLayerId?: string): void {
    native.mln_map_add_style_layer_json(
      this.#owner.id,
      layerJson,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Adds a source to the map's style.
   *
   * `sourceJson` is a
   * &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/) object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleSourceJson(sourceId: string, sourceJson: unknown): void {
    native.mln_map_add_style_source_json(
      this.#owner.id,
      sourceId,
      sourceJson,
      undefined,
    );
  }

  /**
   * Moves the camera relative to where it is, as a gesture does.
   *
   * `delta` can pan the map's content by an offset on the screen, scale the
   * zoom, and add to the bearing and pitch. An anchor keeps one screen point in
   * place while the scale, bearing, or pitch changes. An anchor needs one of
   * those changes, and combines with an offset only when the delta has no
   * animation. The function finishes when the map applies the delta, before an
   * animation ends.
   *
   * A delta that replaces an animated one starts from where that animation left
   * the camera, so the rest of the replaced change never happens. To reach an
   * exact camera, use `MapHandle.updateCamera()`.
   *
   * A delta in the end phase can carry momentum, which keeps the camera moving
   * from the gesture's release velocities and slows it on each rendered frame.
   * The gesture lasts until the momentum ends: the snapshot keeps
   * `gesture_in_progress` set, and camera events report
   * `CameraChangeSource.momentum`. With momentum, `animation` sets only
   * `transition_id` and the end handler, which runs when the gesture ends.
   *
   * A delta in the begin phase, `MapHandle.cancelCameraTransitions()`, or a
   * cancellation by that ID stops the momentum. A later command that sets a
   * field stops only the parts of the momentum that move that field. On a
   * tilted map, the pan moves the map's content the same distance on the screen
   * in every direction.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  applyCameraDelta(delta: CameraDelta): void {
    withRegistrations((registrations) => {
      native.mln_map_apply_camera_delta(this.#owner.id, delta, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Holds back the map's changes from rendering until
   * `MapHandle.endCommandGroup()`, so they show in the same frame.
   *
   * Groups nest, and the changes render when the outermost group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  beginCommandGroup(): void {
    native.mln_map_begin_command_group(this.#owner.id, undefined);
  }

  /**
   * Stops every running camera animation where it is.
   *
   * The end handler of each stopped animation reports
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransitions(): void {
    native.mln_map_cancel_camera_transitions(this.#owner.id, undefined);
  }

  /**
   * Stops the running camera animations whose animation options set
   * `transitionId`.
   *
   * Other animations keep running. The end handler of each stopped animation
   * reports `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transitions_with_id` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransitionsWithId(transitionId: bigint): void {
    native.mln_map_cancel_camera_transitions_with_id(
      this.#owner.id,
      transitionId,
      undefined,
    );
  }

  /**
   * Logs debugging information about the map's style and sources.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  dumpDebugLogs(): void {
    native.mln_map_dump_debug_logs(this.#owner.id, undefined);
  }

  /**
   * Ends the group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  endCommandGroup(): void {
    native.mln_map_end_command_group(this.#owner.id, undefined);
  }

  /**
   * Moves the camera to fit a GeoJSON geometry in the map's view, at once or
   * with an animation.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
   * feature. The camera is the one that `MapHandle.cameraForGeometry()`
   * computes after the commands sent before this one apply. The function
   * finishes when the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  fitCameraToGeometry(geometry: unknown, update: CameraFitUpdate): void {
    withRegistrations((registrations) => {
      native.mln_map_fit_camera_to_geometry(
        this.#owner.id,
        geometry,
        update,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Moves the camera to fit `bounds` in the map's view, at once or with an
   * animation.
   *
   * The camera is the one that `MapHandle.cameraForLatLngBounds()` computes
   * after the commands sent before this one apply. The function finishes when
   * the map applies the change, before an animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  fitCameraToLatLngBounds(bounds: LatLngBounds, update: CameraFitUpdate): void {
    withRegistrations((registrations) => {
      native.mln_map_fit_camera_to_lat_lng_bounds(
        this.#owner.id,
        bounds,
        update,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Moves the camera to fit `coordinates` in the map's view, at once or with an
   * animation.
   *
   * `coordinates` must not be empty. The camera is the one that
   * `MapHandle.cameraForLatLngs()` computes after the commands sent before this
   * one apply. The function finishes when the map applies the change, before an
   * animation ends.
   *
   * See `mln_map_fit_camera_to_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  fitCameraToLatLngs(
    coordinates: readonly LatLng[],
    update: CameraFitUpdate,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_fit_camera_to_lat_lngs(
        this.#owner.id,
        coordinates,
        update,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Makes a custom geometry source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomGeometrySourceRegion(
    sourceId: string,
    bounds: LatLngBounds,
  ): void {
    native.mln_map_invalidate_custom_geometry_source_region(
      this.#owner.id,
      sourceId,
      bounds,
      undefined,
    );
  }

  /**
   * Makes a custom geometry source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomGeometrySourceTile(
    sourceId: string,
    tileId: CanonicalTileId,
  ): void {
    native.mln_map_invalidate_custom_geometry_source_tile(
      this.#owner.id,
      sourceId,
      tileId,
      undefined,
    );
  }

  /**
   * Makes a custom vector source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_vector_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomVectorSourceRegion(
    sourceId: string,
    bounds: LatLngBounds,
  ): void {
    native.mln_map_invalidate_custom_vector_source_region(
      this.#owner.id,
      sourceId,
      bounds,
      undefined,
    );
  }

  /**
   * Makes a custom vector source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId,
  ): void {
    native.mln_map_invalidate_custom_vector_source_tile(
      this.#owner.id,
      sourceId,
      tileId,
      undefined,
    );
  }

  /**
   * Moves a layer of the map's style below the layer named `beforeLayerId`, or
   * above every layer when `beforeLayerId` is empty.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  moveStyleLayer(layerId: string, beforeLayerId?: string): void {
    native.mln_map_move_style_layer(
      this.#owner.id,
      layerId,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Removes the feature state that `selector` selects.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  removeFeatureState(selector: FeatureStateSelector): void {
    native.mln_map_remove_feature_state(this.#owner.id, selector, undefined);
  }

  /**
   * Removes an image from the map's style.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleImage(imageId: string): void {
    native.mln_map_remove_style_image(this.#owner.id, imageId, undefined);
  }

  /**
   * Removes a layer from the map's style.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleLayer(layerId: string): void {
    native.mln_map_remove_style_layer(this.#owner.id, layerId, undefined);
  }

  /**
   * Removes a source from the map's style.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleSource(sourceId: string): void {
    native.mln_map_remove_style_source(this.#owner.id, sourceId, undefined);
  }

  /**
   * Renders a new frame of a continuous map, even when nothing changed.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestRepaint(): void {
    native.mln_map_request_repaint(this.#owner.id, undefined);
  }

  /**
   * Sets the map's size.
   *
   * A map with a render session takes its size from the session, so resize the
   * session with `RenderSessionHandle.resize()` instead.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the map has a render session.
   */
  resize(logicalSize: LogicalSize): void {
    native.mln_map_resize(this.#owner.id, logicalSize, undefined);
  }

  /**
   * Limits the camera's center to an area, and its zoom and pitch to ranges.
   *
   * Limits that `options` doesn't set keep their current values. When the
   * camera is outside the new limits, it moves inside them.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setBounds(options: BoundOptions): void {
    native.mln_map_set_bounds(this.#owner.id, options, undefined);
  }

  /**
   * Sets the data of a tile in a custom geometry source.
   *
   * `data` is a GeoJSON feature collection, feature, or geometry. The source
   * ignores data for a tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: unknown,
  ): void {
    native.mln_map_set_custom_geometry_source_tile_data(
      this.#owner.id,
      sourceId,
      tileId,
      data,
      undefined,
    );
  }

  /**
   * Reports that a tile of a custom geometry source failed to load.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): void {
    native.mln_map_set_custom_geometry_source_tile_error(
      this.#owner.id,
      sourceId,
      tileId,
      message,
      undefined,
    );
  }

  /**
   * Sets the data of a tile in a custom vector source.
   *
   * `data` is the tile, encoded as `encoding`. The source ignores data for a
   * tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: Uint8Array,
  ): void {
    native.mln_map_set_custom_vector_source_tile_data(
      this.#owner.id,
      sourceId,
      tileId,
      encoding,
      data,
      undefined,
    );
  }

  /**
   * Reports that a tile of a custom vector source failed to load.
   *
   * See `mln_map_set_custom_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): void {
    native.mln_map_set_custom_vector_source_tile_error(
      this.#owner.id,
      sourceId,
      tileId,
      message,
      undefined,
    );
  }

  /**
   * Draws debugging overlays, such as tile borders, on the map.
   *
   * `flags` replaces the overlays set before.
   *
   * See `mln_map_set_debug_flags` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugFlags(flags: MapDebugFlag): void {
    native.mln_map_set_debug_flags(this.#owner.id, flags, undefined);
  }

  /**
   * Selects the events that the map reports.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventFlag): void {
    native.mln_map_set_event_mask(this.#owner.id, mask, undefined);
  }

  /**
   * Merges `state` into the state of a feature.
   *
   * `state` is a JSON object. Its keys replace the same keys in the feature's
   * state, and the other keys stay.
   *
   * The map keeps the state until it is removed, its source leaves the style,
   * or a style loads. `MapHandle.removeStyleSource()` removes the state of the
   * source's features, and a style that loads removes the state of every
   * source, so a later source with the same ID starts with no state. To set
   * state for a new style's sources, wait until `MapHandle.setStyleUrl()` or
   * `MapHandle.setStyleJson()` completes. The style load removes state set
   * before then.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(selector: FeatureStateSelector, state: unknown): void {
    native.mln_map_set_feature_state(
      this.#owner.id,
      selector,
      state,
      undefined,
    );
  }

  /**
   * Places the camera at a position and orientation in space.
   *
   * The map adjusts them to a camera that it can show. It stops running camera
   * animations first, and their end handlers report
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setFreeCameraOptions(options: FreeCameraOptions): void {
    native.mln_map_set_free_camera_options(this.#owner.id, options, undefined);
  }

  /**
   * Replaces the data of a GeoJSON source.
   *
   * `data` is a GeoJSON feature collection, feature, or geometry. The library
   * prepares it for the source with the source's options, off the threads that
   * the caller and the runtime use, and the command completes once the source
   * holds it. Later commands don't wait for it. While the source is still
   * preparing earlier data, only the newest data waits to follow it.
   *
   * With synchronous tiling on, as
   * `MapHandle.setGeoJsonSourceSynchronousTiling()` describes, the command
   * prepares `data` before later commands run, so the data shows in the next
   * frame.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceData(sourceId: string, data: unknown): void {
    native.mln_map_set_geo_json_source_data(
      this.#owner.id,
      sourceId,
      data,
      undefined,
    );
  }

  /**
   * Sets whether a GeoJSON source builds its tiles while the map renders.
   *
   * Building tiles while the map renders, instead of in the background, makes
   * new data show in the next frame. The setting replaces the
   * `synchronousUpdate` member of the source's JSON, which defaults to false.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceSynchronousTiling(sourceId: string, enabled: boolean): void {
    native.mln_map_set_geo_json_source_synchronous_tiling(
      this.#owner.id,
      sourceId,
      enabled,
      undefined,
    );
  }

  /**
   * Makes a GeoJSON source load its data from `url`.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceUrl(sourceId: string, url: string): void {
    native.mln_map_set_geo_json_source_url(
      this.#owner.id,
      sourceId,
      url,
      undefined,
    );
  }

  /**
   * Sets a value that the style's `global-state`
   * &#91;expressions&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * read.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGlobalStateProperty(propertyName: string, value: unknown): void {
    native.mln_map_set_global_state_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Moves the image of an image source.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceCoordinates(
    sourceId: string,
    coordinates: readonly LatLng[],
  ): void {
    native.mln_map_set_image_source_coordinates(
      this.#owner.id,
      sourceId,
      coordinates,
      undefined,
    );
  }

  /**
   * Replaces the image of an image source.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceImage(sourceId: string, image: PremultipliedRgba8Image): void {
    native.mln_map_set_image_source_image(
      this.#owner.id,
      sourceId,
      image,
      undefined,
    );
  }

  /**
   * Makes an image source load its image from `url`.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceUrl(sourceId: string, url: string): void {
    native.mln_map_set_image_source_url(
      this.#owner.id,
      sourceId,
      url,
      undefined,
    );
  }

  /**
   * Sets whether the map draws in perspective or axonometric projection, and
   * how much it skews the view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setProjectionMode(options: ProjectionModeOptions): void {
    native.mln_map_set_projection_mode(this.#owner.id, options, undefined);
  }

  /**
   * Shows or hides an overlay of rendering statistics on the map.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setRenderingStatsViewEnabled(enabled: boolean): void {
    native.mln_map_set_rendering_stats_view_enabled(
      this.#owner.id,
      enabled,
      undefined,
    );
  }

  /**
   * Adds an image to the map's style, or replaces the image named `imageId`.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image,
    options?: StyleImageOptions,
  ): void {
    native.mln_map_set_style_image(
      this.#owner.id,
      imageId,
      image,
      options,
      undefined,
    );
  }

  /**
   * Loads the map's style from `json`.
   *
   * The command completes once the style has parsed, and the map's style is
   * then loaded, as `MapHandle.setStyleUrl()` describes. The style's sources,
   * sprites, and glyphs load in the background.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleJson(json: unknown): void {
    native.mln_map_set_style_json(this.#owner.id, json, undefined);
  }

  /**
   * Sets a property of a layer in the map's style.
   *
   * `propertyName` is one of the layer's layout or paint
   * &#91;properties&#93;(https://maplibre.org/maplibre-style-spec/layers/), or
   * `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value`
   * resets any of these but `source` and `source-layer` to its default.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: unknown,
  ): void {
    native.mln_map_set_style_layer_property(
      this.#owner.id,
      layerId,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Replaces the
   * &#91;light&#93;(https://maplibre.org/maplibre-style-spec/light/) of the
   * map's style.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightJson(lightJson: unknown): void {
    native.mln_map_set_style_light_json(this.#owner.id, lightJson, undefined);
  }

  /**
   * Sets a property of the light of the map's style.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightProperty(propertyName: string, value: unknown): void {
    native.mln_map_set_style_light_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Sets whether a source is volatile.
   *
   * The map doesn't store the tiles of a volatile source in the persistent
   * cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleSourceVolatile(sourceId: string, isVolatile: boolean): void {
    native.mln_map_set_style_source_volatile(
      this.#owner.id,
      sourceId,
      isVolatile,
      undefined,
    );
  }

  /**
   * Sets the default transition of the map's style.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleTransitionOptions(options: StyleTransitionOptions): void {
    native.mln_map_set_style_transition_options(
      this.#owner.id,
      options,
      undefined,
    );
  }

  /**
   * Loads the map's style from `url`.
   *
   * The command completes once the style document has loaded and parsed. The
   * style's sources, sprites, and glyphs then load in the background.
   *
   * The map's style is loaded from the time a style document parses until the
   * next call to this function. While the style isn't loaded, the commands that
   * change it complete with `Status.invalidState`. To change a new map's style
   * without a style document, first load an empty style, such as
   * `{"version":8,"sources":{},"layers":[]}`.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleUrl(url: string): void {
    native.mln_map_set_style_url(this.#owner.id, url, undefined);
  }

  /**
   * Sets how the map chooses which tiles to load, and at what detail.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setTileOptions(options: MapTileOptions): void {
    native.mln_map_set_tile_options(this.#owner.id, options, undefined);
  }

  /**
   * Sets how the map orients and constrains its view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setViewportOptions(options: MapViewportOptions): void {
    native.mln_map_set_viewport_options(this.#owner.id, options, undefined);
  }

  /**
   * Moves the camera, at once or with an animation.
   *
   * Camera fields that `update` doesn't set keep their values, and the update
   * stops any running animation of the fields it sets. The function finishes
   * when the map applies the update, before an animation ends. To learn when an
   * animation ends, set an `CameraTransitionHandler` in its animation options.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  updateCamera(update: CameraUpdate): void {
    withRegistrations((registrations) => {
      native.mln_map_update_camera(this.#owner.id, update, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }
}

/**
 * A camera view for converting between screen points and coordinates.
 *
 * See `mln_map_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class MapProjectionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "MapProjectionHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Creates a projection that converts coordinates for one view of a map.
   *
   * Create it from the `view` of an `MapSnapshot` or an `RenderFrameResult`.
   * The projection doesn't follow the map, so create a new one after the map's
   * camera or size changes.
   *
   * See `mln_map_projection_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  static create(view: MapView): MapProjectionHandle {
    const handle = native.mln_map_projection_create(view);
    return adopt(
      handle,
      () => new MapProjectionHandle(ADOPT, handle),
      (disposed) => native.mln_map_projection_release(disposed, undefined),
    );
  }

  /**
   * Reads the projection's camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  getCamera(): MapProjectionCameraResult {
    return native.mln_map_projection_get_camera(this.id);
  }

  /**
   * Finds the coordinate at a screen point.
   *
   * The longitude stays between -180 and 180.
   *
   * See `mln_map_projection_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngForPixel(point: ScreenPoint): LatLng {
    return native.mln_map_projection_lat_lng_for_pixel(this.id, point);
  }

  /**
   * Finds the coordinate at a screen point, without wrapping its longitude.
   *
   * The longitude can fall outside -180 to 180, to show which copy of the world
   * the point is in.
   *
   * See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngForPixelUnwrapped(point: ScreenPoint): LatLng {
    return native.mln_map_projection_lat_lng_for_pixel_unwrapped(
      this.id,
      point,
    );
  }

  /**
   * Finds the coordinates at several screen points, as
   * `MapProjectionHandle.latLngForPixel()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngsForPixels(points: readonly ScreenPoint[]): readonly LatLng[] {
    return native.mln_map_projection_lat_lngs_for_pixels(this.id, points);
  }

  /**
   * Finds the coordinates at several screen points, as
   * `MapProjectionHandle.latLngForPixelUnwrapped()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngsForPixelsUnwrapped(points: readonly ScreenPoint[]): readonly LatLng[] {
    return native.mln_map_projection_lat_lngs_for_pixels_unwrapped(
      this.id,
      points,
    );
  }

  /**
   * Computes how many meters one screen pixel covers at `latitude`, at the
   * projection's zoom.
   *
   * The result ignores the camera's pitch.
   *
   * See `mln_map_projection_meters_per_pixel_at_latitude` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  metersPerPixelAtLatitude(latitude: number): number {
    return native.mln_map_projection_meters_per_pixel_at_latitude(
      this.id,
      latitude,
    );
  }

  /**
   * Finds the screen point of a coordinate.
   *
   * The point is in the copy of the world nearest the camera's center.
   *
   * See `mln_map_projection_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelForLatLng(coordinate: LatLng): ScreenPoint {
    return native.mln_map_projection_pixel_for_lat_lng(this.id, coordinate);
  }

  /**
   * Finds the screen points of several coordinates, as
   * `MapProjectionHandle.pixelForLatLng()` does.
   *
   * See `mln_map_projection_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelsForLatLngs(coordinates: readonly LatLng[]): readonly ScreenPoint[] {
    return native.mln_map_projection_pixels_for_lat_lngs(this.id, coordinates);
  }

  /**
   * Releases a projection.
   *
   * See `mln_map_projection_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_map_projection_release(handle, token),
    );
  }

  /**
   * Moves the projection's camera, without changing the map.
   *
   * Camera fields that `camera` doesn't set keep their values.
   *
   * See `mln_map_projection_set_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  setCamera(camera: CameraOptions): void {
    native.mln_map_projection_set_camera(this.id, camera);
  }
}
defineDisposers(MapProjectionHandle, "release", "release");

/**
 * A GPU device that the library creates to render maps offscreen.
 *
 * See `mln_offscreen_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class OffscreenDeviceHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "OffscreenDeviceHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Creates a GPU device for rendering maps offscreen.
   *
   * `MapHandle.attachOffscreen()` creates a device of its own when it gets
   * none. Create one to share it among the sessions on many maps, or to pick a
   * GPU.
   *
   * In a browser WebGPU build, attach without a device instead.
   *
   * See `mln_offscreen_device_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` in a browser WebGPU build, or when the device can't
   *   render with `client_api`.
   * - `Status.nativeError` when `adapter_index` names no GPU, or the platform
   *   can't create the device.
   */
  static create(options?: OffscreenDeviceOptions): OffscreenDeviceHandle {
    const handle = native.mln_offscreen_device_create(options);
    return adopt(
      handle,
      () => new OffscreenDeviceHandle(ADOPT, handle),
      (disposed) => native.mln_offscreen_device_release(disposed, undefined),
    );
  }

  /**
   * Releases an offscreen device.
   *
   * Render sessions on the device keep working until they detach.
   *
   * See `mln_offscreen_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_offscreen_device_release(handle, token),
    );
  }
}
defineDisposers(OffscreenDeviceHandle, "release", "release");

/**
 * A render session draws a map into a surface or a texture.
 *
 * See `mln_render_session` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class RenderSessionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: MapHandle,
    ready?: Promise<void>,
    registrations?: Registrations,
  ) {
    super(key, handle, "RenderSessionHandle", {
      parent,
      registrations,
      session: true,
    });
    this.ready = ready ?? Promise.resolve();
  }

  /**
   * Settles once the session is ready, and rejects with a `MaplibreError` when
   * it fails to become ready. Asynchronous calls on the session before then
   * wait for it. A rejection that nothing awaits isn't reported as unhandled.
   */
  readonly ready: Promise<void>;

  /** The commands of this owner that discard their outcomes. */
  get discarding(): RenderSessionHandleDiscarding {
    return new RenderSessionHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Stops the session right away, without running its pending graphics work.
   *
   * Use it when the target's graphics objects are going away and a detach can't
   * finish first. Pending work completes with `Status.targetLost`, and pending
   * frame demands finish with `RenderResult.targetNotReady`. Once the call
   * returns, the library no longer uses your graphics objects, and you can
   * attach another session to the map. Release the session afterward.
   *
   * Abandon keeps the session's OpenGL and WebGPU objects until the process
   * exits, and reports how many it kept. Detach the session instead to free
   * them.
   *
   * See `mln_render_session_abandon` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  abandon(): RenderAbandonResult {
    return native.mln_render_session_abandon(this.id);
  }

  /**
   * Takes the newest rendered frame from the session's texture ring, if one is
   * ready.
   *
   * Older frames that you haven't acquired are dropped, and the session can
   * render over a frame before you acquire it. While you hold a frame in every
   * texture of the ring, new frames wait until you release one. When no frame
   * is ready, it succeeds and writes the null handle.
   *
   * See `mln_render_session_acquire_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session lacks
   *   `RenderSessionCapabilityFlag.frameAcquisition`.
   * - `Status.targetLost` or `Status.nativeError`, as
   *   `RenderSessionSnapshot.failureStatus` reports, when the session has
   *   failed.
   * - `Status.invalidState` when you detached, abandoned, released, or disposed
   *   the session.
   */
  acquireFrame(): AcquiredFrameHandle | undefined {
    const handle = native.mln_render_session_acquire_frame(this.id);
    return handle === undefined
      ? undefined
      : adopt(
          handle,
          () => new AcquiredFrameHandle(ADOPT, handle, this),
          (disposed) => native.mln_acquired_frame_discard(disposed, undefined),
        );
  }

  /**
   * Ends the session's pending frame demands and waits for its earlier work.
   *
   * Frame demands that haven't started, including those that wait for a map
   * update, finish with `RenderResult.noUpdate`.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the barrier
   *   runs.
   */
  barrier(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_barrier(this.id, completion),
    );
  }

  /**
   * Clears the data that the session's renderer holds, such as loaded tiles,
   * and draws the map again.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  clearData(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_clear_data(this.id, completion),
    );
  }

  /**
   * Asks the session to render a frame.
   *
   * The frame shows the map commands that you sent before the call, except the
   * changes that an open command group holds back.
   *
   * You can call it once the attach call returns. While the attach runs, the
   * demand waits for it to complete. Every demand finishes with one frame
   * result that carries its token, and
   * `RenderSessionHandle.drainFrameResults()` takes it. A demand without
   * `FrameDemandFlag.force` replaces each pending demand with the same flags.
   *
   * See `mln_render_session_demand_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` or `Status.nativeError`, as
   *   `RenderSessionSnapshot.failureStatus` reports, when the session has
   *   failed.
   * - `Status.invalidState` when you detached, abandoned, released, or disposed
   *   the session, or discarded a frame from every texture of its ring.
   */
  demandFrame(demand: FrameDemand): void {
    native.mln_render_session_demand_frame(this.id, demand);
  }

  /**
   * Detaches the session from its map and frees its graphics objects.
   *
   * Release every acquired frame first. Pending frame demands finish with
   * `RenderResult.targetNotReady`. Once the detach completes, the library no
   * longer uses your graphics objects, and you can attach another session to
   * the map. Release the session afterward.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the detach
   *   runs.
   */
  detach(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_detach(this.id, completion),
    );
  }

  /**
   * Releases the session in any state, and finishes in the background.
   *
   * Use it from destructors and finalizers, and `RenderSessionHandle.release()`
   * elsewhere. It ends the session as a release does, except that it abandons a
   * session whose frames you hold, which ends those frames. Keep servicing a
   * session driven by `RenderDriverKind.callerGraphicsThread` until
   * `RenderSessionHandle.serviceDriverWork()` no longer accepts it.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) =>
      native.mln_render_session_dispose(handle, token),
    );
  }

  /**
   * Takes the frame results that the session has queued.
   *
   * When no result is queued, it succeeds and writes the null handle and an
   * empty view.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  drainFrameResults(): readonly RenderFrameResult[] {
    return native.mln_render_session_drain_frame_results(this.id) ?? [];
  }

  /**
   * Logs debugging information about the session's renderer.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  dumpDebugLogs(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_dump_debug_logs(this.id, completion),
    );
  }

  /**
   * Reads what the session supports.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getCapabilities(): RenderSessionCapabilities {
    return native.mln_render_session_get_capabilities(this.id);
  }

  /**
   * Reads the children of a cluster in a clustered GeoJSON source.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. The result
   * is a GeoJSON feature collection of the clusters and points that the cluster
   * splits into at the next zoom level.
   *
   * The query reads the source's data from the session's most recent render
   * attempt, so a change to the data shows once the session tries to render
   * again.
   *
   * A cluster ID belongs to one version of the source's data. After the data
   * changes, feature queries can return IDs from the earlier version until the
   * source's tiles reload, and such an ID completes with `Status.notFound` or
   * names a different cluster. Query the features again for current IDs.
   *
   * See `mln_render_session_get_cluster_children` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the session's most recent render attempt had no
   *   source named `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.invalidArgument` when the source isn't a GeoJSON source with
   *   clustering turned on.
   */
  getClusterChildren(sourceId: string, clusterId: number): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_render_session_get_cluster_children(
        this.id,
        sourceId,
        clusterId,
        completion,
      ),
    );
  }

  /**
   * Reads the zoom level at which a cluster of a clustered GeoJSON source
   * splits.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. At the
   * result's zoom level, the cluster splits into two or more clusters or
   * points.
   *
   * A cluster ID can go out of date as
   * `RenderSessionHandle.getClusterChildren()` describes.
   *
   * See `mln_render_session_get_cluster_expansion_zoom` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the session's most recent render attempt had no
   *   source named `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.invalidArgument` when the source isn't a GeoJSON source with
   *   clustering turned on.
   */
  getClusterExpansionZoom(
    sourceId: string,
    clusterId: number,
  ): Promise<number> {
    return query<number>((completion) =>
      native.mln_render_session_get_cluster_expansion_zoom(
        this.id,
        sourceId,
        clusterId,
        completion,
      ),
    );
  }

  /**
   * Reads the points in a cluster of a clustered GeoJSON source.
   *
   * `clusterId` is the "cluster_id" property of a cluster feature. The result
   * is a GeoJSON feature collection of the original point features. It skips
   * the first `offset` points and then holds up to `limit` of them, so a
   * `limit` of 0 gives an empty collection.
   *
   * A cluster ID can go out of date as
   * `RenderSessionHandle.getClusterChildren()` describes.
   *
   * See `mln_render_session_get_cluster_leaves` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the session's most recent render attempt had no
   *   source named `sourceId`, or the source has no cluster `clusterId`.
   * - `Status.invalidArgument` when the source isn't a GeoJSON source with
   *   clustering turned on.
   */
  getClusterLeaves(
    sourceId: string,
    clusterId: number,
    limit: number,
    offset: number,
  ): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_render_session_get_cluster_leaves(
        this.id,
        sourceId,
        clusterId,
        limit,
        offset,
        completion,
      ),
    );
  }

  /**
   * Reads the message of the error that made the session fail.
   *
   * `RenderSessionSnapshot.failureStatus` reports the error's status. The
   * message is empty while the session hasn't failed, and doesn't change once
   * it fails.
   *
   * See `mln_render_session_get_failure` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getFailure(): string {
    return native.mln_render_session_get_failure(this.id);
  }

  /**
   * Reads the session's current state.
   *
   * See `mln_render_session_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getSnapshot(): RenderSessionSnapshot {
    return native.mln_render_session_get_snapshot(this.id);
  }

  /**
   * Finds the features that the session drew in an area of the screen.
   *
   * The query reads the last frame that the session rendered. A feature that
   * several layers draw appears once for each layer.
   *
   * See `mln_render_session_query_rendered_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options?: RenderedFeatureQueryOptions,
  ): Promise<readonly RenderedFeature[]> {
    return query<readonly RenderedFeature[]>((completion) =>
      native.mln_render_session_query_rendered_features(
        this.id,
        geometry,
        options,
        completion,
      ),
    );
  }

  /**
   * Finds the features of the source named `sourceId`.
   *
   * The query covers only the tiles that the source has loaded for the current
   * view. A source loads tiles only while a visible layer uses it.
   *
   * See `mln_render_session_query_source_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  querySourceFeatures(
    sourceId: string,
    options?: SourceFeatureQueryOptions,
  ): Promise<readonly SourceFeature[]> {
    return query<readonly SourceFeature[]>((completion) =>
      native.mln_render_session_query_source_features(
        this.id,
        sourceId,
        options,
        completion,
      ),
    );
  }

  /**
   * Reads the pixels of the last frame that the session rendered.
   *
   * See `mln_render_session_read_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session's capabilities lack
   *   `RenderSessionCapabilityFlag.readback`.
   * - `Status.invalidState` when the session hasn't rendered a frame since it
   *   attached, resized, or changed its target.
   */
  readTexture(): Promise<TextureReadbackResult> {
    return query<TextureReadbackResult>((completion) =>
      native.mln_render_session_read_texture(this.id, completion),
    );
  }

  /**
   * Frees memory that the session's renderer can rebuild, such as caches.
   *
   * See `mln_render_session_reduce_memory_use` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  reduceMemoryUse(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_reduce_memory_use(this.id, completion),
    );
  }

  /**
   * Releases the session, detaching it first when it is attached.
   *
   * Pending frame demands finish with `RenderResult.targetNotReady`. Once the
   * release completes, the library no longer uses your graphics objects, and
   * you can attach another session to the map.
   *
   * A session driven by `RenderDriverKind.callerGraphicsThread` detaches at
   * your next `RenderSessionHandle.serviceDriverWork()`, so keep servicing it
   * until the release completes. When the session isn't attached, such as while
   * it attaches or after it fails, or when you release or dispose its map
   * before that service, the release abandons it as
   * `RenderSessionHandle.abandon()` does.
   *
   * See `mln_render_session_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` while you hold one of the session's acquired
   *   frames. Release or discard them first.
   */
  release(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_render_session_release(handle, token, completion),
      ),
    );
  }

  /**
   * Lets go of the session's surface or textures, and keeps the rest of the
   * session.
   *
   * Use it when the platform takes a surface away, such as when your app goes
   * to the background. Release every acquired frame first. Frame demands that
   * are still pending, and later ones until you give the session a new target,
   * finish with `RenderResult.targetNotReady`. Once the release completes, you
   * can destroy the old target. An Android or OpenHarmony window can go away
   * sooner, as `MapHandle.attachAndroidSurface()` and
   * `MapHandle.attachOhosSurface()` say.
   *
   * See `mln_render_session_release_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders offscreen or into Android
   *   hardware buffers, dmabufs, DXGI shared textures, or IOSurfaces that it
   *   allocates.
   * - `Status.unsupported` when the target is a WebGL canvas.
   * - `Status.targetLost` when the session loses its target before the release
   *   runs.
   */
  releaseTarget(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_release_target(this.id, completion),
    );
  }

  /**
   * Resizes the session's target and its map.
   *
   * The session renders at the map's scale factor. Release every acquired frame
   * first. You can call it once the attach call returns. While the attach runs,
   * the resize waits for it to complete.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders into a borrowed ring. Give
   *   it new textures of the new size instead.
   * - `Status.invalidState` while you hold one of the session's acquired
   *   frames, or when the session failed, or you detached, abandoned, released,
   *   or disposed it.
   * - `Status.targetLost` when the session loses its target before the resize
   *   applies.
   */
  resize(logicalSize: LogicalSize): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_render_session_resize(this.id, logicalSize, completion),
    );
  }

  /**
   * Runs the session's ready graphics work on your graphics thread.
   *
   * Use it with `RenderDriverKind.callerGraphicsThread`, and call it from the
   * same thread every time. Call it each time
   * `RenderSessionAttachOptions.driverWorkWake` is called, even while you
   * demand no frames, until a detach, release, or disposal of the session
   * finishes. It returns once no work is ready.
   *
   * It accepts a released or disposed session until that release or disposal
   * finishes.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  serviceDriverWork(): void {
    native.mln_render_session_service_driver_work(this.id);
  }

  /**
   * Replaces the Android window that the session presents to.
   *
   * Use it to give the session a window again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another window, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old window.
   *
   * See `mln_render_session_set_android_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `surface` has no window, such as after the
   *   platform destroyed it.
   * - `Status.unsupported` when, in a Vulkan build, the new window needs a
   *   different color format or pre-rotation support than the old one. Attach a
   *   new session instead.
   * - `Status.nativeError` when the library can't create the window's surface.
   */
  setAndroidSurfaceTarget(
    replacement: AndroidSurfaceReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_android_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Sets the frame pacer that demands the session's frames.
   *
   * At the first display refresh after the session has something new to draw,
   * `pacer` demands a frame without flags and with token 0. One pacer can serve
   * many sessions. A null `pacer` stops pacing the session. You can call it
   * once the attach call returns.
   *
   * See `mln_render_session_set_frame_pacer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` or `Status.nativeError`, as
   *   `RenderSessionSnapshot.failureStatus` reports, when the session has
   *   failed.
   * - `Status.invalidState` when you attached the session with a redraw wake,
   *   or detached, abandoned, released, or disposed it.
   */
  setFramePacer(pacer?: FramePacerHandle): void {
    native.mln_render_session_set_frame_pacer(this.id, pacer?.id);
  }

  /**
   * Replaces the Metal textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * Once the replacement completes, the session no longer uses the old
   * textures.
   *
   * See `mln_render_session_set_metal_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new textures have a different pixel format
   *   than the old ones. Attach a new session to change it.
   */
  setMetalBorrowedTextureTarget(
    replacement: MetalBorrowedTextureReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_metal_borrowed_texture_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the Metal layer that the session renders to.
   *
   * Use it to give the session a layer again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another layer, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old layer.
   *
   * See `mln_render_session_set_metal_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setMetalSurfaceTarget(replacement: MetalSurfaceReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_metal_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the OpenHarmony window that the session presents to.
   *
   * Use it to give the session a window again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another window, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old window.
   *
   * See `mln_render_session_set_ohos_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when no surface in this process has
   *   `OhosSurfaceReplacement.surfaceId`, such as after the platform destroyed
   *   it.
   * - `Status.unsupported` when, in a Vulkan build, the new window needs a
   *   different color format or pre-rotation support than the old one. Attach a
   *   new session instead.
   * - `Status.nativeError` when the library can't create the window's surface.
   */
  setOhosSurfaceTarget(replacement: OhosSurfaceReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_ohos_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the OpenGL textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * Once the replacement completes, the session no longer uses the old
   * textures.
   *
   * See `mln_render_session_set_open_gl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setOpenGlBorrowedTextureTarget(
    replacement: OpenGlBorrowedTextureReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_open_gl_borrowed_texture_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the OpenGL surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another surface, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old surface.
   *
   * See `mln_render_session_set_open_gl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setOpenGlSurfaceTarget(replacement: OpenGlSurfaceReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_open_gl_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the Vulkan images that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session images again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * Once the replacement completes, the session no longer uses the old images.
   *
   * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setVulkanBorrowedTextureTarget(
    replacement: VulkanBorrowedTextureReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_vulkan_borrowed_texture_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the Vulkan surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another surface, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old surface.
   *
   * See `mln_render_session_set_vulkan_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new surface needs a different color format
   *   or pre-rotation support than the old one. Attach a new session instead.
   */
  setVulkanSurfaceTarget(replacement: VulkanSurfaceReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_vulkan_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the Wayland surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another surface, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old surface.
   *
   * See `mln_render_session_set_wayland_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when, in a Vulkan build, the new surface needs a
   *   different color format than the old one. Attach a new session instead.
   * - `Status.nativeError` when the library can't present to the new surface.
   */
  setWaylandSurfaceTarget(
    replacement: WaylandSurfaceReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_wayland_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the WebGPU textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * Once the replacement completes, the session no longer uses the old
   * textures.
   *
   * See `mln_render_session_set_web_gpu_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setWebGpuBorrowedTextureTarget(
    replacement: WebGpuBorrowedTextureReplacement,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_web_gpu_borrowed_texture_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the WebGPU surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another surface, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old surface.
   *
   * See `mln_render_session_set_web_gpu_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setWebGpuSurfaceTarget(replacement: WebGpuSurfaceReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_web_gpu_surface_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }

  /**
   * Replaces the Win32 window that the session presents to.
   *
   * Use it to give the session a window again after
   * `RenderSessionHandle.releaseTarget()`, to move it to another window, or to
   * resize it. Once the replacement completes, the session no longer uses the
   * old window.
   *
   * See `mln_render_session_set_win32_window_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `window` isn't a window, or, in an OpenGL
   *   build, already has another pixel format.
   * - `Status.invalidState` when `window` is minimized or has no client area.
   * - `Status.invalidState` when `window` was minimized before the replacement
   *   ran.
   * - `Status.unsupported` when, in a Vulkan build, the new window needs a
   *   different color format than the old one. Attach a new session instead.
   * - `Status.nativeError` when the library can't present to the new window.
   */
  setWin32WindowTarget(replacement: Win32WindowReplacement): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_win32_window_target(
        this.id,
        replacement,
        completion,
      ),
    );
  }
}
defineDisposers(RenderSessionHandle, "release", "dispose");

/**
 * The commands of a {@link RenderSessionHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class RenderSessionHandleDiscarding {
  readonly #owner: RenderSessionHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: RenderSessionHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Resizes the session's target and its map.
   *
   * The session renders at the map's scale factor. Release every acquired frame
   * first. You can call it once the attach call returns. While the attach runs,
   * the resize waits for it to complete.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders into a borrowed ring. Give
   *   it new textures of the new size instead.
   * - `Status.invalidState` while you hold one of the session's acquired
   *   frames, or when the session failed, or you detached, abandoned, released,
   *   or disposed it.
   */
  resize(logicalSize: LogicalSize): void {
    native.mln_render_session_resize(this.#owner.id, logicalSize, undefined);
  }
}

/**
 * A resource load that a resource provider answers.
 *
 * See `mln_resource_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class ResourceRequestHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "ResourceRequestHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Answers a resource request.
   *
   * A failed answer fails the request with `ResourceErrorReason.other`, as
   * `ResourceRequestHandle.release()` does.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the library cancelled the request.
   */
  complete(response: ResourceResponse): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_request_complete(handle, token, response),
    );
  }

  /**
   * Reads whether the library cancelled a resource request.
   *
   * See `mln_resource_request_is_cancelled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  isCancelled(): boolean {
    return native.mln_resource_request_is_cancelled(this.id);
  }

  /**
   * Hands a resource request back to the library to load as usual.
   *
   * A failed call fails the request with `ResourceErrorReason.other`, as
   * `ResourceRequestHandle.release()` does.
   *
   * See `mln_resource_request_pass_through` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the library cancelled the request.
   */
  passThrough(): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_request_pass_through(handle, token),
    );
  }

  /**
   * Releases a resource request.
   *
   * Releasing a request without an answer fails it with
   * `ResourceErrorReason.other`.
   *
   * See `mln_resource_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_resource_request_release(handle, token),
    );
  }

  /**
   * Calls a callback when the library cancels a resource request.
   *
   * The callback never runs after the request is answered. A request takes one
   * cancel callback. When the library has already cancelled the request, the
   * call reports that instead and the callback never runs.
   *
   * See `mln_resource_request_set_cancel_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setCancelCallback(handler: ResourceRequestCancelHandler): boolean {
    return withRegistrations((registrations) => {
      const result = native.mln_resource_request_set_cancel_callback(
        this.id,
        handler,
      );
      if (!result) registrations.commit(rootsOf(this));
      return result;
    });
  }
}
defineDisposers(ResourceRequestHandle, "release", "release");

/**
 * A URL rewrite that a resource transform answers with the URL to load.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class ResourceTransformRequestHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "ResourceTransformRequestHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Answers a resource transform request with the URL to load instead.
   *
   * `url` must not be empty. A failed answer fails the request with
   * `ResourceErrorReason.other`, as `ResourceTransformRequestHandle.release()`
   * does.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  complete(url: string): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_transform_request_complete(handle, token, url),
    );
  }

  /**
   * Fails a resource transform request without sending it.
   *
   * The library retries a request that failed for a reason it retries, such as
   * `ResourceErrorReason.connection`, and the retry calls the transform again.
   * When this call fails, the request fails with `ResourceErrorReason.other`,
   * as `ResourceTransformRequestHandle.release()` does.
   *
   * See `mln_resource_transform_request_fail` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  fail(error: ResourceTransformError): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_transform_request_fail(handle, token, error),
    );
  }

  /**
   * Releases a resource transform request.
   *
   * Releasing a request without an answer fails it with
   * `ResourceErrorReason.other`.
   *
   * See `mln_resource_transform_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_resource_transform_request_release(handle, token),
    );
  }
}
defineDisposers(ResourceTransformRequestHandle, "release", "release");

/**
 * A runtime owns maps and the network and cache settings that they share.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class RuntimeHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "RuntimeHandle", { registrations, root: true });
  }

  /**
   * The native handle value, which runtime events report as their
   * source. It stays readable after release, so events drained later
   * still match this handle.
   */
  declare readonly id: bigint;

  /** The commands of this owner that discard their outcomes. */
  get discarding(): RuntimeHandleDiscarding {
    return new RuntimeHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Stops sending the events of the runtime and its maps to a callback.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearEventHandler(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_event_handler(this.id, completion),
    );
  }

  /**
   * Stops adding headers to the runtime's HTTP requests.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearHttpHeaderTransform(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_http_header_transform(this.id, completion),
    );
  }

  /**
   * Removes the runtime's resource provider, so the library loads every request
   * itself.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceProvider(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_resource_provider(this.id, completion),
    );
  }

  /**
   * Stops rewriting the URLs of the runtime's network requests.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceTransform(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_resource_transform(this.id, completion),
    );
  }

  /**
   * Creates a runtime.
   *
   * A runtime holds maps and the resource loading, cache, and offline regions
   * that they share. In a browser, create runtimes on a Web Worker, not on the
   * main thread.
   *
   * Resolves once the call completes, so a browser page, which cannot block,
   * can call it.
   *
   * See `mln_runtime_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the cache database file that `options`
   *   names can't be opened or created.
   */
  static create(options?: RuntimeOptions): Promise<RuntimeHandle> {
    return threadStart(
      (start) => native.mln_runtime_create(options, start),
      (handle) =>
        adopt(
          handle,
          () => new RuntimeHandle(ADOPT, handle),
          (disposed) => native.mln_runtime_dispose(disposed, undefined),
        ),
    );
  }

  /**
   * Creates a map on `runtime`.
   *
   * Asynchronous calls on the map before the completion runs wait for it, and
   * complete with `Status.invalidState` when creation fails.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the `fallback_font_faces` of `options`
   *   isn't an array of font files.
   */
  createMap(options: MapOptions): MapHandle {
    return attach(
      (completion) =>
        native.mln_runtime_create_map(this.id, options, completion),
      (handle, ready) =>
        adopt(
          handle,
          () => new MapHandle(ADOPT, handle, this, ready),
          (disposed) => native.mln_map_dispose(disposed, undefined),
        ),
    );
  }

  /**
   * Creates an offline region in the runtime's cache file.
   *
   * `metadata` holds any bytes to store with the region, such as its name. The
   * region doesn't download until
   * `RuntimeHandle.setOfflineRegionDownloadState()` makes its download active.
   *
   * See `mln_runtime_create_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>((completion) =>
      native.mln_runtime_create_offline_region(
        this.id,
        definition,
        metadata,
        completion,
      ),
    );
  }

  /**
   * Deletes the offline region `regionId` from the runtime's cache file.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  deleteOfflineRegion(regionId: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_delete_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Releases a runtime in any state, without waiting.
   *
   * The runtime finishes its teardown after its maps are released.
   *
   * See `mln_runtime_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) =>
      native.mln_runtime_dispose(handle, token),
    );
  }

  /**
   * Reads the offline region `regionId`.
   *
   * The result is null when the cache file has no region with that ID.
   *
   * See `mln_runtime_get_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getOfflineRegion(regionId: bigint): Promise<OfflineRegionInfo | undefined> {
    return query<OfflineRegionInfo | undefined>((completion) =>
      native.mln_runtime_get_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Reads the download progress of the offline region `regionId`.
   *
   * While the download is active, the counts cover what it has visited. Once
   * it's inactive, they cover every resource the region holds, including those
   * that only earlier versions of its style listed, so the completed counts can
   * exceed the required counts.
   *
   * See `mln_runtime_get_offline_region_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  getOfflineRegionStatus(regionId: bigint): Promise<OfflineRegionStatus> {
    return query<OfflineRegionStatus>((completion) =>
      native.mln_runtime_get_offline_region_status(
        this.id,
        regionId,
        completion,
      ),
    );
  }

  /**
   * Makes maps check each resource of the offline region `regionId` with the
   * server before they use it.
   *
   * The region keeps its resources. Starting its download again fetches the
   * invalidated resources for offline use. If the style or a TileJSON document
   * is then missing or invalid, the download reports an offline region response
   * error and goes on from the stored copy. Resources that the current style no
   * longer lists stay in the region until it's deleted.
   *
   * See `mln_runtime_invalidate_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  invalidateOfflineRegion(regionId: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_invalidate_offline_region(
        this.id,
        regionId,
        completion,
      ),
    );
  }

  /**
   * Lists the offline regions in the runtime's cache file.
   *
   * See `mln_runtime_list_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  listOfflineRegions(): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>((completion) =>
      native.mln_runtime_list_offline_regions(this.id, completion),
    );
  }

  /**
   * Copies the offline regions of another cache file into the runtime's cache
   * file.
   *
   * A region whose definition and metadata match one that the runtime's file
   * already has isn't copied again. The result lists the runtime's copy of each
   * region in the other file.
   *
   * See `mln_runtime_merge_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `sideDatabasePath` isn't a readable
   *   database file.
   * - `Status.nativeError` when the merge would exceed the tile limit that
   *   `RuntimeEventType.offlineRegionTileCountLimitExceeded` describes.
   */
  mergeOfflineRegions(
    sideDatabasePath: string,
  ): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>((completion) =>
      native.mln_runtime_merge_offline_regions(
        this.id,
        sideDatabasePath,
        completion,
      ),
    );
  }

  /**
   * Releases a runtime.
   *
   * Release the runtime's maps first.
   *
   * See `mln_runtime_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  release(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_runtime_release(handle, token, completion),
      ),
    );
  }

  /**
   * Runs a maintenance action on the runtime's cache database.
   *
   * See `mln_runtime_run_cache_database_operation` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  runCacheDatabaseOperation(operation: CacheDatabaseOperation): Promise<void> {
    return run((completion) =>
      native.mln_runtime_run_cache_database_operation(
        this.id,
        operation,
        completion,
      ),
    );
  }

  /**
   * Sends the events of the runtime and its maps to a callback.
   *
   * The callback replaces the one set before.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventHandler(handler: RuntimeEventHandler): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_event_handler(this.id, handler, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Adds headers to the runtime's HTTP requests.
   *
   * The transform replaces the one set before. In a browser or on OpenHarmony,
   * serve the requests with `RuntimeHandle.setResourceProvider()` instead.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the library runs in a browser or on
   *   OpenHarmony.
   */
  setHttpHeaderTransform(
    transform: HttpHeaderTransform,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_http_header_transform(
          this.id,
          transform,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Limits the size of the runtime's ambient cache.
   *
   * `size` is in bytes. Offline regions don't count toward it. It defaults to
   * 50 MB, or to 0 in the browser when the runtime has no cache file.
   *
   * See `mln_runtime_set_maximum_ambient_cache_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setMaximumAmbientCacheSize(size: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_maximum_ambient_cache_size(
        this.id,
        size,
        completion,
      ),
    );
  }

  /**
   * Starts or pauses the download of the offline region `regionId`.
   *
   * Setting the state that the download already has does nothing. A download
   * stops when the runtime closes.
   *
   * See `mln_runtime_set_offline_region_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  setOfflineRegionDownloadState(
    regionId: bigint,
    state: OfflineRegionDownloadState,
  ): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_offline_region_download_state(
        this.id,
        regionId,
        state,
        completion,
      ),
    );
  }

  /**
   * Selects the events that the runtime reports about the download of the
   * offline region `regionId`.
   *
   * `events` holds bits of `RuntimeEventFlag.allRuntimeEvents` and replaces the
   * region's previous selection. `RuntimeEventFlag.none` stops the region's
   * events.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  setOfflineRegionObserved(
    regionId: bigint,
    events: RuntimeEventFlag,
  ): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_offline_region_observed(
        this.id,
        regionId,
        events,
        completion,
      ),
    );
  }

  /**
   * Serves the runtime's network requests through a callback.
   *
   * The provider receives the network requests that its routes match, and the
   * library loads the rest as usual. A provider can serve URL schemes that the
   * library can't load itself. The provider replaces the one set before.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceProvider(provider: ResourceProvider): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_resource_provider(this.id, provider, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Rewrites the URLs of the runtime's network requests.
   *
   * The transform replaces the one set before.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceTransform(
    transform: ResourceTransform,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_resource_transform(
          this.id,
          transform,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Replaces the metadata of the offline region `regionId`.
   *
   * See `mln_runtime_update_offline_region_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  updateOfflineRegionMetadata(
    regionId: bigint,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>((completion) =>
      native.mln_runtime_update_offline_region_metadata(
        this.id,
        regionId,
        metadata,
        completion,
      ),
    );
  }
}
defineDisposers(RuntimeHandle, "release", "dispose");

/**
 * The commands of a {@link RuntimeHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class RuntimeHandleDiscarding {
  readonly #owner: RuntimeHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: RuntimeHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Stops sending the events of the runtime and its maps to a callback.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearEventHandler(): void {
    native.mln_runtime_clear_event_handler(this.#owner.id, undefined);
  }

  /**
   * Stops adding headers to the runtime's HTTP requests.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearHttpHeaderTransform(): void {
    native.mln_runtime_clear_http_header_transform(this.#owner.id, undefined);
  }

  /**
   * Removes the runtime's resource provider, so the library loads every request
   * itself.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceProvider(): void {
    native.mln_runtime_clear_resource_provider(this.#owner.id, undefined);
  }

  /**
   * Stops rewriting the URLs of the runtime's network requests.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceTransform(): void {
    native.mln_runtime_clear_resource_transform(this.#owner.id, undefined);
  }

  /**
   * Sends the events of the runtime and its maps to a callback.
   *
   * The callback replaces the one set before.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventHandler(handler: RuntimeEventHandler): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_event_handler(this.#owner.id, handler, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds headers to the runtime's HTTP requests.
   *
   * The transform replaces the one set before. In a browser or on OpenHarmony,
   * serve the requests with `RuntimeHandle.setResourceProvider()` instead.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the library runs in a browser or on
   *   OpenHarmony.
   */
  setHttpHeaderTransform(transform: HttpHeaderTransform): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_http_header_transform(
        this.#owner.id,
        transform,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Serves the runtime's network requests through a callback.
   *
   * The provider receives the network requests that its routes match, and the
   * library loads the rest as usual. A provider can serve URL schemes that the
   * library can't load itself. The provider replaces the one set before.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceProvider(provider: ResourceProvider): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_resource_provider(
        this.#owner.id,
        provider,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Rewrites the URLs of the runtime's network requests.
   *
   * The transform replaces the one set before.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceTransform(transform: ResourceTransform): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_resource_transform(
        this.#owner.id,
        transform,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }
}

/**
 * A parsed style expression.
 *
 * See `mln_style_expression` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class StyleExpressionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "StyleExpressionHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Parses a style
   * &#91;expression&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * to evaluate without a map.
   *
   * See `mln_style_expression_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `expression` isn't a valid expression, or
   *   its result can't have the expected type.
   */
  static create(
    expression: unknown,
    options?: StyleExpressionOptions,
  ): StyleExpressionHandle {
    const handle = native.mln_style_expression_create(expression, options);
    return adopt(
      handle,
      () => new StyleExpressionHandle(ADOPT, handle),
      (disposed) => native.mln_style_expression_release(disposed, undefined),
    );
  }

  /**
   * Evaluates a style expression and writes its result as JSON.
   *
   * A number in the result that isn't finite becomes null. A color is `["rgba",
   * r, g, b, a]`, with red, green, and blue from 0 to 255 and alpha from 0 to
   * 1, each exact only to single precision. Formatted text is a `["format", …]`
   * expression, an image is `{"name": …, "available": …}`, padding is `[top,
   * right, bottom, left]`, and a variable anchor offset is `[anchor, [x, y],
   * …]`.
   *
   * See `mln_style_expression_evaluate` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the expression fails to evaluate, such as
   *   when it reads an input that `inputs` doesn't set.
   */
  evaluate(inputs?: StyleExpressionInputs): unknown {
    return native.mln_style_expression_evaluate(this.id, inputs);
  }

  /**
   * Reads what a style expression's result depends on.
   *
   * See `mln_style_expression_get_info` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  getInfo(): StyleExpressionInfo {
    return native.mln_style_expression_get_info(this.id);
  }

  /**
   * Releases a parsed style expression.
   *
   * See `mln_style_expression_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_style_expression_release(handle, token),
    );
  }
}
defineDisposers(StyleExpressionHandle, "release", "release");

/**
 * A Vulkan instance, device, and queue that the library creates and owns.
 *
 * See `mln_vulkan_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class VulkanDeviceHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "VulkanDeviceHandle", { registrations });
  }
  /**
   * Calls `release()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `release()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Creates a Vulkan instance, device, and graphics queue for render sessions.
   *
   * Attach sessions with the context from `VulkanDeviceHandle.getContext()`, in
   * place of a device of your own. Without `device_uuid` or `device_luid`, it
   * prefers a discrete GPU. The device enables the external memory and
   * semaphore extensions that the GPU offers.
   *
   * See `mln_vulkan_device_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`, or when `presentation` is true and the Vulkan
   *   loader or the GPU can't present.
   * - `Status.notFound` when no GPU matches `device_uuid` and `device_luid`.
   * - `Status.nativeError` when there is no Vulkan loader, no GPU has a
   *   graphics queue, or Vulkan can't create the instance or the device.
   */
  static create(options?: VulkanDeviceOptions): VulkanDeviceHandle {
    const handle = native.mln_vulkan_device_create(options);
    return adopt(
      handle,
      () => new VulkanDeviceHandle(ADOPT, handle),
      (disposed) => native.mln_vulkan_device_release(disposed, undefined),
    );
  }

  /**
   * Gets the context to attach render sessions to the device with.
   *
   * Pass it as the context of `MapHandle.attachVulkanSurface()` or
   * `MapHandle.attachVulkanBorrowedTexture()`. The sessions on the device share
   * its graphics queue. To submit to the queue yourself, set
   * `RenderSessionAttachOptions.queueLock` on each session.
   *
   * See `mln_vulkan_device_get_context` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
   */
  getContext(): VulkanDeviceContextResult {
    return native.mln_vulkan_device_get_context(this.id);
  }

  /**
   * Releases a Vulkan device.
   *
   * Render sessions that attached with the device's context keep using it until
   * they detach or their release finishes.
   *
   * See `mln_vulkan_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
   */
  release(): void {
    closeSync(this, (handle, token) =>
      native.mln_vulkan_device_release(handle, token),
    );
  }
}
defineDisposers(VulkanDeviceHandle, "release", "release");

/**
 * A `AndroidHardwareBufferFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withAndroidHardwareBuffer} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedAndroidHardwareBufferFrame {
  readonly #value: AndroidHardwareBufferFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    scope: ViewScope,
    value: AndroidHardwareBufferFrame,
  ) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): AndroidHardwareBufferFrame["width"] {
    return this.#value.width;
  }

  get height(): AndroidHardwareBufferFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): AndroidHardwareBufferFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): AndroidHardwareBufferFrame["slot"] {
    return this.#value.slot;
  }

  get buffer(): AndroidHardwareBufferFrame["buffer"] {
    return this.#scope.active(this.#value).buffer;
  }
}

/**
 * A `DmaBufFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withDmaBuf} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedDmaBufFrame {
  readonly #value: DmaBufFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: DmaBufFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): DmaBufFrame["width"] {
    return this.#value.width;
  }

  get height(): DmaBufFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): DmaBufFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): DmaBufFrame["slot"] {
    return this.#value.slot;
  }

  get fourcc(): DmaBufFrame["fourcc"] {
    return this.#value.fourcc;
  }

  get modifier(): DmaBufFrame["modifier"] {
    return this.#value.modifier;
  }

  get planes(): DmaBufFrame["planes"] {
    return this.#scope.active(this.#value).planes;
  }
}

/**
 * A `DxgiSharedTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withDxgiSharedTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedDxgiSharedTextureFrame {
  readonly #value: DxgiSharedTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: DxgiSharedTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): DxgiSharedTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): DxgiSharedTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): DxgiSharedTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): DxgiSharedTextureFrame["slot"] {
    return this.#value.slot;
  }

  get format(): DxgiSharedTextureFrame["format"] {
    return this.#value.format;
  }

  get handle(): DxgiSharedTextureFrame["handle"] {
    return this.#scope.active(this.#value).handle;
  }
}

/**
 * A `GpuSync` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withProducerSync} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedGpuSync {
  readonly #value: GpuSync;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: GpuSync) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get kind(): GpuSync["kind"] {
    return this.#value.kind;
  }

  get handle(): GpuSync["handle"] {
    return this.#scope.active(this.#value).handle;
  }

  get value(): GpuSync["value"] {
    return this.#value.value;
  }
}

/**
 * A `IoSurfaceFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withIoSurface} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedIoSurfaceFrame {
  readonly #value: IoSurfaceFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: IoSurfaceFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): IoSurfaceFrame["width"] {
    return this.#value.width;
  }

  get height(): IoSurfaceFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): IoSurfaceFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): IoSurfaceFrame["slot"] {
    return this.#value.slot;
  }

  get surface(): IoSurfaceFrame["surface"] {
    return this.#scope.active(this.#value).surface;
  }
}

/**
 * A `MetalTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withMetalTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedMetalTextureFrame {
  readonly #value: MetalTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: MetalTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): MetalTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): MetalTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): MetalTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): MetalTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): MetalTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get device(): MetalTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get pixelFormat(): MetalTextureFrame["pixelFormat"] {
    return this.#value.pixelFormat;
  }
}

/**
 * A `OpenGlTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withOpenGlTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedOpenGlTextureFrame {
  readonly #value: OpenGlTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: OpenGlTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): OpenGlTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): OpenGlTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): OpenGlTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): OpenGlTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): OpenGlTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get target(): OpenGlTextureFrame["target"] {
    return this.#value.target;
  }
}

/**
 * A `VulkanTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withVulkanTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedVulkanTextureFrame {
  readonly #value: VulkanTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: VulkanTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): VulkanTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): VulkanTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): VulkanTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): VulkanTextureFrame["slot"] {
    return this.#value.slot;
  }

  get image(): VulkanTextureFrame["image"] {
    return this.#scope.active(this.#value).image;
  }

  get imageView(): VulkanTextureFrame["imageView"] {
    return this.#scope.active(this.#value).imageView;
  }

  get device(): VulkanTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get format(): VulkanTextureFrame["format"] {
    return this.#value.format;
  }

  get layout(): VulkanTextureFrame["layout"] {
    return this.#value.layout;
  }
}

/**
 * A `WebGpuTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withWebGpuTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedWebGpuTextureFrame {
  readonly #value: WebGpuTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: WebGpuTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get width(): WebGpuTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): WebGpuTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): WebGpuTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get slot(): WebGpuTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): WebGpuTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get textureView(): WebGpuTextureFrame["textureView"] {
    return this.#scope.active(this.#value).textureView;
  }

  get device(): WebGpuTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get format(): WebGpuTextureFrame["format"] {
    return this.#value.format;
  }
}

// How a dispatched call reaches the host value that its registration
// rooted, by the callback type that the addon's trampoline numbers.
defineCallbacks([
  // mln_camera_transition_end_callback
  (target, args) => {
    const callback = (target as CameraTransitionHandler)
      .callback as NonNullable<CameraTransitionHandler["callback"]>;
    const [end] = args as Parameters<
      NonNullable<CameraTransitionHandler["callback"]>
    >;
    callback(end);
  },
  // mln_custom_source_cancel_tile_callback
  (target, args) => {
    const callback = (target as CustomGeometrySourceOptions)
      .cancelTile as NonNullable<CustomGeometrySourceOptions["cancelTile"]>;
    const [map, sourceId, tileId] = args as Parameters<
      NonNullable<CustomGeometrySourceOptions["cancelTile"]>
    >;
    const mapOwner = new MapHandle(BORROW, map as unknown as bigint, undefined);
    callback(mapOwner, sourceId, tileId);
  },
  // mln_custom_source_fetch_tile_callback
  (target, args) => {
    const callback = (target as CustomGeometrySourceOptions).fetchTile;
    const [map, sourceId, tileId] = args as Parameters<
      NonNullable<CustomGeometrySourceOptions["fetchTile"]>
    >;
    const mapOwner = new MapHandle(BORROW, map as unknown as bigint, undefined);
    callback(mapOwner, sourceId, tileId);
  },
  // mln_log_callback
  (target, args) => {
    const callback = (target as LogHandler).callback;
    const [severity, event, code, message] = args as Parameters<
      NonNullable<LogHandler["callback"]>
    >;
    callback(severity, event, code, message);
  },
  // mln_resource_provider_callback
  (target, args) => {
    const [request, handleValue] = args as Parameters<
      NonNullable<ResourceProvider["callback"]>
    >;
    const owner = adopt(
      handleValue as unknown as bigint,
      () => new ResourceRequestHandle(ADOPT, handleValue as unknown as bigint),
      (disposed) => native.mln_resource_request_release(disposed, undefined),
    );
    try {
      const callback = (target as ResourceProvider).callback;
      callback(request, owner);
    } catch (error) {
      owner.release();
      throw error;
    }
  },
  // mln_resource_request_cancel_callback
  (target, _args) => {
    const callback = (target as ResourceRequestCancelHandler).callback;
    callback();
  },
  // mln_resource_transform_callback
  (target, args) => {
    const [kind, url, request] = args as Parameters<
      NonNullable<ResourceTransform["callback"]>
    >;
    const owner = adopt(
      request as unknown as bigint,
      () =>
        new ResourceTransformRequestHandle(ADOPT, request as unknown as bigint),
      (disposed) =>
        native.mln_resource_transform_request_release(disposed, undefined),
    );
    try {
      const callback = (target as ResourceTransform).callback as NonNullable<
        ResourceTransform["callback"]
      >;
      callback(kind, url, owner);
    } catch (error) {
      owner.release();
      throw error;
    }
  },
  // mln_runtime_event_callback
  (target, args) => {
    const callback = (target as RuntimeEventHandler).callback;
    const [events] = args as Parameters<
      NonNullable<RuntimeEventHandler["callback"]>
    >;
    callback(events);
  },
  // mln_wake_callback
  (target, _args) => {
    const callback = (target as Exclude<Wake, NativeRegistration>)
      .callback as NonNullable<Exclude<Wake, NativeRegistration>["callback"]>;
    callback();
  },
]);
