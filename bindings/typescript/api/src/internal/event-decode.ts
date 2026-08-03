/**
 * Copies a polled event out of native storage.
 *
 * The C API's event struct borrows its message and payload from storage the next
 * poll invalidates, so everything is copied before this function returns. A
 * payload whose native size is smaller than the layout this build expects is
 * kept as raw bytes rather than read past its end.
 */

import {
  CameraChangeMode,
  MapIdentity,
  RenderMode,
  type RuntimeEvent,
  type RuntimeEventPayload,
  RuntimeEventSourceType,
  RuntimeEventType,
  TileOperation,
} from "../events.ts";
import {
  MLN_RUNTIME_EVENT_PAYLOAD_TYPE,
  MLN_RUNTIME_EVENT_SOURCE_TYPE,
} from "../raw/enums.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

export function decodeEvent(
  native: Native,
  event: Ptr,
  resolveMap: (id: bigint) => object | undefined,
): RuntimeEvent {
  const layout = native.layout("mln_runtime_event");
  const view = native.memory.view(event, layout.size);
  const fields = layout.fields;

  const sourceType = RuntimeEventSourceType.fromRawValue(
    view.getUint32(fields.source_type!.offset, true),
  );
  const source = view.getBigUint64(fields.source!.offset, true);
  const isMapSource =
    sourceType.rawValue ===
      MLN_RUNTIME_EVENT_SOURCE_TYPE.MLN_RUNTIME_EVENT_SOURCE_MAP &&
    source !== 0n;
  const messagePointer = native.memory.readPointer(
    (event + BigInt(fields.message!.offset)) as Ptr,
  );
  const messageSize = native.readSize(
    (event + BigInt(fields.message_size!.offset)) as Ptr,
  );
  const payloadPointer = native.memory.readPointer(
    (event + BigInt(fields.payload!.offset)) as Ptr,
  );
  const payloadSize = native.readSize(
    (event + BigInt(fields.payload_size!.offset)) as Ptr,
  );

  return {
    type: RuntimeEventType.fromRawValue(
      view.getUint32(fields.type!.offset, true),
    ),
    sourceType,
    source: isMapSource ? new MapIdentity(source) : undefined,
    // A map an event names may already be closed, in which case the event
    // carries its identity and no wrapper rather than a stale one.
    map: isMapSource ? (resolveMap(source) as RuntimeEvent["map"]) : undefined,
    code: view.getInt32(fields.code!.offset, true),
    message: native.foreignString(messagePointer, messageSize),
    payload: decodePayload(
      native,
      view.getUint32(fields.payload_type!.offset, true),
      payloadPointer,
      payloadSize,
    ),
  };
}

function decodePayload(
  native: Native,
  payloadType: number,
  pointer: Ptr,
  size: number,
): RuntimeEventPayload {
  if (
    payloadType ===
    MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_NONE
  ) {
    return { kind: "none" };
  }
  if (pointer === 0n || size === 0) {
    return { kind: "unknown", rawType: payloadType, bytes: new Uint8Array(0) };
  }

  const decoder = DECODERS[payloadType];
  if (decoder === undefined) {
    return {
      kind: "unknown",
      rawType: payloadType,
      bytes: native.foreignBytes(pointer, size),
    };
  }
  const layout = native.layout(decoder.record);
  if (size < layout.size) {
    // A future library can shrink or replace a payload. Reading the fields this
    // build expects would read past what it sent.
    return {
      kind: "unknown",
      rawType: payloadType,
      bytes: native.foreignBytes(pointer, size),
    };
  }
  const bytes = native.foreignBytes(pointer, layout.size);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return decoder.decode(native, view);
}

interface PayloadDecoder {
  readonly record: string;
  decode(native: Native, view: DataView): RuntimeEventPayload;
}

