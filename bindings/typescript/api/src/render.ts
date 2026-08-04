/**
 * Render sessions.
 *
 * A session drives one map's rendering into a target the host owns. The binding
 * takes no ownership of backend resources: a `NativePointer` is an address the
 * host already has, and the host keeps it valid for the window the C API
 * documents.
 *
 * A session is affine to the thread that attached it, which need not be the
 * map's owner thread. Attaching validates that the map is live rather than that
 * the caller owns it, so a host may render from a context that never touches the
 * map.
 */

import { MaplibreError } from "./errors.ts";
import type { ScreenPoint } from "./geo.ts";
import { writeSize } from "./internal/callbacks.ts";
import { HandleState } from "./internal/handle.ts";
import {
  stringView,
  writeJsonValue,
  writeStringView,
} from "./internal/json-encode.ts";
import type { Scope } from "./internal/memory.ts";
import type { Native } from "./internal/native.ts";
import { asInt32, asUint32 } from "./internal/numbers.ts";
import { attachHandleState, handleStateOf } from "./internal/private.ts";
import { readQueriedFeature } from "./internal/query-decode.ts";
import type { Ptr } from "./internal/transport.ts";
import type { JsonValue } from "./json.ts";
import type { Map } from "./map.ts";
import type {
  QueriedFeature,
  RenderedFeatureQueryOptions,
  RenderedQueryGeometry,
  SourceFeatureQueryOptions,
} from "./query.ts";
import { EP } from "./raw/entrypoints.ts";
import {
  MLN_OPENGL_CONTEXT_PLATFORM,
  MLN_RENDERED_FEATURE_QUERY_OPTION_FIELD,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE,
  MLN_SOURCE_FEATURE_QUERY_OPTION_FIELD,
} from "./raw/enums.ts";
import { MLN_FEATURE_STATE_SELECTOR_FIELD } from "./raw/enums.ts";
import type { RecordLayout } from "./raw/layouts.ts";

declare const pointerBrand: unique symbol;

/**
 * A borrowed, opaque backend-native address.
 *
 * It transfers no ownership and grants no memory access. Public APIs accept one
 * only where the C API accepts a host-owned backend handle, and a host builds
 * one from an address its own graphics binding gave it.
 */
export type NativePointer = bigint & { readonly [pointerBrand]?: never };

/**
 * Names a backend address the host already owns.
 *
 * The caller states the lifetime: the address must stay valid for as long as the
 * C API documents that it borrows it.
 */
export function nativePointer(address: bigint): NativePointer {
  if (address < 0n || address > (1n << 64n) - 1n) {
    throw new MaplibreError(
      "invalidInput",
      `a native pointer is an unsigned 64-bit address, not ${address}`,
    );
  }
  return address as NativePointer;
}

/** How large a render target is, and at what device scale. */
export interface RenderTargetExtent {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor?: number;
}

/** A borrowed Vulkan context. Every handle is required. */
export interface VulkanContext {
  readonly instance: NativePointer;
  readonly physicalDevice: NativePointer;
  readonly device: NativePointer;
  readonly graphicsQueue: NativePointer;
  readonly graphicsQueueFamilyIndex: number;
  readonly getInstanceProcAddr: NativePointer;
}

/**
 * Which feature's state an operation names.
 *
 * The source is required. A vector source needs its layer to disambiguate, and
 * a key narrows a removal to one entry rather than the whole state.
 */
export interface FeatureStateSelector {
  readonly sourceId: string;
  readonly sourceLayerId?: string;
  readonly featureId?: string;
  readonly stateKey?: string;
}

/** A session that renders into a texture the session itself creates. */
export interface VulkanOwnedTextureDescriptor {
  readonly extent: RenderTargetExtent;
  readonly context: VulkanContext;
}

export class RenderSession {
  readonly #state: HandleState;

  /** @internal Builds a wrapper for a session the caller just attached. */
  static own(native: Native, id: bigint): RenderSession {
    return new RenderSession(native, id);
  }

  private constructor(native: Native, id: bigint) {
    // No parent: the C API keeps the map alive instead, by rejecting a map
    // destroy while a session is attached.
    this.#state = new HandleState(native, "RenderSession", id);
    this.#state.watchForLeaks(this);
    attachHandleState(this, this.#state);
  }

  /**
   * Attaches a Vulkan session that owns its texture.
   *
   * The calling context becomes the session's owner for the session's whole
   * life, and every other session operation from another context reports the
   * binding's wrong-thread error.
   */
  static attachVulkanOwnedTexture(
    native: Native,
    map: Map,
    descriptor: VulkanOwnedTextureDescriptor,
  ): RenderSession {
    const id = native.scope((scope) => {
      const layout = native.layout("mln_vulkan_owned_texture_descriptor");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(
        scope,
        EP.mln_vulkan_owned_texture_descriptor_default,
        storage,
      );
      writeExtent(
        native,
        (storage + BigInt(layout.fields.extent!.offset)) as Ptr,
        descriptor.extent,
      );
      writeVulkanContext(
        native,
        (storage + BigInt(layout.fields.context!.offset)) as Ptr,
        descriptor.context,
      );
      const outSession = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_vulkan_owned_texture_attach, [
        handleStateOf(map).use("Map.attachVulkanOwnedTexture"),
        storage,
        outSession,
      ]);
      return native.memory.view(outSession, 8).getBigUint64(0, true);
    });
    try {
      return new RenderSession(native, id);
    } catch (error) {
      native.scope((scope) => {
        native.raw(scope, EP.mln_render_session_destroy, [id]);
      });
      throw error;
    }
  }

