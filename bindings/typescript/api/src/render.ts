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
import { HandleState } from "./internal/handle.ts";
import type { Native } from "./internal/native.ts";
import { asUint32 } from "./internal/numbers.ts";
import { attachHandleState, handleStateOf } from "./internal/private.ts";
import type { Ptr } from "./internal/transport.ts";
import type { Map } from "./map.ts";
import { EP } from "./raw/entrypoints.ts";

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

/** A session that renders into a texture the session itself creates. */
export interface VulkanOwnedTextureDescriptor {
  readonly extent: RenderTargetExtent;
  readonly context: VulkanContext;
}

export class RenderSession {
  readonly #state: HandleState;

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
