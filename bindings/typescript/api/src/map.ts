/**
 * A map, which shares its runtime's owner context.
 *
 * Camera commands are accepted synchronously and their effects arrive as runtime
 * events, so a host applies a command and then pumps and polls. Style loading
 * works the same way: the call is accepted, and the load reports through events.
 */

import type { AnimationOptions, CameraOptions } from "./camera.ts";
import { MaplibreError } from "./errors.ts";
import { MapIdentity, NamedValue } from "./events.ts";
import type { GeoJson } from "./geojson.ts";
import {
  type CallbackRegistry,
  CustomGeometryListener,
  CustomGeometryRegistration,
  type CustomGeometryTile,
  writeSize,
} from "./internal/callbacks.ts";
import { writeGeoJson } from "./internal/geojson-encode.ts";
import { HandleState } from "./internal/handle.ts";
import { stringView, writeJsonValue } from "./internal/json-encode.ts";
import type { Native } from "./internal/native.ts";
import { asRawEnum, asUint32 } from "./internal/numbers.ts";
import {
  attachHandleState,
  handleStateOf,
  registerMap,
  unregisterMap,
} from "./internal/private.ts";
import {
  copyOutText,
  readCameraOptions,
  writeAnimationOptions,
  writeCameraOptions,
} from "./internal/struct.ts";
import type { Ptr } from "./internal/transport.ts";
import type { JsonValue } from "./json.ts";
import { MapProjection } from "./projection.ts";
import { EP } from "./raw/entrypoints.ts";
import {
  MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_FIELD,
  MLN_MAP_MODE,
  MLN_STYLE_IMAGE_OPTION_FIELD,
} from "./raw/enums.ts";
import {
  ATTACH,
  type MetalBorrowedTexture,
  type MetalContext,
  type OpenGlBorrowedTexture,
  type OpenGlContext,
  RenderSession,
  type RenderTargetExtent,
  type SurfaceDescriptor,
  type VulkanBorrowedTexture,
  type VulkanContext,
  type VulkanOwnedTextureDescriptor,
  type WebGpuBorrowedTexture,
  type WebGpuContext,
} from "./render.ts";
import type { Runtime } from "./runtime.ts";

/** How a map renders. */
export class MapMode extends NamedValue {
  /** Renders continuously and reports render updates. */
  static readonly continuous = new MapMode(
    MLN_MAP_MODE.MLN_MAP_MODE_CONTINUOUS,
    "continuous",
  );
  /** Renders one still image per request. */
  static readonly static = new MapMode(
    MLN_MAP_MODE.MLN_MAP_MODE_STATIC,
    "static",
  );
  /** Renders nothing, for tile fetching and offline work. */
  static readonly tile = new MapMode(MLN_MAP_MODE.MLN_MAP_MODE_TILE, "tile");