  /**
   * Reports the features this session has rendered at a place on screen.
   *
   * The answer depends on which tiles are loaded and what the last frame
   * placed, which is why it belongs to a session rather than to the map.
   */
  queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions = {},
  ): readonly QueriedFeature[] {
    const id = this.#state.use("RenderSession.queryRenderedFeatures");
    const native = this.#state.native;
    const result = native.scope((scope) => {
      const layout = native.layout("mln_rendered_feature_query_options");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(
        scope,
        EP.mln_rendered_feature_query_options_default,
        storage,
      );
      let mask = 0;
      if (options.layerIds !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.layer_ids!.offset)) as Ptr,
          writeStringViews(native, scope, options.layerIds),
        );
        writeSize(
          native,
          (storage + BigInt(layout.fields.layer_id_count!.offset)) as Ptr,
          options.layerIds.length,
        );
        mask |=
          MLN_RENDERED_FEATURE_QUERY_OPTION_FIELD.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
      }
      if (options.filter !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.filter!.offset)) as Ptr,
          writeJsonValue(native, scope, options.filter),
        );
      }
      native.memory
        .view(storage, layout.size)
        .setUint32(layout.fields.fields!.offset, mask, true);

      const outResult = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_render_session_query_rendered_features, [
        id,
        writeQueryGeometry(native, scope, geometry),
        storage,
        outResult,
      ]);
      return native.memory.view(outResult, 8).getBigUint64(0, true);
    });
    return drainQueryResult(native, result);
  }

  /** Reports the features a source holds, whether or not they are rendered. */
  querySourceFeatures(
    sourceId: string,
    options: SourceFeatureQueryOptions = {},
  ): readonly QueriedFeature[] {
    const id = this.#state.use("RenderSession.querySourceFeatures");
    const native = this.#state.native;
    const result = native.scope((scope) => {
      const layout = native.layout("mln_source_feature_query_options");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(
        scope,
        EP.mln_source_feature_query_options_default,
        storage,
      );
      let mask = 0;
      if (options.sourceLayerIds !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.source_layer_ids!.offset)) as Ptr,
          writeStringViews(native, scope, options.sourceLayerIds),
        );
        writeSize(
          native,
          (storage +
            BigInt(layout.fields.source_layer_id_count!.offset)) as Ptr,
          options.sourceLayerIds.length,
        );
        mask |=
          MLN_SOURCE_FEATURE_QUERY_OPTION_FIELD.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
      }
      if (options.filter !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.filter!.offset)) as Ptr,
          writeJsonValue(native, scope, options.filter),
        );
      }
      native.memory
        .view(storage, layout.size)
        .setUint32(layout.fields.fields!.offset, mask, true);

      const outResult = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_render_session_query_source_features, [
        id,
        stringView(native, scope, sourceId),
        storage,
        outResult,
      ]);
      return native.memory.view(outResult, 8).getBigUint64(0, true);
    });
    return drainQueryResult(native, result);
  }

  /** Renders the map's latest update, reporting whether there was one. */
  renderUpdate(): boolean {
    const id = this.#state.use("RenderSession.renderUpdate");
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(1);
      native.checked(scope, EP.mln_render_session_render_update, [id, out]);
      return native.memory.bytes(out, 1)[0] !== 0;
    });
  }

  /**
   * Describes the image a readback would produce, without copying one.
   *
   * A size probe, so a host can size a buffer once and reuse it. A backend that
   * cannot read pixels back reports unsupported rather than answering with a
   * size, so this is not a way to ask whether readback works.
   */
  imageInfo(): TextureImageInfo {
    const id = this.#state.use("RenderSession.imageInfo");
    const native = this.#state.native;
    return native.scope((scope) => {
      const storage = this.#imageInfoStorage(scope);
      native.checked(scope, EP.mln_texture_read_premultiplied_rgba8, [
        id,
        0n,
        0n,
        storage,
      ]);
      return readImageInfo(native, storage);
    });
  }

  /**
   * Copies the last rendered frame into `into` as premultiplied RGBA8.
   *
   * `into` is the caller's, and stays the caller's: a buffer too small for the
   * image leaves it untouched and reports what size was needed, so a host can
   * grow it and ask again.
   */
  readPremultipliedRgba8(into: Uint8Array): TextureImageInfo {
    const id = this.#state.use("RenderSession.readPremultipliedRgba8");
    const native = this.#state.native;
    return native.scope((scope) => {
      const storage = this.#imageInfoStorage(scope);
      // The C API writes through a pointer, and a host buffer is not native
      // memory, so the copy lands in a scope allocation first.
      const pixels = scope.allocateZeroed(Math.max(into.byteLength, 1));
      native.checked(scope, EP.mln_texture_read_premultiplied_rgba8, [
        id,
        pixels,
        BigInt(into.byteLength),
        storage,
      ]);
      const info = readImageInfo(native, storage);
      into.set(native.memory.bytes(pixels, info.byteLength));
      return info;
    });
  }

  /**
   * Takes the most recently rendered texture, which is the session's until it
   * is given back.
   *
   * While a frame is held the session renders nothing new and every operation
   * that would disturb the texture reports invalid state, so the frame is
   * released in a `finally` wherever one is taken.
   */
  acquireOpenGlFrame(): OpenGlTextureFrame {
    const id = this.#state.use("RenderSession.acquireOpenGlFrame");
    const native = this.#state.native;
    return native.scope((scope) => {
      const layout = native.layout("mln_opengl_owned_texture_frame");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.memory
        .view(storage, layout.size)
        .setUint32(layout.fields.size!.offset, layout.size, true);
      native.checked(scope, EP.mln_opengl_owned_texture_acquire_frame, [
        id,
        storage,
      ]);
      const view = native.memory.view(storage, layout.size);
      const read = (field: string): number =>
        view.getUint32(layout.fields[field]!.offset, true);
      return new OpenGlTextureFrame(
        this,
        native.memory.bytes(storage, layout.size).slice(),
        {
          width: read("width"),
          height: read("height"),
          scaleFactor: view.getFloat64(
            layout.fields.scale_factor!.offset,
            true,
          ),
          texture: read("texture"),
          target: read("target"),
          internalFormat: read("internal_format"),
          format: read("format"),
          type: read("type"),
        },
      );
    });
  }

  /** @internal Gives a frame back, which only the frame calls. */
  releaseOpenGlFrame(record: Uint8Array): void {
    const id = this.#state.use("RenderSession.releaseOpenGlFrame");
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_opengl_owned_texture_frame");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.memory.bytes(storage, layout.size).set(record);
      native.checked(scope, EP.mln_opengl_owned_texture_release_frame, [
        id,
        storage,
      ]);
    });
  }

  #imageInfoStorage(scope: Scope): Ptr {
    const native = this.#state.native;
    const layout = native.layout("mln_texture_image_info");
    const storage = scope.allocateZeroed(layout.size, layout.align);
    native.structValue(scope, EP.mln_texture_image_info_default, storage);
    native.memory
      .view(storage, layout.size)
      .setUint32(layout.fields.size!.offset, layout.size, true);
    return storage;
  }

  /**
   * Points this session at another texture the host owns.
   *
   * The session is kept, along with the map it renders and the state it holds;
   * only where it draws changes. A session that owns its target has nothing to
   * replace and reports unsupported.
   */
  setOpenGlBorrowedTexture(descriptor: OpenGlBorrowedTexture): void {
    const id = this.#state.use("RenderSession.setOpenGlBorrowedTexture");
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_opengl_borrowed_texture_descriptor");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      native.structValue(
        scope,
        EP.mln_opengl_borrowed_texture_descriptor_default,
        storage,
      );
      fillExtent(native, storage, layout, descriptor.extent);
      fillPhysicalSize(native, storage, layout, descriptor);
      writeOpenGlContext(
        native,
        (storage + BigInt(layout.fields.context!.offset)) as Ptr,
        descriptor.context,
      );
      const view = native.memory.view(storage, layout.size);
      view.setUint32(
        layout.fields.texture!.offset,
        asUint32(descriptor.texture, "a texture name"),
        true,
      );
      view.setUint32(
        layout.fields.target!.offset,
        asUint32(descriptor.target, "a texture target"),
        true,
      );
      native.checked(scope, EP.mln_opengl_borrowed_texture_set_target, [
        id,
        storage,
      ]);
    });
  }

  /** Resizes the target, which the map applies on its next pump. */
  resize(extent: RenderTargetExtent): void {
    const id = this.#state.use("RenderSession.resize");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_render_session_resize, [
        id,
        BigInt(asUint32(extent.width, "width")),
        BigInt(asUint32(extent.height, "height")),
        f64Bits(extent.scaleFactor ?? 1),
      ]);
    });
  }

  /**
   * Sets per-feature state on a source this session renders.
   *
   * Feature state lives on the renderer rather than on the map, so it belongs
   * to a session: a resize that retires the renderer starts it empty again.
   */
  setFeatureState(selector: FeatureStateSelector, state: JsonValue): void {
    const id = this.#state.use("RenderSession.setFeatureState");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_render_session_set_feature_state, [
        id,
        writeFeatureStateSelector(native, scope, selector),
        writeJsonValue(native, scope, state),
      ]);
    });
  }

  /** Removes per-feature state, or one key of it. */
  removeFeatureState(selector: FeatureStateSelector): void {
    const id = this.#state.use("RenderSession.removeFeatureState");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_render_session_remove_feature_state, [
        id,
        writeFeatureStateSelector(native, scope, selector),
      ]);
    });
  }

  /** Releases the backend resources, leaving this handle usable. */
  detach(): void {
    this.#call("RenderSession.detach", EP.mln_render_session_detach);
  }

  /** Asks the renderer to release what it can. */
  reduceMemoryUse(): void {
    this.#call(
      "RenderSession.reduceMemoryUse",
      EP.mln_render_session_reduce_memory_use,
    );
  }

  /** Clears the renderer's cached data. */
  clearData(): void {
    this.#call("RenderSession.clearData", EP.mln_render_session_clear_data);
  }

  /** Writes the renderer's debug state to the log. */
  dumpDebugLogs(): void {
    this.#call(
      "RenderSession.dumpDebugLogs",
      EP.mln_render_session_dump_debug_logs,
    );
  }

  /** Releases the session, on the context that attached it. */
  close(): void {
    const native = this.#state.native;
    this.#state.close((id) => {
      native.scope((scope) => {
        native.checked(scope, EP.mln_render_session_destroy, [id]);
      });
    });
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }

  #call(operation: string, entrypoint: number): void {
    const id = this.#state.use(operation);
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, entrypoint, [id]);
    });
  }
}

