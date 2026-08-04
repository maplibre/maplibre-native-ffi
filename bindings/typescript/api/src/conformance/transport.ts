/**
 * The normalized call ABI and the memory model beneath the public API.
 *
 * These reach the transport directly, which the binding specification allows
 * for bindability and layout tests. They matter most on more than one
 * transport: the Node-API addon and the WebAssembly module implement the same
 * rules over different memory, so a rule that holds on one and not the other is
 * exactly what this proves.
 */

import {
  AbiCallError,
  Caller,
  statusFromSlot,
  u32Slot,
} from "../internal/call.ts";
import {
  expectedPayloadIdentity,
  verifyPayload,
} from "../internal/handshake.ts";
import { Memory, MemoryError } from "../internal/memory.ts";
import { AbiCallStatus, type Ptr } from "../internal/transport.ts";
import { EP } from "../raw/entrypoints.ts";
import { MLN_STATUS } from "../raw/enums.ts";
import { LAYOUTS } from "../raw/layouts.ts";
import type { CaseContext, ConformanceGroup } from "./harness.ts";
import { transportOf } from "./harness.ts";

/** Runs a body with memory and a caller of its own. */
function withCaller<T>(
  context: CaseContext,
  body: (context: { memory: Memory; caller: Caller }) => T,
): T {
  const transport = transportOf(context.maplibre);
  const memory = new Memory(transport);
  return body({ memory, caller: new Caller(transport, memory) });
}