  static fromRawValue(rawValue: number): MapMode {
    return (
      [MapMode.continuous, MapMode.static, MapMode.tile].find(
        (value) => value.rawValue === rawValue,
      ) ?? new MapMode(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** How a map is configured at creation. */
export interface MapOptions {
  /** Initial logical map width in UI pixels. */
  readonly width: number;
  /** Initial logical map height in UI pixels. */
  readonly height: number;
  /**
   * UI-to-device pixel scale.
   *
   * Unlike the extent, this is fixed for the map's life and selects the sprites,
   * glyphs, and raster tiles every frame uses.
   */
  readonly scaleFactor?: number;
  readonly mode?: MapMode;
  /** Whether this map decodes FastPFOR-encoded MLT tiles. */
  readonly fastPforEnabled?: boolean;
}

/** An image the style can name, in premultiplied RGBA. */
export interface StyleImage {
  readonly width: number;
  readonly height: number;
  /** Premultiplied RGBA bytes, row-major, copied at the boundary. */
  readonly pixels: Uint8Array;
  /** How many image pixels one logical pixel is. */
  readonly pixelRatio?: number;
  /** Whether the image is a signed distance field the style may recolor. */
  readonly sdf?: boolean;
}

/** The map's logical viewport. */
export interface MapSize {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
}

/** How a custom geometry source is configured. */
export interface CustomGeometrySourceOptions {
  readonly minZoom?: number;
  readonly maxZoom?: number;
}

export class Map {
  readonly #state: HandleState;
  readonly #identity: MapIdentity;
  readonly #runtime: object;
  readonly #callbacks: CallbackRegistry;

  private constructor(
    native: Native,
    id: bigint,
    runtimeState: HandleState,
    runtime: object,
    callbacks: CallbackRegistry,
  ) {
    this.#state = new HandleState(native, "Map", id, runtimeState);
    this.#state.watchForLeaks(this);
    this.#identity = new MapIdentity(id);
    this.#runtime = runtime;
    this.#callbacks = callbacks;
    attachHandleState(this, this.#state);
    registerMap(runtime, id, this);
  }

  /**
   * This map's identity, which an event it produced carries.
   *
   * The value compares and keys by the native object it names, and carries no
   * operations, so it cannot become a second owner.
   */
  get identity(): MapIdentity {
    return this.#identity;
  }

  /** @internal */
  static create(
    runtime: Runtime,
    native: Native,
    callbacks: CallbackRegistry,
    options: MapOptions,
  ): Map {
    const id = native.scope((scope) => {
      const layout = native.layout("mln_map_options");
      const storage = scope.allocateZeroed(layout.size);
      native.structValue(scope, EP.mln_map_options_default, storage);
      const view = native.memory.view(storage, layout.size);
      const fields = layout.fields;
      view.setUint32(
        fields.width!.offset,
        asUint32(options.width, "width"),
        true,
      );
      view.setUint32(
        fields.height!.offset,
        asUint32(options.height, "height"),
        true,
      );
      if (options.scaleFactor !== undefined) {
        view.setFloat64(fields.scale_factor!.offset, options.scaleFactor, true);
      }
      if (options.mode !== undefined) {
        view.setUint32(
          fields.map_mode!.offset,
          asRawEnum(options.mode.rawValue, "map mode"),
          true,
        );
      }
      if (options.fastPforEnabled !== undefined) {
        view.setUint8(
          fields.fast_pfor_enabled!.offset,
          options.fastPforEnabled ? 1 : 0,
        );
      }
      const outMap = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_map_create, [
        handleStateOf(runtime).use("Runtime.createMap"),
        storage,
        outMap,
      ]);
      return native.memory.view(outMap, 8).getBigUint64(0, true);
    });
    try {
      return new Map(native, id, handleStateOf(runtime), runtime, callbacks);
    } catch (error) {
      native.scope((scope) => {
        native.raw(scope, EP.mln_map_destroy, [id]);
      });
      throw error;
    }
  }

  /** The map's logical viewport size and its pixel ratio. */
  getSize(): MapSize {
    const id = this.#state.use("Map.getSize");
    const native = this.#state.native;
    return native.scope((scope) => {
      const width = scope.allocateZeroed(4);
      const height = scope.allocateZeroed(4);
      const scaleFactor = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_map_get_size, [
        id,
        width,
        height,
        scaleFactor,
      ]);
      return {
        width: native.memory.view(width, 4).getUint32(0, true),
        height: native.memory.view(height, 4).getUint32(0, true),
        scaleFactor: native.memory.view(scaleFactor, 8).getFloat64(0, true),
      };
    });
  }

  /** Requests a repaint for a continuous map. */
  requestRepaint(): void {
    this.#call("Map.requestRepaint", EP.mln_map_request_repaint, []);
  }

  /** Loads a style from a URL. The load reports through runtime events. */
  setStyleUrl(url: string): void {
    const id = this.#state.use("Map.setStyleUrl");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_set_style_url, [
        id,
        native.cString(scope, url, "url"),
      ]);
    });
  }

  /** Loads a style from a JSON document. */
  setStyleJson(json: string): void {
    const id = this.#state.use("Map.setStyleJson");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_set_style_json, [
        id,
        native.cString(scope, json, "json"),
      ]);
    });
  }

  /**
   * Copies the style document this map loaded.
   *
   * The bytes are what crossed the boundary, so a document read back here is the
   * document that was set. It is empty until a style loads.
   */
  copyLoadedStyleJson(): string {
    const id = this.#state.use("Map.copyLoadedStyleJson");
    return copyOutText(this.#state.native, EP.mln_map_copy_loaded_style_json, [
      id,
    ]);
  }

  /** Copies the URL this map's style was last requested from. */
  copyStyleUrl(): string {
    const id = this.#state.use("Map.copyStyleUrl");
    return copyOutText(this.#state.native, EP.mln_map_copy_style_url, [id]);
  }

  /** Reports whether the style and every tile it needs have loaded. */
  isFullyLoaded(): boolean {
    const id = this.#state.use("Map.isFullyLoaded");
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(1);
      native.checked(scope, EP.mln_map_is_fully_loaded, [id, out]);
      return native.memory.bytes(out, 1)[0] !== 0;
    });
  }

  /** The camera as the map last computed it. */
  getCamera(): CameraOptions {
    const id = this.#state.use("Map.getCamera");
    const native = this.#state.native;
    return native.scope((scope) => {
      // The C API checks a struct's `size` field before it fills one, so this
      // out-parameter starts from the default initializer rather than zeroed.
      const storage = scope.allocateZeroed(
        native.layout("mln_camera_options").size,
      );
      native.structValue(scope, EP.mln_camera_options_default, storage);
      native.checked(scope, EP.mln_map_get_camera, [id, storage]);
      return readCameraOptions(native, storage);
    });
  }

  /** Applies a camera command without a transition. */
  jumpTo(camera: CameraOptions): void {
    this.#cameraCommand("Map.jumpTo", EP.mln_map_jump_to, camera);
  }

  /** Applies an eased camera transition. */
  easeTo(camera: CameraOptions, animation: AnimationOptions = {}): void {
    this.#cameraTransition("Map.easeTo", EP.mln_map_ease_to, camera, animation);
  }

  /** Applies a fly-to camera transition. */
  flyTo(camera: CameraOptions, animation: AnimationOptions = {}): void {
    this.#cameraTransition("Map.flyTo", EP.mln_map_fly_to, camera, animation);
  }

  /** Cancels every camera transition in progress. */
  cancelTransitions(): void {
    this.#call("Map.cancelTransitions", EP.mln_map_cancel_transitions, []);
  }

  /** Brackets a host-driven gesture, which suspends camera easing. */
  setGestureInProgress(inProgress: boolean): void {
    this.#call("Map.setGestureInProgress", EP.mln_map_set_gesture_in_progress, [
      inProgress ? 1n : 0n,
    ]);
  }

  /** Reports whether a host-driven gesture is in progress. */
  isGestureInProgress(): boolean {
    const id = this.#state.use("Map.isGestureInProgress");
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(1);
      native.checked(scope, EP.mln_map_is_gesture_in_progress, [id, out]);
      return native.memory.bytes(out, 1)[0] !== 0;
    });
  }

  /**
   * Adds a source to the loaded style.
   *
   * The source descriptor is a structured JSON value, because MapLibre holds it
   * as one: an object member's order and an integer's alternative both matter to
   * what the style means.
   */
  addStyleSource(sourceId: string, source: JsonValue): void {
    const id = this.#state.use("Map.addStyleSource");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_add_style_source_json, [
        id,
        stringView(native, scope, sourceId),
        writeJsonValue(native, scope, source),
      ]);
    });
  }

  /** Reports whether the loaded style has this source. */
  hasStyleSource(sourceId: string): boolean {
    return this.#styleMemberCheck(
      "Map.hasStyleSource",
      EP.mln_map_style_source_exists,
      sourceId,
    );
  }

  /** Removes a source, reporting whether there was one to remove. */
  removeStyleSource(sourceId: string): boolean {
    return this.#styleMemberCheck(
      "Map.removeStyleSource",
      EP.mln_map_remove_style_source,
      sourceId,
    );
  }

  /**
   * Adds a layer to the loaded style.
   *
   * `beforeLayerId` names the layer this one goes under; omitting it puts the
   * layer on top.
   */
  addStyleLayer(layer: JsonValue, beforeLayerId?: string): void {
    const id = this.#state.use("Map.addStyleLayer");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_add_style_layer_json, [
        id,
        writeJsonValue(native, scope, layer),
        stringView(native, scope, beforeLayerId ?? ""),
      ]);
    });
  }

  /** Reports whether the loaded style has this layer. */
  hasStyleLayer(layerId: string): boolean {
    return this.#styleMemberCheck(
      "Map.hasStyleLayer",
      EP.mln_map_style_layer_exists,
      layerId,
    );
  }

  /** Removes a layer, reporting whether there was one to remove. */
  removeStyleLayer(layerId: string): boolean {
    return this.#styleMemberCheck(
      "Map.removeStyleLayer",
      EP.mln_map_remove_style_layer,
      layerId,
    );
  }

  /** Runs one of the style entry points that answers with a boolean. */
  #styleMemberCheck(
    operation: string,
    entrypoint: number,
    memberId: string,
  ): boolean {
    const id = this.#state.use(operation);
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(1);
      native.checked(scope, entrypoint, [
        id,
        stringView(native, scope, memberId),
        out,
      ]);
      return native.memory.bytes(out, 1)[0] !== 0;
    });
  }

  /**
   * Takes a projection helper from this map's current camera and viewport.
   *
   * The projection owns its snapshot, so it keeps answering after this map
   * closes.
   */
  createProjection(): MapProjection {
    this.#state.use("Map.createProjection");
    return MapProjection.create(this.#state.native, this);
  }

  /**
   * Adds an image the style can name, or replaces one it already has.
   *
   * The pixels are premultiplied RGBA and are copied at the boundary, so the
   * caller may reuse or mutate its buffer afterwards.
   */
  setStyleImage(imageId: string, image: StyleImage): void {
    const id = this.#state.use("Map.setStyleImage");
    const native = this.#state.native;
    const expected = image.width * image.height * 4;
    if (image.pixels.length < expected) {
      throw new MaplibreError(
        "invalidInput",
        `a ${image.width}x${image.height} premultiplied RGBA image needs ` +
          `${expected} bytes, and this one has ${image.pixels.length}`,
      );
    }
    native.scope((scope) => {
      const layout = native.layout("mln_premultiplied_rgba8_image");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      const view = native.memory.view(storage, layout.size);
      view.setUint32(layout.fields.size!.offset, layout.size, true);
      view.setUint32(
        layout.fields.width!.offset,
        asUint32(image.width, "width"),
        true,
      );
      view.setUint32(
        layout.fields.height!.offset,
        asUint32(image.height, "height"),
        true,
      );
      view.setUint32(layout.fields.stride!.offset, image.width * 4, true);
      const pixels = scope.allocate(expected, 1);
      native.memory
        .bytes(pixels, expected)
        .set(image.pixels.subarray(0, expected));
      native.memory.writePointer(
        (storage + BigInt(layout.fields.pixels!.offset)) as Ptr,
        pixels,
      );
      // The C API checks the byte length against the extent and the stride
      // rather than deriving it, so a short buffer is caught before it reads.
      writeSize(
        native,
        (storage + BigInt(layout.fields.byte_length!.offset)) as Ptr,
        expected,
      );

      const optionsLayout = native.layout("mln_style_image_options");
      const options = scope.allocateZeroed(
        optionsLayout.size,
        optionsLayout.align,
      );
      native.structValue(scope, EP.mln_style_image_options_default, options);
      const optionsView = native.memory.view(options, optionsLayout.size);
      let mask = 0;
      if (image.pixelRatio !== undefined) {
        // A four-byte field: writing a double here would run over `sdf`.
        optionsView.setFloat32(
          optionsLayout.fields.pixel_ratio!.offset,
          image.pixelRatio,
          true,
        );
        mask |= MLN_STYLE_IMAGE_OPTION_FIELD.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
      }
      if (image.sdf !== undefined) {
        optionsView.setUint8(
          optionsLayout.fields.sdf!.offset,
          image.sdf ? 1 : 0,
        );
        mask |= MLN_STYLE_IMAGE_OPTION_FIELD.MLN_STYLE_IMAGE_OPTION_SDF;
      }
      optionsView.setUint32(optionsLayout.fields.fields!.offset, mask, true);

      native.checked(scope, EP.mln_map_set_style_image, [
        id,
        stringView(native, scope, imageId),
        storage,
        options,
      ]);
    });
  }

  /** Reports whether the loaded style has this image. */
  hasStyleImage(imageId: string): boolean {
    return this.#styleMemberCheck(
      "Map.hasStyleImage",
      EP.mln_map_style_image_exists,
      imageId,
    );
  }

  /** Removes an image, reporting whether there was one to remove. */
  removeStyleImage(imageId: string): boolean {
    return this.#styleMemberCheck(
      "Map.removeStyleImage",
      EP.mln_map_remove_style_image,
      imageId,
    );
  }

  /**
   * Adds a GeoJSON source from data this host holds.
   *
   * The data crosses as the descriptor graph MapLibre holds rather than as
   * text, so an integer keeps the alternative it arrived as and an object keeps
   * its member order.
   */
  addGeoJsonSource(sourceId: string, data: GeoJson): void {
    const id = this.#state.use("Map.addGeoJsonSource");
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_geojson_source_options");
      const options = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(scope, EP.mln_geojson_source_options_default, options);
      native.checked(scope, EP.mln_map_add_geojson_source_data, [
        id,
        stringView(native, scope, sourceId),
        writeGeoJson(native, scope, data),
        options,
      ]);
    });
  }

  /** Replaces the data of a GeoJSON source, keeping the options it has. */
  setGeoJsonSourceData(sourceId: string, data: GeoJson): void {
    const id = this.#state.use("Map.setGeoJsonSourceData");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_set_geojson_source_data, [
        id,
        stringView(native, scope, sourceId),
        writeGeoJson(native, scope, data),
      ]);
    });
  }

  /**
   * Adds a source whose tiles this host supplies.
   *
   * MapLibre asks for a tile on one of its own threads and expects nothing
   * back, so the request is copied and the handler runs on this context. Answer
   * with `setCustomGeometryTileData`, whenever the data is ready.
   */
  addCustomGeometrySource(
    sourceId: string,
    handlers: {
      readonly onFetchTile: (tile: CustomGeometryTile) => void;
      readonly onCancelTile?: (tile: CustomGeometryTile) => void;
    },
    options: CustomGeometrySourceOptions = {},
  ): void {
    const id = this.#state.use("Map.addCustomGeometrySource");
    const native = this.#state.native;
    const registration = new CustomGeometryRegistration(
      native,
      this.#callbacks,
      handlers.onFetchTile,
      handlers.onCancelTile,
    );
    try {
      this.#addCustomGeometrySource(
        id,
        sourceId,
        registration,
        options,
        handlers.onCancelTile !== undefined,
      );
    } catch (error) {
      // The registration was made before the call, because native code needs
      // the listener addresses to install. A call that refuses leaves it
      // holding the handlers, and through them this map, for the runtime's
      // life, so it is retired here rather than waiting for a teardown that
      // never comes.
      registration.retire();
      this.#callbacks.discard(registration.id);
      throw error;
    }
  }

  #addCustomGeometrySource(
    id: bigint,
    sourceId: string,
    registration: CustomGeometryRegistration,
    options: CustomGeometrySourceOptions,
    cancels: boolean,
  ): void {
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_custom_geometry_source_options");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(
        scope,
        EP.mln_custom_geometry_source_options_default,
        storage,
      );
      const view = native.memory.view(storage, layout.size);
      native.memory.writePointer(
        (storage + BigInt(layout.fields.fetch_tile!.offset)) as Ptr,
        native.transport.listenerAddress(CustomGeometryListener.fetch),
      );
      if (cancels) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.cancel_tile!.offset)) as Ptr,
          native.transport.listenerAddress(CustomGeometryListener.cancel),
        );
      }
      native.memory.writePointer(
        (storage + BigInt(layout.fields.user_data!.offset)) as Ptr,
        registration.id as Ptr,
      );
      let mask = 0;
      if (options.minZoom !== undefined) {
        view.setFloat64(layout.fields.min_zoom!.offset, options.minZoom, true);
        mask |=
          MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_FIELD.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
      }
      if (options.maxZoom !== undefined) {
        view.setFloat64(layout.fields.max_zoom!.offset, options.maxZoom, true);
        mask |=
          MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_FIELD.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
      }
      view.setUint32(layout.fields.fields!.offset, mask, true);

      native.checked(scope, EP.mln_map_add_custom_geometry_source, [
        id,
        stringView(native, scope, sourceId),
        storage,
      ]);
    });
  }

  /** Supplies the data for one tile a custom geometry source asked about. */
  setCustomGeometryTileData(
    sourceId: string,
    tile: CustomGeometryTile,
    data: GeoJson,
  ): void {
    const id = this.#state.use("Map.setCustomGeometryTileData");
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_canonical_tile_id");
      const tileStorage = scope.allocateZeroed(layout.size, layout.align);
      const view = native.memory.view(tileStorage, layout.size);
      view.setUint32(layout.fields.z!.offset, asUint32(tile.z, "tile z"), true);
      view.setUint32(layout.fields.x!.offset, asUint32(tile.x, "tile x"), true);
      view.setUint32(layout.fields.y!.offset, asUint32(tile.y, "tile y"), true);
      native.checked(scope, EP.mln_map_set_custom_geometry_source_tile_data, [
        id,
        stringView(native, scope, sourceId),
        tileStorage,
        writeGeoJson(native, scope, data),
      ]);
    });
  }

  /**
   * Attaches a Vulkan render session that owns its texture.
   *
   * The calling context becomes the session's owner. Attaching validates that
   * this map is live rather than that the caller owns it, so a host may render
   * from a context that never touches the map.
   */
  attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
  ): RenderSession {
    this.#state.use("Map.attachVulkanOwnedTexture");
    return RenderSession.attachVulkanOwnedTexture(
      this.#state.native,
      this,
      descriptor,
    );
  }

  /** Attaches a Metal session that presents through a host surface. */
  attachMetalSurface(
    descriptor: SurfaceDescriptor<MetalContext>,
  ): RenderSession {
    return ATTACH.metalSurface(this.#state.native, this, descriptor);
  }

  /** Attaches a Vulkan session that presents through a host surface. */
  attachVulkanSurface(
    descriptor: SurfaceDescriptor<VulkanContext>,
  ): RenderSession {
    return ATTACH.vulkanSurface(this.#state.native, this, descriptor);
  }

  /** Attaches an OpenGL session that presents through a host surface. */
  attachOpenGlSurface(
    descriptor: SurfaceDescriptor<OpenGlContext>,
  ): RenderSession {
    return ATTACH.openglSurface(this.#state.native, this, descriptor);
  }

  /** Attaches a Metal session that owns its texture. */
  attachMetalOwnedTexture(descriptor: {
    extent: RenderTargetExtent;
    context: MetalContext;
  }): RenderSession {
    return ATTACH.metalOwnedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches an OpenGL session that owns its texture. */
  attachOpenGlOwnedTexture(descriptor: {
    extent: RenderTargetExtent;
    context: OpenGlContext;
  }): RenderSession {
    return ATTACH.openglOwnedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches a WebGPU session that owns its texture. */
  attachWebGpuOwnedTexture(descriptor: {
    extent: RenderTargetExtent;
    context: WebGpuContext;
  }): RenderSession {
    return ATTACH.webgpuOwnedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches a Metal session that renders into a texture the host owns. */
  attachMetalBorrowedTexture(descriptor: MetalBorrowedTexture): RenderSession {
    return ATTACH.metalBorrowedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches a Vulkan session that renders into an image the host owns. */
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTexture,
  ): RenderSession {
    return ATTACH.vulkanBorrowedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches an OpenGL session that renders into a texture the host owns. */
  attachOpenGlBorrowedTexture(
    descriptor: OpenGlBorrowedTexture,
  ): RenderSession {
    return ATTACH.openglBorrowedTexture(this.#state.native, this, descriptor);
  }

  /** Attaches a WebGPU session that renders into a texture the host owns. */
  attachWebGpuBorrowedTexture(
    descriptor: WebGpuBorrowedTexture,
  ): RenderSession {
    return ATTACH.webgpuBorrowedTexture(this.#state.native, this, descriptor);
  }

  /** Releases the map. Closing twice succeeds. */
  close(): void {
    if (this.#state.isClosed) {
      return;
    }
    const native = this.#state.native;
    this.#state.close((id) => {
      native.scope((scope) => {
        native.checked(scope, EP.mln_map_destroy, [id]);
      });
      // The id stops naming this map only once native release succeeds, so a
      // failed destroy leaves the event path resolving it as before.
      unregisterMap(this.#runtime, id);
    });
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }

  #call(operation: string, entrypoint: number, args: readonly bigint[]): void {
    const id = this.#state.use(operation);
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, entrypoint, [id, ...args]);
    });
  }

  #cameraCommand(
    operation: string,
    entrypoint: number,
    camera: CameraOptions,
  ): void {
    const id = this.#state.use(operation);
    const native = this.#state.native;
    native.scope((scope) => {
      const storage: Ptr = writeCameraOptions(native, scope, camera);
      native.checked(scope, entrypoint, [id, storage]);
    });
  }

  #cameraTransition(
    operation: string,
    entrypoint: number,
    camera: CameraOptions,
    animation: AnimationOptions,
  ): void {
    const id = this.#state.use(operation);
    const native = this.#state.native;
    native.scope((scope) => {
      const cameraStorage = writeCameraOptions(native, scope, camera);
      const animationStorage = writeAnimationOptions(native, scope, animation);
      native.checked(scope, entrypoint, [id, cameraStorage, animationStorage]);
    });
  }
}