function writeFeatureStateSelector(
  native: Native,
  scope: Scope,
  selector: FeatureStateSelector,
): Ptr {
  const layout = native.layout("mln_feature_state_selector");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  writeStringView(
    native,
    scope,
    (storage + BigInt(layout.fields.source_id!.offset)) as Ptr,
    selector.sourceId,
  );
  let mask = 0;
  if (selector.sourceLayerId !== undefined) {
    writeStringView(
      native,
      scope,
      (storage + BigInt(layout.fields.source_layer_id!.offset)) as Ptr,
      selector.sourceLayerId,
    );
    mask |=
      MLN_FEATURE_STATE_SELECTOR_FIELD.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
  }
  if (selector.featureId !== undefined) {
    writeStringView(
      native,
      scope,
      (storage + BigInt(layout.fields.feature_id!.offset)) as Ptr,
      selector.featureId,
    );
    mask |=
      MLN_FEATURE_STATE_SELECTOR_FIELD.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  }
  if (selector.stateKey !== undefined) {
    writeStringView(
      native,
      scope,
      (storage + BigInt(layout.fields.state_key!.offset)) as Ptr,
      selector.stateKey,
    );
    mask |=
      MLN_FEATURE_STATE_SELECTOR_FIELD.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  }
  view.setUint32(layout.fields.fields!.offset, mask, true);
  return storage;
}

