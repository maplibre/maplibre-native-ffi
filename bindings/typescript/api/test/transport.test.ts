/**
 * Transport-level tests.
 *
 * These call the C API through the raw layer on purpose: the slice under test is
 * the generated dispatch and the memory model, below any public wrapper. Public
 * behavior is tested through the public API in the conformance suite.
 */

import {
  AbiCallError,
  Caller,
  statusFromSlot,
  u32Slot,
} from "../src/internal/call.ts";
import { Memory, MemoryError } from "../src/internal/memory.ts";
import {
  type NodeApiAddon,
  AbiMismatchError,
  nodeApiTransport,
} from "../src/internal/node-transport.ts";
import type { Ptr, Transport } from "../src/internal/transport.ts";
import { AbiCallStatus } from "../src/internal/transport.ts";
import { EP } from "../src/raw/entrypoints.ts";
import { MLN_STATUS } from "../src/raw/enums.ts";
import { ABI_FINGERPRINT } from "../src/raw/fingerprint.ts";
import { LAYOUTS } from "../src/raw/layouts.ts";
import { describe, expect, it } from "vitest";

const { addon } = (await import("@maplibre/native-ffi-runtime-node")) as {
  addon: NodeApiAddon;
};

const transport = nodeApiTransport(addon);
const layouts = LAYOUTS[transport.abi];

function withCaller<T>(
  body: (context: { memory: Memory; caller: Caller }) => T,
): T {
  const memory = new Memory(transport);
  const caller = new Caller(transport, memory);
  return body({ memory, caller });
}

describe("the normalized call ABI", () => {
  it("reports a scalar result", () => {
    withCaller(({ memory, caller }) => {
      const mask = memory.scope((scope) => {
        const result = caller.invoke(
          scope,
          EP.mln_supported_render_backend_mask,
          [],
        );
        return Number(result.raw & 0xffff_ffffn);
      });
      // Every build compiles exactly one renderer, so the mask names one backend.
      expect(mask).toBeGreaterThan(0);
      expect(mask & (mask - 1)).toBe(0);
    });
  });

  it("writes a by-value struct return into caller storage", () => {
    withCaller(({ memory, caller }) => {
      const layout = layouts.mln_runtime_options!;
      const size = memory.scope((scope) => {
        const storage = scope.allocateZeroed(layout.size);
        caller.invoke(scope, EP.mln_runtime_options_default, [], storage);
        return memory
          .view(storage, layout.size)
          .getUint32(layout.fields.size!.offset, true);
      });
      // The C API's `size` field is the struct's own size, so this compares the
      // generated layout against the library that was linked, not against
      // another copy of the same generated table.
      expect(size).toBe(layout.size);
    });
  });

  it("passes a struct argument by value", () => {
    withCaller(({ memory, caller }) => {
      const coordinate = layouts.mln_lat_lng!;
      const meters = layouts.mln_projected_meters!;
      const { status, northing, easting } = memory.scope((scope) => {
        const coordinateStorage = scope.allocateZeroed(coordinate.size);
        const coordinateView = memory.view(coordinateStorage, coordinate.size);
        coordinateView.setFloat64(coordinate.fields.latitude!.offset, 45, true);
        coordinateView.setFloat64(
          coordinate.fields.longitude!.offset,
          -122,
          true,
        );
        const metersStorage = scope.allocateZeroed(meters.size);
        const result = caller.invoke(
          scope,
          EP.mln_projected_meters_for_lat_lng,
          [coordinateStorage, metersStorage],
        );
        const metersView = memory.view(metersStorage, meters.size);
        return {
          status: statusFromSlot(result.raw),
          northing: metersView.getFloat64(meters.fields.northing!.offset, true),
          easting: metersView.getFloat64(meters.fields.easting!.offset, true),
        };
      });
      expect(status).toBe(MLN_STATUS.MLN_STATUS_OK);
      // Web Mercator northing for 45°N, which a lost or zeroed argument cannot
      // produce.
      expect(northing).toBeCloseTo(5_621_521.486, 2);
      expect(easting).toBeCloseTo(-13_580_977.876, 2);
    });
  });

  it("copies the diagnostic of a failing call inside that call", () => {
    withCaller(({ memory, caller }) => {
      const [first, second] = memory.scope((scope) => {
        const failing = caller.invoke(scope, EP.mln_runtime_create, [0n, 0n]);
        const alsoFailing = caller.invoke(scope, EP.mln_network_status_set, [
          u32Slot(9999),
        ]);
        return [failing, alsoFailing];
      });
      expect(statusFromSlot(first.raw)).toBe(
        MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT,
      );
      expect(first.diagnostic).toBe("out_runtime must not be null");
      // The second call's diagnostic is its own, and the first result still
      // carries the message that belonged to it.
      expect(statusFromSlot(second.raw)).toBe(
        MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT,
      );
      expect(second.diagnostic).not.toBe("");
      expect(second.diagnostic).not.toBe(first.diagnostic);
      expect(first.diagnostic).toBe("out_runtime must not be null");
    });
  });

  it("carries handles through out-parameters", () => {
    withCaller(({ memory, caller }) => {
      const { createStatus, handle, destroyStatus, secondDestroyStatus } =
        memory.scope((scope) => {
          const layout = layouts.mln_runtime_options!;
          const options = scope.allocateZeroed(layout.size);
          caller.invoke(scope, EP.mln_runtime_options_default, [], options);
          const outRuntime = scope.allocateZeroed(8);
          const created = caller.invoke(scope, EP.mln_runtime_create, [
            options,
            outRuntime,
          ]);
          const runtime = memory.view(outRuntime, 8).getBigUint64(0, true);
          const destroyed = caller.invoke(scope, EP.mln_runtime_destroy, [
            runtime,
          ]);
          const destroyedAgain = caller.invoke(scope, EP.mln_runtime_destroy, [
            runtime,
          ]);
          return {
            createStatus: statusFromSlot(created.raw),
            handle: runtime,
            destroyStatus: statusFromSlot(destroyed.raw),
            secondDestroyStatus: statusFromSlot(destroyedAgain.raw),
          };
        });
      expect(createStatus).toBe(MLN_STATUS.MLN_STATUS_OK);
      // A live handle carries its kind tag in the top byte, so it cannot be a
      // JavaScript number and cannot be zero.
      expect(handle).toBeGreaterThan(2n ** 56n);
      expect(destroyStatus).toBe(MLN_STATUS.MLN_STATUS_OK);
      expect(secondDestroyStatus).toBe(MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT);
    });
  });

  it("rejects malformed dispatch input", () => {
    withCaller(({ memory, caller }) => {
      memory.scope((scope) => {
        expect(() => caller.invoke(scope, 0xffff, [])).toThrow(AbiCallError);
        const slots = scope.allocateZeroed(64);
        expect(
          transport.call(EP.mln_c_version, 0n as Ptr, slots, 0, slots),
        ).toBe(AbiCallStatus.nullSlots);
        expect(
          transport.call(
            EP.mln_c_version,
            (slots + 1n) as Ptr,
            slots,
            0,
            slots,
          ),
        ).toBe(AbiCallStatus.misalignedSlots);
        expect(transport.call(0xffff, slots, slots, 0, slots)).toBe(
          AbiCallStatus.unknownEntrypoint,
        );
      });
    });
  });

  it("reports stable entrypoint addresses and names", () => {
    const callback = transport.symbol(EP.mln_adapter_log_callback);
    expect(callback).toBeGreaterThan(0n);
    expect(transport.symbol(EP.mln_adapter_log_callback)).toBe(callback);
    expect(
      transport.symbol(EP.mln_adapter_resource_provider_rules_callback),
    ).not.toBe(callback);
    expect(transport.symbol(0xffff)).toBe(0n);
    expect(transport.entrypointName(EP.mln_c_version)).toBe("mln_c_version");
    expect(transport.entrypointName(0xffff)).toBeNull();
  });

  it("copies library-owned memory that sits outside every slab", () => {
    withCaller(({ memory, caller }) => {
      const { message, bytes, foreign } = memory.scope((scope) => {
        caller.invoke(scope, EP.mln_runtime_create, [0n, 0n]);
        const pointerSlot = caller.invoke(
          scope,
          EP.mln_thread_last_error_message,
          [],
        );
        const address = pointerSlot.raw as Ptr;
        return {
          foreign: address,
          message: transport.readForeignCString(address),
          bytes: transport.readForeign(address, 3),
        };
      });
      expect(memory.owns(foreign)).toBe(false);
      expect(message).toBe("out_runtime must not be null");
      expect([...bytes]).toEqual([0x6f, 0x75, 0x74]);
    });
  });
});