export const CALL_ABI_GROUP: ConformanceGroup = {
  name: "the normalized call ABI",
  cases: [
    {
      name: "reports a scalar result",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const mask = memory.scope((scope) =>
            Number(
              caller.invoke(scope, EP.mln_supported_render_backend_mask, [])
                .raw & 0xffff_ffffn,
            ),
          );
          // Every build compiles exactly one renderer, so the mask names one.
          context.expect.ok(mask > 0, "a backend is reported");
          context.expect.equal(mask & (mask - 1), 0, "exactly one backend");
        });
      },
    },
    {
      name: "writes a by-value struct return into caller storage",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const layout =
            LAYOUTS[transportOf(context.maplibre).abi].mln_runtime_options!;
          const size = memory.scope((scope) => {
            const storage = scope.allocateZeroed(layout.size);
            caller.invoke(scope, EP.mln_runtime_options_default, [], storage);
            return memory
              .view(storage, layout.size)
              .getUint32(layout.fields.size!.offset, true);
          });
          // The C API's `size` field is the struct's own size, so this compares
          // the generated layout against the library that was linked.
          context.expect.equal(size, layout.size, "the reported struct size");
        });
      },
    },
    {
      name: "passes a struct argument by value",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const layouts = LAYOUTS[transportOf(context.maplibre).abi];
          const coordinate = layouts.mln_lat_lng!;
          const meters = layouts.mln_projected_meters!;
          const northing = memory.scope((scope) => {
            const input = scope.allocateZeroed(coordinate.size);
            const view = memory.view(input, coordinate.size);
            view.setFloat64(coordinate.fields.latitude!.offset, 45, true);
            view.setFloat64(coordinate.fields.longitude!.offset, -122, true);
            const output = scope.allocateZeroed(meters.size);
            caller.invoke(scope, EP.mln_projected_meters_for_lat_lng, [
              input,
              output,
            ]);
            return memory
              .view(output, meters.size)
              .getFloat64(meters.fields.northing!.offset, true);
          });
          // Web Mercator northing for 45°N, which a lost or zeroed argument
          // cannot produce.
          context.expect.closeTo(northing, 5_621_521.486, 2, "the northing");
        });
      },
    },
    {
      name: "copies the diagnostic of a failing call inside that call",
      spec: ["BND-022"],
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const [first, second] = memory.scope((scope) => [
            caller.invoke(scope, EP.mln_runtime_create, [0n, 0n]),
            caller.invoke(scope, EP.mln_network_status_set, [u32Slot(9999)]),
          ]);
          context.expect.equal(
            statusFromSlot(first!.raw),
            MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT,
            "the first status",
          );
          context.expect.equal(
            first!.diagnostic,
            "out_runtime must not be null",
            "the first diagnostic",
          );
          // The second call sets its own thread-local diagnostic, and the first
          // result still carries the message that belonged to it.
          context.expect.notEqual(
            second!.diagnostic,
            first!.diagnostic,
            "the second diagnostic",
          );
          context.expect.equal(
            first!.diagnostic,
            "out_runtime must not be null",
            "the first diagnostic again",
          );
        });
      },
    },
    {
      name: "carries handles through out-parameters",
      spec: ["BND-040"],
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const layout =
            LAYOUTS[transportOf(context.maplibre).abi].mln_runtime_options!;
          const result = memory.scope((scope) => {
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
            const again = caller.invoke(scope, EP.mln_runtime_destroy, [
              runtime,
            ]);
            return {
              created: statusFromSlot(created.raw),
              runtime,
              destroyed: statusFromSlot(destroyed.raw),
              again: statusFromSlot(again.raw),
            };
          });
          context.expect.equal(
            result.created,
            MLN_STATUS.MLN_STATUS_OK,
            "create",
          );
          // A live handle carries its kind tag in the top byte, so it cannot be
          // a JavaScript number and cannot be zero.
          context.expect.ok(result.runtime > 2n ** 56n, "a tagged handle id");
          context.expect.equal(
            result.destroyed,
            MLN_STATUS.MLN_STATUS_OK,
            "destroy",
          );
          context.expect.equal(
            result.again,
            MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT,
            "a released id is rejected",
          );
        });
      },
    },
    {
      name: "rejects malformed dispatch input",
      run(context) {
        const { expect } = context;
        const transport = transportOf(context.maplibre);
        withCaller(context, ({ memory, caller }) => {
          memory.scope((scope) => {
            expect.throwsAny(
              () => caller.invoke(scope, 0xffff, []),
              "an entrypoint id outside the table",
            );
            const slots = scope.allocateZeroed(64);
            expect.equal(
              transport.call(EP.mln_c_version, 0n as Ptr, slots, 0, slots),
              AbiCallStatus.nullSlots,
              "a null slot array",
            );
            expect.equal(
              transport.call(
                EP.mln_c_version,
                (slots + 1n) as Ptr,
                slots,
                0,
                slots,
              ),
              AbiCallStatus.misalignedSlots,
              "a misaligned slot array",
            );
            expect.equal(
              transport.call(0xffff, slots, slots, 0, slots),
              AbiCallStatus.unknownEntrypoint,
              "an unknown entrypoint",
            );
          });
        });
      },
    },
    {
      name: "refuses a call whose caller storage is absent or too small",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          memory.scope((scope) => {
            context.expect.throwsAny(
              () => caller.invoke(scope, EP.mln_runtime_options_default, []),
              "a struct return with no storage",
            );
            const tooSmall = scope.allocateZeroed(4);
            context.expect.throwsAny(
              () =>
                caller.invoke(
                  scope,
                  EP.mln_runtime_options_default,
                  [],
                  tooSmall,
                ),
              "a struct return with too little storage",
            );
          });
        });
      },
    },
    {
      name: "agrees with the linked library about every self-describing struct",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const layouts = LAYOUTS[transportOf(context.maplibre).abi];
          // Each of these C structs carries its own size, so the library
          // reports what the generated table claims and a wrong offset cannot
          // hide.
          const defaults = [
            ["mln_runtime_options", EP.mln_runtime_options_default],
            ["mln_map_options", EP.mln_map_options_default],
            ["mln_camera_options", EP.mln_camera_options_default],
            ["mln_animation_options", EP.mln_animation_options_default],
            ["mln_bound_options", EP.mln_bound_options_default],
            ["mln_map_tile_options", EP.mln_map_tile_options_default],
            ["mln_style_image_info", EP.mln_style_image_info_default],
          ] as const;
          for (const [record, entrypoint] of defaults) {
            const layout = layouts[record]!;
            const reported = memory.scope((scope) => {
              const storage = scope.allocateZeroed(layout.size);
              caller.invoke(scope, entrypoint, [], storage);
              return memory
                .view(storage, layout.size)
                .getUint32(layout.fields.size!.offset, true);
            });
            context.expect.equal(reported, layout.size, `${record} size`);
          }
        });
      },
    },
    {
      name: "reports stable entrypoint addresses and names",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const callback = transport.symbol(EP.mln_adapter_log_callback);
        expect.ok(callback > 0n, "an entrypoint address");
        expect.equal(
          transport.symbol(EP.mln_adapter_log_callback),
          callback,
          "the same address twice",
        );
        expect.notEqual(
          transport.symbol(EP.mln_adapter_resource_provider_rules_callback),
          callback,
          "a different entrypoint",
        );
        expect.equal(transport.symbol(0xffff), 0n, "an id outside the table");
        expect.equal(
          transport.entrypointName(EP.mln_c_version),
          "mln_c_version",
          "an entrypoint's name",
        );
        expect.absent(
          transport.entrypointName(0xffff) ?? undefined,
          "no name outside the table",
        );
      },
    },
    {
      name: "copies library-owned memory that sits outside every slab",
      run(context) {
        withCaller(context, ({ memory, caller }) => {
          const { expect } = context;
          const transport = transportOf(context.maplibre);
          const foreign = memory.scope((scope) => {
            caller.invoke(scope, EP.mln_runtime_create, [0n, 0n]);
            return caller.invoke(scope, EP.mln_thread_last_error_message, [])
              .raw as Ptr;
          });
          expect.ok(!memory.owns(foreign), "the pointer is outside the slabs");
          expect.equal(
            transport.readForeignCString(foreign),
            "out_runtime must not be null",
            "the copied string",
          );
          expect.equal(
            [...transport.readForeign(foreign, 3)].join(","),
            "111,117,116",
            "the copied bytes",
          );
        });
      },
    },
  ],
};