function writeExtent(
  native: Native,
  storage: Ptr,
  extent: RenderTargetExtent,
): void {
  const layout = native.layout("mln_render_target_extent");
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  view.setUint32(
    layout.fields.width!.offset,
    asUint32(extent.width, "width"),
    true,
  );
  view.setUint32(
    layout.fields.height!.offset,
    asUint32(extent.height, "height"),
    true,
  );
  view.setFloat64(
    layout.fields.scale_factor!.offset,
    extent.scaleFactor ?? 1,
    true,
  );
}

function writeVulkanContext(
  native: Native,
  storage: Ptr,
  context: VulkanContext,
): void {
  const layout = native.layout("mln_vulkan_context_descriptor");
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  for (const [field, value] of [
    ["instance", context.instance],
    ["physical_device", context.physicalDevice],
    ["device", context.device],
    ["graphics_queue", context.graphicsQueue],
    ["get_instance_proc_addr", context.getInstanceProcAddr],
  ] as const) {
    native.memory.writePointer(
      (storage + BigInt(layout.fields[field]!.offset)) as Ptr,
      value as Ptr,
    );
  }
  view.setUint32(
    layout.fields.graphics_queue_family_index!.offset,
    asUint32(context.graphicsQueueFamilyIndex, "graphicsQueueFamilyIndex"),
    true,
  );
}

/** A double argument crosses in a slot as its bit pattern. */
function f64Bits(value: number): bigint {
  const scratch = new DataView(new ArrayBuffer(8));
  scratch.setFloat64(0, value, true);
  return scratch.getBigUint64(0, true);
}

/** A borrowed Metal context. */
export interface MetalContext {
  readonly device: NativePointer;
}

/** A borrowed WebGPU context. */
export interface WebGpuContext {
  /** Optional for texture sessions. */
  readonly instance?: NativePointer;
  readonly device: NativePointer;
  readonly queue?: NativePointer;
}

/**
 * A borrowed OpenGL context, named by the provider that made it current.
 *
 * The C API takes one union, and which arm it reads follows the platform, so
 * the public value is tagged rather than leaving a caller to fill the right
 * fields by convention.
 */