describe("binding-owned memory", () => {
  it("honours alignment and reuses freed blocks", () => {
    const memory = new Memory(transport);
    const first = memory.allocate(24, 16);
    expect(first % 16n).toBe(0n);
    const second = memory.allocate(24, 16);
    expect(second).not.toBe(first);
    memory.free(first);
    // A freed block is available again, so a long-lived transport does not grow
    // a slab per call.
    expect(memory.allocate(24, 16)).toBe(first);
    expect(() => memory.free(first + 1n)).toThrow(MemoryError);
  });

  it("releases everything a scope allocated", () => {
    const memory = new Memory(transport);
    const inside = memory.scope((scope) => scope.allocate(64));
    expect(memory.scope((scope) => scope.allocate(64))).toBe(inside);
  });

  it("keeps views inside their slab", () => {
    const memory = new Memory(transport);
    const address = memory.allocate(32);
    expect(memory.view(address, 32).byteLength).toBe(32);
    expect(() => memory.view(address, 1 << 30)).toThrow(MemoryError);
    expect(memory.owns(address)).toBe(true);
    expect(memory.owns(1n as Ptr)).toBe(false);
  });

  it("round-trips a pointer field at the transport's width", () => {
    const memory = new Memory(transport);
    const cell = memory.allocate(8);
    // A tagged Android heap pointer is the case a number cannot carry: its top
    // byte is significant and it exceeds 2^53.
    const tagged = 0xb4_00_7f_12_34_56_78_90n as Ptr;
    memory.writePointer(cell, tagged);
    expect(memory.readPointer(cell)).toBe(tagged);
  });
});

describe("the ABI handshake", () => {
  it("refuses a payload whose fingerprint differs", () => {
    const doctored: NodeApiAddon = {
      ...addon,
      abiFingerprint: () => "0".repeat(64),
    };
    expect(() => nodeApiTransport(doctored)).toThrow(AbiMismatchError);
  });

  it("refuses a payload built against other headers", () => {
    const doctored: NodeApiAddon = {
      ...addon,
      abiHeaderDigest: () => "0".repeat(64),
    };
    expect(() => nodeApiTransport(doctored)).toThrow(AbiMismatchError);
  });

  it("agrees with the payload it loaded", () => {
    expect(transport.abiFingerprint).toBe(ABI_FINGERPRINT);
    expect(addon.entrypointCount()).toBe(Object.keys(EP).length);
  });
});

// Referenced so the unused-import rule sees the shared type alias in use.
export type { Transport };
