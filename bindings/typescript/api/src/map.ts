/**
 * A map, which shares its runtime's owner context.
 *
 * Camera commands are accepted synchronously and their effects arrive as runtime
 * events, so a host applies a command and then pumps and polls. Style loading
 * works the same way: the call is accepted, and the load reports through events.
 */

import type { AnimationOptions, CameraOptions } from "./camera.ts";
import { MapIdentity, NamedValue } from "./events.ts";
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
import { MLN_MAP_MODE } from "./raw/enums.ts";
import { RenderSession, type VulkanOwnedTextureDescriptor } from "./render.ts";
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

/** The map's logical viewport. */
export interface MapSize {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
}

export class Map {
  readonly #state: HandleState;
  readonly #identity: MapIdentity;
  readonly #runtime: object;

  private constructor(
    native: Native,
    id: bigint,
    runtimeState: HandleState,
    runtime: object,
  ) {
    this.#state = new HandleState(native, "Map", id, runtimeState);
    this.#state.watchForLeaks(this);
    this.#identity = new MapIdentity(id);
    this.#runtime = runtime;
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
  static create(runtime: Runtime, native: Native, options: MapOptions): Map {
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
      return new Map(native, id, handleStateOf(runtime), runtime);
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