export type OpenGlContext =
  | {
      readonly platform: "wgl";
      readonly deviceContext: NativePointer;
      readonly shareContext?: NativePointer;
      readonly getProcAddress?: NativePointer;
    }
  | {
      readonly platform: "egl";
      readonly display: NativePointer;
      readonly config?: NativePointer;
      readonly shareContext?: NativePointer;
      readonly getProcAddress?: NativePointer;
    }
  | {
      /** A WebGL context handle, which Emscripten numbers rather than addresses. */
      readonly platform: "webgl";
      readonly context: number;
    };

/** What a readback would produce, in device pixels. */
export interface TextureImageInfo {
  readonly width: number;
  readonly height: number;
  /** Bytes per image row, which padding may make wider than `width * 4`. */
  readonly stride: number;
  /** How many bytes a buffer needs to hold the image. */
  readonly byteLength: number;
}

/** The graphics handles a held frame exposes, which are the session's. */
export interface OpenGlFrameHandles {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /** The texture object name, borrowed until the frame is released. */
  readonly texture: number;
  readonly target: number;
  readonly internalFormat: number;
  readonly format: number;
  readonly type: number;
}

/**
 * A rendered texture the host holds.
 *
 * The texture belongs to the session and is only borrowed, so it is given back
 * as soon as the host has read or drawn it. Reading the handles after that
 * fails rather than handing out a name the session may have reused.
 */
export class OpenGlTextureFrame {
  readonly #session: RenderSession;
  readonly #record: Uint8Array;
  readonly #handles: OpenGlFrameHandles;
  #released = false;

  /** @internal Built by the session that acquired it. */
  constructor(
    session: RenderSession,
    record: Uint8Array,
    handles: OpenGlFrameHandles,
  ) {
    this.#session = session;
    this.#record = record;
    this.#handles = handles;
  }

  /** The borrowed graphics handles, while this frame is still held. */
  get handles(): OpenGlFrameHandles {
    if (this.#released) {
      throw new MaplibreError(
        "closedHandle",
        "this texture frame was released, so its handles are no longer valid",
        { operation: "OpenGlTextureFrame.handles" },
      );
    }
    return this.#handles;
  }

  get isReleased(): boolean {
    return this.#released;
  }

  /**
   * Gives the texture back to the session.
   *
   * A release that fails leaves the frame held, because the session still owns
   * a frame it has not taken back; a later release can succeed.
   */
  release(): void {
    if (this.#released) {
      return;
    }
    this.#session.releaseOpenGlFrame(this.#record);
    this.#released = true;
  }
}

/** Reads the metadata a readback filled in. */
function readImageInfo(native: Native, storage: Ptr): TextureImageInfo {
  const layout = native.layout("mln_texture_image_info");
  const view = native.memory.view(storage, layout.size);
  return {
    width: view.getUint32(layout.fields.width!.offset, true),
    height: view.getUint32(layout.fields.height!.offset, true),
    stride: view.getUint32(layout.fields.stride!.offset, true),
    byteLength: native.readSize(
      (storage + BigInt(layout.fields.byte_length!.offset)) as Ptr,
    ),
  };
}

/** A session that presents through a host surface. */
export interface SurfaceDescriptor<Context> {
  readonly extent: RenderTargetExtent;
  readonly context: Context;
  /** The host surface: a CAMetalLayer, VkSurfaceKHR, or platform surface. */
  readonly surface: NativePointer;
}

/** A session that renders into a texture the host owns. */
export interface BorrowedTextureDescriptor<Context> {
  readonly extent: RenderTargetExtent;
  /** Physical image size in device pixels, stated rather than derived. */
  readonly physicalWidth: number;
  readonly physicalHeight: number;
  readonly context: Context;
}

/** A Vulkan image the host owns. */
export interface VulkanBorrowedTexture extends BorrowedTextureDescriptor<VulkanContext> {
  readonly image: NativePointer;
  readonly imageView: NativePointer;
  readonly format: number;
  readonly initialLayout: number;
}

/** A Metal texture the host owns. */
export interface MetalBorrowedTexture extends BorrowedTextureDescriptor<MetalContext> {
  readonly texture: NativePointer;
}

/** An OpenGL texture the host owns. */
export interface OpenGlBorrowedTexture extends BorrowedTextureDescriptor<OpenGlContext> {
  readonly texture: number;
  readonly target: number;
}

/** A WebGPU texture the host owns. */
export interface WebGpuBorrowedTexture extends BorrowedTextureDescriptor<WebGpuContext> {
  readonly texture: NativePointer;
  readonly textureView: NativePointer;
  readonly format: number;
}

/** Writes a Metal context descriptor. */
export function writeMetalContext(
  native: Native,
  storage: Ptr,
  context: MetalContext,
): void {
  const layout = native.layout("mln_metal_context_descriptor");
  native.memory
    .view(storage, layout.size)
    .setUint32(layout.fields.size!.offset, layout.size, true);
  native.memory.writePointer(
    (storage + BigInt(layout.fields.device!.offset)) as Ptr,
    context.device,
  );
}