export const MEMORY_GROUP: ConformanceGroup = {
  name: "binding-owned memory",
  cases: [
    {
      name: "honours alignment and reuses freed blocks",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const first = memory.allocate(24, 16);
        expect.equal(first % 16n, 0n, "an aligned address");
        const second = memory.allocate(24, 16);
        expect.notEqual(second, first, "a distinct second block");
        memory.free(first);
        // A freed block is available again, so a long-lived transport does not
        // grow a slab per call.
        expect.equal(memory.allocate(24, 16), first, "the block was reused");
        expect.throwsAny(
          () => memory.free((first + 1n) as Ptr),
          "freeing an address that names no allocation",
        );
      },
    },
    {
      name: "releases everything a scope allocated",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const inside = memory.scope((scope) => scope.allocate(64));
        expect.equal(
          memory.scope((scope) => scope.allocate(64)),
          inside,
          "the scope released its allocation",
        );
      },
    },
    {
      name: "keeps views inside their slab",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const address = memory.allocate(32);
        expect.equal(memory.view(address, 32).byteLength, 32, "a whole view");
        expect.throwsAny(
          () => memory.view(address, 1 << 30),
          "a view that leaves its slab",
        );
        expect.ok(memory.owns(address), "the address is owned");
        expect.ok(!memory.owns(1n as Ptr), "and a stray address is not");
      },
    },
    {
      name: "round-trips a pointer field at the transport's width",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const cell = memory.allocate(8);
        // A tagged Android heap pointer is the value a number could not carry:
        // its top byte is significant and it exceeds 2^53. A 32-bit transport
        // cannot hold one, and says so rather than truncating.
        const tagged = 0xb4_00_7f_12_34_56_78_90n as Ptr;
        if (transport.pointerSize === 8) {
          memory.writePointer(cell, tagged);
          expect.equal(memory.readPointer(cell), tagged, "the tagged pointer");
        } else {
          expect.throwsAny(
            () => memory.writePointer(cell, tagged),
            "a 64-bit pointer on a 32-bit transport",
          );
          const narrow = 0xdead_beefn as Ptr;
          memory.writePointer(cell, narrow);
          expect.equal(memory.readPointer(cell), narrow, "a 32-bit pointer");
        }
      },
    },
    {
      name: "adds a slab for an oversized request and retires it when it empties",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const baseline = memory.allocate(8);
        const large = memory.allocate(1 << 20);
        expect.ok(memory.owns(large), "the large block was allocated");
        memory.free(large);
        // The slab that carried it is gone, so a burst of large temporaries
        // does not become permanent memory.
        expect.ok(!memory.owns(large), "the slab retired");
        // The first slab stays, because a binding that allocates and frees in a
        // loop would otherwise ask the transport for one every time.
        expect.ok(memory.owns(baseline), "the first slab stays");
        memory.free(baseline);
        expect.ok(memory.owns(baseline), "and stays after it empties");
      },
    },
    {
      name: "coalesces neighbouring free blocks",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const memory = new Memory(transport);
        const first = memory.allocate(64);
        const second = memory.allocate(64);
        const third = memory.allocate(64);
        memory.free(first);
        memory.free(third);
        memory.free(second);
        // The three became one, so a request none could satisfy alone now fits.
        expect.equal(memory.allocate(192), first, "the blocks coalesced");
      },
    },
  ],
};