const DECODERS: Readonly<Record<number, PayloadDecoder>> = {
  [MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME]: {
    record: "mln_runtime_event_render_frame",
    decode(native, view) {
      const fields = native.layout("mln_runtime_event_render_frame").fields;
      const stats = native.layout("mln_rendering_stats").fields;
      const statsOffset = fields.stats!.offset;
      return {
        kind: "renderFrame",
        mode: RenderMode.fromRawValue(
          view.getUint32(fields.mode!.offset, true),
        ),
        needsRepaint: view.getUint8(fields.needs_repaint!.offset) !== 0,
        placementChanged: view.getUint8(fields.placement_changed!.offset) !== 0,
        stats: {
          encodingTime: view.getFloat64(
            statsOffset + stats.encoding_time!.offset,
            true,
          ),
          renderingTime: view.getFloat64(
            statsOffset + stats.rendering_time!.offset,
            true,
          ),
          frameCount: view.getBigInt64(
            statsOffset + stats.frame_count!.offset,
            true,
          ),
          drawCallCount: view.getBigInt64(
            statsOffset + stats.draw_call_count!.offset,
            true,
          ),
          totalDrawCallCount: view.getBigInt64(
            statsOffset + stats.total_draw_call_count!.offset,
            true,
          ),
        },
      };
    },
  },
  [MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP]: {
    record: "mln_runtime_event_render_map",
    decode(native, view) {
      const fields = native.layout("mln_runtime_event_render_map").fields;
      return {
        kind: "renderMap",
        mode: RenderMode.fromRawValue(
          view.getUint32(fields.mode!.offset, true),
        ),
      };
    },
  },
  [MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING]:
    {
      record: "mln_runtime_event_style_image_missing",
      decode(native, view) {
        const fields = native.layout(
          "mln_runtime_event_style_image_missing",
        ).fields;
        return {
          kind: "styleImageMissing",
          imageId: readForeignStringField(
            native,
            view,
            fields.image_id!.offset,
            fields.image_id_size!.offset,
          ),
        };
      },
    },
  [MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION]: {
    record: "mln_runtime_event_tile_action",
    decode(native, view) {
      const fields = native.layout("mln_runtime_event_tile_action").fields;
      const tile = native.layout("mln_tile_id").fields;
      const tileOffset = fields.tile_id!.offset;
      return {
        kind: "tileAction",
        operation: TileOperation.fromRawValue(
          view.getUint32(fields.operation!.offset, true),
        ),
        tileId: {
          overscaledZ: view.getUint32(
            tileOffset + tile.overscaled_z!.offset,
            true,
          ),
          wrap: view.getInt32(tileOffset + tile.wrap!.offset, true),
          canonicalZ: view.getUint32(
            tileOffset + tile.canonical_z!.offset,
            true,
          ),
          canonicalX: view.getUint32(
            tileOffset + tile.canonical_x!.offset,
            true,
          ),
          canonicalY: view.getUint32(
            tileOffset + tile.canonical_y!.offset,
            true,
          ),
        },
        sourceId: readForeignStringField(
          native,
          view,
          fields.source_id!.offset,
          fields.source_id_size!.offset,
        ),
      };
    },
  },
  [MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED]:
    {
      record: "mln_runtime_event_camera_transition_finished",
      decode(native, view) {
        const fields = native.layout(
          "mln_runtime_event_camera_transition_finished",
        ).fields;
        return {
          kind: "cameraTransitionFinished",
          transitionId: view.getBigUint64(fields.transition_id!.offset, true),
        };
      },
    },
};

/**
 * Reads a borrowed string a payload points at.
 *
 * The payload has already been copied, but its interior pointers still name
 * library-owned storage, so the text itself is copied here.
 */
function readForeignStringField(
  native: Native,
  view: DataView,
  pointerOffset: number,
  sizeOffset: number,
): string {
  const pointerSize = native.transport.pointerSize;
  const pointer =
    pointerSize === 8
      ? view.getBigUint64(pointerOffset, true)
      : BigInt(view.getUint32(pointerOffset, true));
  const length =
    pointerSize === 8
      ? Number(view.getBigUint64(sizeOffset, true))
      : view.getUint32(sizeOffset, true);
  return native.foreignString(pointer as Ptr, length);
}

/** Exposed so a test can reach the same conversion the poll path uses. */
export { CameraChangeMode };