/** Writes a WebGPU context descriptor. */
export function writeWebGpuContext(
  native: Native,
  storage: Ptr,
  context: WebGpuContext,
): void {
  const layout = native.layout("mln_webgpu_context_descriptor");
  native.memory
    .view(storage, layout.size)
    .setUint32(layout.fields.size!.offset, layout.size, true);
  for (const [field, value] of [
    ["instance", context.instance],
    ["device", context.device],
    ["queue", context.queue],
  ] as const) {
    if (value !== undefined) {
      native.memory.writePointer(
        (storage + BigInt(layout.fields[field]!.offset)) as Ptr,
        value,
      );
    }
  }
}

/** Writes an OpenGL context descriptor, filling the arm its platform names. */
export function writeOpenGlContext(
  native: Native,
  storage: Ptr,
  context: OpenGlContext,
): void {
  const layout = native.layout("mln_opengl_context_descriptor");
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;

  const platform = {
    wgl: MLN_OPENGL_CONTEXT_PLATFORM.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
    egl: MLN_OPENGL_CONTEXT_PLATFORM.MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    webgl: MLN_OPENGL_CONTEXT_PLATFORM.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL,
  }[context.platform];
  view.setUint32(layout.fields.platform!.offset, platform, true);

  const writeArm = (
    record: string,
    fields: readonly (readonly [string, NativePointer | undefined])[],
  ): void => {
    const arm = native.layout(record);
    native.memory
      .view(data, arm.size)
      .setUint32(arm.fields.size!.offset, arm.size, true);
    for (const [field, value] of fields) {
      if (value !== undefined) {
        native.memory.writePointer(
          (data + BigInt(arm.fields[field]!.offset)) as Ptr,
          value,
        );
      }
    }
  };

  switch (context.platform) {
    case "wgl":
      writeArm("mln_wgl_context_descriptor", [
        ["device_context", context.deviceContext],
        ["share_context", context.shareContext],
        ["get_proc_address", context.getProcAddress],
      ]);
      return;
    case "egl":
      writeArm("mln_egl_context_descriptor", [
        ["display", context.display],
        ["config", context.config],
        ["share_context", context.shareContext],
        ["get_proc_address", context.getProcAddress],
      ]);
      return;
    case "webgl": {
      const arm = native.layout("mln_webgl_context_descriptor");
      const view_ = native.memory.view(data, arm.size);
      view_.setUint32(arm.fields.size!.offset, arm.size, true);
      view_.setInt32(
        arm.fields.context!.offset,
        asInt32(context.context, "a WebGL context handle"),
        true,
      );
      return;
    }
  }
}

/**
 * Attaches a session, whichever backend and target family it is.
 *
 * Every attach takes the same shape — build the descriptor from the C API's
 * default, fill the extent and the borrowed context, then hand it to the
 * backend's entry point — so the families differ only in what they add.
 */
function attach(
  native: Native,
  map: Map,
  operation: string,
  record: string,
  defaultEntrypoint: number,
  attachEntrypoint: number,
  fill: (storage: Ptr, layout: RecordLayout) => void,
): RenderSession {
  const id = native.scope((scope) => {
    const layout = native.layout(record);
    const storage = scope.allocateZeroed(layout.size, layout.align);
    native.structValue(scope, defaultEntrypoint, storage);
    fill(storage, layout);
    const outSession = scope.allocateZeroed(8);
    native.checked(scope, attachEntrypoint, [
      handleStateOf(map).use(operation),
      storage,
      outSession,
    ]);
    return native.memory.view(outSession, 8).getBigUint64(0, true);
  });
  try {
    return RenderSession.own(native, id);
  } catch (error) {
    // The session exists and nothing owns it, so it is released rather than
    // left for a leak report that names no wrapper.
    native.scope((scope) => {
      native.raw(scope, EP.mln_render_session_destroy, [id]);
    });
    throw error;
  }
}

/** Fills the extent every descriptor starts with. */
function fillExtent(
  native: Native,
  storage: Ptr,
  layout: RecordLayout,
  extent: RenderTargetExtent,
): void {
  writeExtent(
    native,
    (storage + BigInt(layout.fields.extent!.offset)) as Ptr,
    extent,
  );
}

/** Fills the physical size a caller-owned texture states separately. */
function fillPhysicalSize(
  native: Native,
  storage: Ptr,
  layout: RecordLayout,
  descriptor: { physicalWidth: number; physicalHeight: number },
): void {
  const view = native.memory.view(storage, layout.size);
  view.setUint32(
    layout.fields.physical_width!.offset,
    asUint32(descriptor.physicalWidth, "physicalWidth"),
    true,
  );
  view.setUint32(
    layout.fields.physical_height!.offset,
    asUint32(descriptor.physicalHeight, "physicalHeight"),
    true,
  );
}