export const TRANSFER_GROUP: ConformanceGroup = {
  name: "handle transfer",
  cases: [
    {
      name: "claims a token once, whatever a host does with the carrier",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        // A tagged Android-style pointer is the value a number could not carry,
        // so the token path is exercised with one rather than a small integer.
        const handle = 0xb4_00_7f_12_34_56_78_90n;
        const token = transport.transferIssue(handle);
        expect.ok(token > 0n, "a token was issued");
        expect.equal(transport.transferClaim(token), handle, "the first claim");
        expect.equal(transport.transferClaim(token), 0n, "the second claim");
        expect.equal(transport.transferDiscard(token), 0n, "a spent token");
      },
    },
    {
      name: "reports exhaustion rather than losing a handle",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        const tokens: bigint[] = [];
        // The table is fixed, so issuing beyond it has to fail rather than
        // evict something already outstanding.
        for (let attempt = 0; attempt < 256; attempt += 1) {
          const token = transport.transferIssue(BigInt(attempt + 1));
          if (token === 0n) {
            break;
          }
          tokens.push(token);
        }
        expect.ok(tokens.length > 0, "tokens were issued");
        expect.equal(transport.transferIssue(1n), 0n, "the table is full");
        for (const [index, token] of tokens.entries()) {
          expect.equal(
            transport.transferDiscard(token),
            BigInt(index + 1),
            "each handle came back",
          );
        }
        const token = transport.transferIssue(7n);
        expect.ok(token > 0n, "the table is usable again");
        expect.equal(transport.transferClaim(token), 7n, "the handle");
      },
    },
    {
      name: "refuses to issue a token for the null handle",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        expect.equal(transport.transferIssue(0n), 0n, "no token for null");
      },
    },
  ],
};

/** Referenced so the error type stays part of this module's contract. */
export type { AbiCallError, MemoryError };

export const HANDSHAKE_GROUP: ConformanceGroup = {
  name: "the payload handshake",
  cases: [
    {
      name: "refuses a payload whose fingerprint differs",
      spec: ["BND-001"],
      run({ expect }) {
        const expected = expectedPayloadIdentity();
        expect.throwsAny(
          () => verifyPayload({ ...expected, fingerprint: "0".repeat(64) }),
          "a payload generated from another schema",
        );
      },
    },
    {
      name: "refuses a payload built against other headers",
      spec: ["BND-001"],
      run({ expect }) {
        const expected = expectedPayloadIdentity();
        expect.throwsAny(
          () => verifyPayload({ ...expected, headerDigest: "0".repeat(64) }),
          "a payload built against other headers",
        );
      },
    },
    {
      name: "accepts the payload it is running over",
      run({ maplibre, expect }) {
        const transport = transportOf(maplibre);
        // The loaded transport reports what it was built from, and the same
        // rule that would refuse a mismatch accepts this one.
        verifyPayload({
          fingerprint: transport.abiFingerprint,
          headerDigest: transport.headerDigest,
        });
        expect.notEqual(transport.abiFingerprint, "", "a reported fingerprint");
      },
    },
  ],
};

export const PACKAGING_GROUP: ConformanceGroup = {
  name: "the built package",
  cases: [
    {
      name: "drives the native library through either module format",
      needs: ["packageResolution"],
      run: async ({ expect, loadPackage }) => {
        for (const format of ["esm", "cjs"] as const) {
          const api = await loadPackage(format);
          const maplibre = await api.Maplibre.load();
          const runtime = maplibre.createRuntime();
          try {
            const map = runtime.createMap({ width: 64, height: 48 });
            expect.equal(map.getSize().width, 64, `${format} drives a map`);
            map.close();
          } finally {
            runtime.close();
          }
          // The two builds are one implementation, so an error from either is
          // the same class to a consumer that catches it.
          expect.equal(
            api.MaplibreError.name,
            "MaplibreError",
            `${format} errors`,
          );
        }
      },
    },
  ],
};