/** Every attach the C API offers, by backend and target family. */
export const ATTACH: {
  metalSurface(
    native: Native,
    map: Map,
    descriptor: SurfaceDescriptor<MetalContext>,
  ): RenderSession;
  vulkanSurface(
    native: Native,
    map: Map,
    descriptor: SurfaceDescriptor<VulkanContext>,
  ): RenderSession;
  openglSurface(
    native: Native,
    map: Map,
    descriptor: SurfaceDescriptor<OpenGlContext>,
  ): RenderSession;
  metalOwnedTexture(
    native: Native,
    map: Map,
    descriptor: { extent: RenderTargetExtent; context: MetalContext },
  ): RenderSession;
  openglOwnedTexture(
    native: Native,
    map: Map,
    descriptor: { extent: RenderTargetExtent; context: OpenGlContext },
  ): RenderSession;
  webgpuOwnedTexture(
    native: Native,
    map: Map,
    descriptor: { extent: RenderTargetExtent; context: WebGpuContext },
  ): RenderSession;
  metalBorrowedTexture(
    native: Native,
    map: Map,
    descriptor: MetalBorrowedTexture,
  ): RenderSession;
  vulkanBorrowedTexture(
    native: Native,
    map: Map,
    descriptor: VulkanBorrowedTexture,
  ): RenderSession;
  openglBorrowedTexture(
    native: Native,
    map: Map,
    descriptor: OpenGlBorrowedTexture,
  ): RenderSession;
  webgpuBorrowedTexture(
    native: Native,
    map: Map,
    descriptor: WebGpuBorrowedTexture,
  ): RenderSession;
} = {
  metalSurface(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachMetalSurface",
      "mln_metal_surface_descriptor",
      EP.mln_metal_surface_descriptor_default,
      EP.mln_metal_surface_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeMetalContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.layer!.offset)) as Ptr,
          descriptor.surface,
        );
      },
    );
  },
  vulkanSurface(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachVulkanSurface",
      "mln_vulkan_surface_descriptor",
      EP.mln_vulkan_surface_descriptor_default,
      EP.mln_vulkan_surface_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeVulkanContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.surface!.offset)) as Ptr,
          descriptor.surface,
        );
      },
    );
  },
  openglSurface(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachOpenGlSurface",
      "mln_opengl_surface_descriptor",
      EP.mln_opengl_surface_descriptor_default,
      EP.mln_opengl_surface_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeOpenGlContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.surface!.offset)) as Ptr,
          descriptor.surface,
        );
      },
    );
  },
  metalOwnedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachMetalOwnedTexture",
      "mln_metal_owned_texture_descriptor",
      EP.mln_metal_owned_texture_descriptor_default,
      EP.mln_metal_owned_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeMetalContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
      },
    );
  },
  openglOwnedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachOpenGlOwnedTexture",
      "mln_opengl_owned_texture_descriptor",
      EP.mln_opengl_owned_texture_descriptor_default,
      EP.mln_opengl_owned_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeOpenGlContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
      },
    );
  },
  webgpuOwnedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachWebGpuOwnedTexture",
      "mln_webgpu_owned_texture_descriptor",
      EP.mln_webgpu_owned_texture_descriptor_default,
      EP.mln_webgpu_owned_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        writeWebGpuContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
      },
    );
  },
  metalBorrowedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachMetalBorrowedTexture",
      "mln_metal_borrowed_texture_descriptor",
      EP.mln_metal_borrowed_texture_descriptor_default,
      EP.mln_metal_borrowed_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        fillPhysicalSize(native, storage, layout, descriptor);
        writeMetalContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.texture!.offset)) as Ptr,
          descriptor.texture,
        );
      },
    );
  },
  vulkanBorrowedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachVulkanBorrowedTexture",
      "mln_vulkan_borrowed_texture_descriptor",
      EP.mln_vulkan_borrowed_texture_descriptor_default,
      EP.mln_vulkan_borrowed_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        fillPhysicalSize(native, storage, layout, descriptor);
        writeVulkanContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.image!.offset)) as Ptr,
          descriptor.image,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.image_view!.offset)) as Ptr,
          descriptor.imageView,
        );
        const view = native.memory.view(storage, layout.size);
        view.setInt32(
          layout.fields.format!.offset,
          asInt32(descriptor.format, "a Vulkan format"),
          true,
        );
        view.setInt32(
          layout.fields.initial_layout!.offset,
          asInt32(descriptor.initialLayout, "a Vulkan image layout"),
          true,
        );
      },
    );
  },
  openglBorrowedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachOpenGlBorrowedTexture",
      "mln_opengl_borrowed_texture_descriptor",
      EP.mln_opengl_borrowed_texture_descriptor_default,
      EP.mln_opengl_borrowed_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        fillPhysicalSize(native, storage, layout, descriptor);
        writeOpenGlContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        const view = native.memory.view(storage, layout.size);
        view.setUint32(
          layout.fields.texture!.offset,
          asUint32(descriptor.texture, "a texture name"),
          true,
        );
        view.setUint32(
          layout.fields.target!.offset,
          asUint32(descriptor.target, "a texture target"),
          true,
        );
      },
    );
  },
  webgpuBorrowedTexture(native, map, descriptor) {
    return attach(
      native,
      map,
      "Map.attachWebGpuBorrowedTexture",
      "mln_webgpu_borrowed_texture_descriptor",
      EP.mln_webgpu_borrowed_texture_descriptor_default,
      EP.mln_webgpu_borrowed_texture_attach,
      (storage, layout) => {
        fillExtent(native, storage, layout, descriptor.extent);
        fillPhysicalSize(native, storage, layout, descriptor);
        writeWebGpuContext(
          native,
          (storage + BigInt(layout.fields.context!.offset)) as Ptr,
          descriptor.context,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.texture!.offset)) as Ptr,
          descriptor.texture,
        );
        native.memory.writePointer(
          (storage + BigInt(layout.fields.texture_view!.offset)) as Ptr,
          descriptor.textureView,
        );
        native.memory
          .view(storage, layout.size)
          .setUint32(
            layout.fields.format!.offset,
            asUint32(descriptor.format, "a WebGPU format"),
            true,
          );
      },
    );
  },
};

/** Writes an `mln_rendered_query_geometry` for a rendered-feature query. */
function writeQueryGeometry(
  native: Native,
  scope: Scope,
  geometry: RenderedQueryGeometry,
): Ptr {
  const layout = native.layout("mln_rendered_query_geometry");
  const storage = scope.allocateZeroed(layout.size, layout.align);
  const point = native.layout("mln_screen_point");
  const data = (storage + BigInt(layout.fields.data!.offset)) as Ptr;
  const view = native.memory.view(storage, layout.size);
  view.setUint32(layout.fields.size!.offset, layout.size, true);

  const writePoint = (at: Ptr, value: ScreenPoint): void => {
    const pointView = native.memory.view(at, point.size);
    pointView.setFloat64(point.fields.x!.offset, value.x, true);
    pointView.setFloat64(point.fields.y!.offset, value.y, true);
  };

  switch (geometry.kind) {
    case "point":
      view.setUint32(
        layout.fields.type!.offset,
        MLN_RENDERED_QUERY_GEOMETRY_TYPE.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
        true,
      );
      writePoint(data, geometry.point);
      return storage;
    case "box": {
      view.setUint32(
        layout.fields.type!.offset,
        MLN_RENDERED_QUERY_GEOMETRY_TYPE.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
        true,
      );
      const box = native.layout("mln_screen_box");
      writePoint((data + BigInt(box.fields.min!.offset)) as Ptr, geometry.min);
      writePoint((data + BigInt(box.fields.max!.offset)) as Ptr, geometry.max);
      return storage;
    }
    case "lineString": {
      view.setUint32(
        layout.fields.type!.offset,
        MLN_RENDERED_QUERY_GEOMETRY_TYPE.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
        true,
      );
      const span = native.layout("mln_screen_point_span");
      const array = scope.allocateZeroed(
        Math.max(point.size * geometry.points.length, 1),
        point.align,
      );
      geometry.points.forEach((value, index) => {
        writePoint((array + BigInt(index * point.size)) as Ptr, value);
      });
      native.memory.writePointer(
        (data + BigInt(span.fields.points!.offset)) as Ptr,
        array,
      );
      writeSize(
        native,
        (data + BigInt(span.fields.point_count!.offset)) as Ptr,
        geometry.points.length,
      );
      return storage;
    }
  }
}

/** Writes a borrowed array of string views, which both query options take. */
function writeStringViews(
  native: Native,
  scope: Scope,
  values: readonly string[],
): Ptr {
  const layout = native.layout("mln_string_view");
  const array = scope.allocateZeroed(
    Math.max(layout.size * values.length, 1),
    layout.align,
  );
  values.forEach((value, index) => {
    writeStringView(
      native,
      scope,
      (array + BigInt(index * layout.size)) as Ptr,
      value,
    );
  });
  return array;
}

/** Copies every feature a query result holds, then releases the result. */
function drainQueryResult(native: Native, result: bigint): QueriedFeature[] {
  try {
    const count = native.scope((scope) => {
      const out = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_feature_query_result_count, [result, out]);
      return native.readSize(out);
    });
    const features: QueriedFeature[] = [];
    for (let index = 0; index < count; index += 1) {
      features.push(
        native.scope((scope) => {
          const layout = native.layout("mln_queried_feature");
          const storage = scope.allocateZeroed(layout.size, layout.align);
          native.memory
            .view(storage, layout.size)
            .setUint32(layout.fields.size!.offset, layout.size, true);
          native.checked(scope, EP.mln_feature_query_result_get, [
            result,
            BigInt(index),
            storage,
          ]);
          return readQueriedFeature(native, storage);
        }),
      );
    }
    return features;
  } finally {
    // The result handle is released whether the copy succeeded or not: it is
    // native storage this call acquired and nothing else can reach.
    native.scope((scope) => {
      native.raw(scope, EP.mln_feature_query_result_destroy, [result]);
    });
  }
}
