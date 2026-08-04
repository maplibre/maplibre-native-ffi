/**
 * The loaded library, runtimes, wake sources, and event polling.
 */

import { errorKindForStatus, MaplibreError } from "../errors.ts";
import { RuntimeEventType } from "../events.ts";
import { decodePayload } from "../internal/event-decode.ts";
import { clearForcedStatuses, forceStatus } from "../internal/faults.ts";
import { handleStateOf, nativeOf } from "../internal/private.ts";
import { NetworkStatus } from "../maplibre.ts";
import { EP } from "../raw/entrypoints.ts";
import { MLN_RUNTIME_EVENT_PAYLOAD_TYPE } from "../raw/enums.ts";
import type { ConformanceGroup } from "./harness.ts";
import { EMPTY_STYLE, pumpFor, withRuntime } from "./harness.ts";

export const LIBRARY_GROUP: ConformanceGroup = {
  name: "the loaded library",
  cases: [
    {
      name: "reports the ABI version and one compiled render backend",
      spec: ["BND-001"],
      run({ maplibre, expect }) {
        expect.equal(maplibre.cVersion, 0, "the C ABI contract version");
        const enabled = Object.values(maplibre.renderBackends).filter(Boolean);
        // MapLibre Native compiles exactly one renderer per build.
        expect.equal(enabled.length, 1, "compiled render backends");
      },
    },
    {
      name: "round-trips the network status",
      run({ maplibre, expect }) {
        expect.ok(
          maplibre.getNetworkStatus().equals(NetworkStatus.online),
          "a runtime starts online",
        );
        maplibre.setNetworkStatus(NetworkStatus.offline);
        expect.ok(
          maplibre.getNetworkStatus().equals(NetworkStatus.offline),
          "the status that was set",
        );
        maplibre.setNetworkStatus(NetworkStatus.online);
      },
    },
    {
      name: "reports an unknown network status by its raw value",
      spec: ["BND-062", "BND-068"],
      run({ maplibre, expect }) {
        const unknown = NetworkStatus.fromRawValue(4242);
        expect.equal(unknown.rawValue, 4242, "the preserved raw value");
        expect.equal(unknown.name, "unknown(4242)", "the unknown value's name");
        // The C API rejects it and reports its own diagnostic; the binding does
        // not duplicate native enum validation.
        const error = expect.throws(
          () => maplibre.setNetworkStatus(unknown),
          "an unknown status reaches C",
        );
        expect.equal(error.kind, "invalidArgument", "the error kind");
        expect.notEqual(error.diagnostic, "", "the native diagnostic");
        expect.equal(
          error.operation,
          "mln_network_status_set",
          "the failing operation",
        );
      },
    },
    {
      name: "copies a diagnostic that a later call cannot replace",
      spec: ["BND-022", "BND-026"],
      run({ maplibre, expect }) {
        const first = expect.throws(
          () => maplibre.setNetworkStatus(NetworkStatus.fromRawValue(4242)),
          "the first failing call",
        );
        const captured = first.diagnostic;
        expect.notEqual(captured, "", "the first diagnostic");
        // A later failing call sets its own thread-local diagnostic. The first
        // error still carries the message that belonged to it.
        const second = expect.throws(
          () => maplibre.setNetworkStatus(NetworkStatus.fromRawValue(9999)),
          "the second failing call",
        );
        expect.notEqual(second.diagnostic, "", "the second diagnostic");
        expect.equal(first.diagnostic, captured, "the first diagnostic again");
      },
    },
  ],
};

export const RUNTIME_GROUP: ConformanceGroup = {
  name: "a runtime",
  cases: [
    {
      name: "creates, pumps, drains an empty queue, and closes",
      spec: ["BND-040", "BND-080"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        runtime.pump(0);
        expect.absent(runtime.pollEvent(), "an empty event queue");
        runtime.close();
        expect.ok(runtime.isClosed, "the runtime is closed");
        // Closing twice succeeds without crossing into native code.
        runtime.close();
      },
    },
    {
      name: "reports its own closed state before reaching native code",
      spec: ["BND-023"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        runtime.close();
        const error = expect.throws(
          () => runtime.pump(0),
          "a closed runtime rejects a pump",
        );
        expect.equal(error.kind, "closedHandle", "the error kind");
        // Nothing crossed into C, so there is no native diagnostic to carry.
        expect.equal(error.diagnostic, "", "no native diagnostic");
      },
    },
    {
      name: "maps every native status category to its error kind",
      spec: ["BND-020", "BND-021"],
      run({ expect }) {
        // No real call can produce every status on demand, and a future
        // library can report one this build does not name, so the conversion
        // is driven directly. It is the same function every failing call uses.
        const expected: readonly [number, string][] = [
          [-1, "invalidArgument"],
          [-2, "invalidState"],
          [-3, "wrongThread"],
          [-4, "unsupported"],
          [-5, "nativeError"],
        ];
        for (const [status, kind] of expected) {
          expect.equal(
            errorKindForStatus(status),
            kind,
            `status ${status} maps to ${kind}`,
          );
        }

        // A status this build does not name is not collapsed onto a kind that
        // happens to be nearby, and the raw value survives on the error so a
        // caller can report what actually came back.
        expect.equal(
          errorKindForStatus(-999),
          "unknownStatus",
          "an unnamed status keeps its own kind",
        );
        const error = new MaplibreError("unknownStatus", "from the future", {
          nativeStatus: -999,
        });
        expect.equal(error.nativeStatus, -999, "the raw status is preserved");
      },
    },
    {
      name: "reports its own diagnostic rather than a stale native one",
      spec: ["BND-025"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        const map = runtime.createMap({ width: 64, height: 64 });
        // A real native failure first, which leaves a diagnostic behind in the
        // library's thread-local storage.
        const native = expect.throws(
          () => map.setStyleJson("{not json"),
          "a style the library rejects",
        );
        expect.ok(
          native.diagnostic.length > 0,
          "the native failure carried a diagnostic",
        );

        // A failure the binding decides for itself must not carry that one. It
        // describes what this binding refused, and names no native status
        // because no call was made.
        map.close();
        const own = expect.throws(
          () => map.setStyleJson("{}"),
          "using a map after it closed",
        );
        expect.equal(own.kind, "closedHandle", "the binding's own kind");
        expect.absent(
          own.nativeStatus,
          "a binding-owned failure has no native status",
        );
        expect.notEqual(
          own.diagnostic,
          native.diagnostic,
          "and does not repeat the last native diagnostic",
        );
        runtime.close();
      },
    },
    {
      name: "keeps a payload smaller than this build expects as raw bytes",
      spec: ["BND-087"],
      run({ maplibre, expect }) {
        const native = nativeOf(maplibre);
        const layout = native.layout("mln_runtime_event_render_frame");
        const type =
          MLN_RUNTIME_EVENT_PAYLOAD_TYPE.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;

        native.scope((scope) => {
          // A future library can shrink or replace a payload, and no call this
          // build can make produces one, so the decoder is given a short one
          // directly.
          const storage = scope.allocateZeroed(layout.size, layout.align);
          const short = decodePayload(native, type, storage, layout.size - 1);
          expect.equal(
            short.kind,
            "unknown",
            "a payload shorter than the layout is not read as one",
          );
          if (short.kind === "unknown") {
            expect.equal(short.rawType, type, "the raw payload type survives");
            expect.equal(
              short.bytes.length,
              layout.size - 1,
              "and exactly what was sent is kept",
            );
          }

          // The same payload at its full size decodes as itself, so the guard
          // is a size check rather than a decoder that never runs.
          const whole = decodePayload(native, type, storage, layout.size);
          expect.notEqual(
            whole.kind,
            "unknown",
            "a payload of the expected size is decoded",
          );
        });
      },
    },
    {
      name: "keeps a handle live when native release refuses",
      spec: ["BND-041"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        try {
          const map = runtime.createMap({ width: 64, height: 64 });
          // Nothing a caller can do makes a destroy fail, so it is arranged.
          forceStatus(EP.mln_map_destroy, -5);
          try {
            expect.equal(
              expect.throws(() => map.close(), "a destroy that refuses").kind,
              "nativeError",
              "the failure is reported rather than swallowed",
            );
            // A handle whose release failed is still a handle: the native map
            // is still there, and treating it as gone would leak it.
            expect.equal(map.isClosed, false, "the map is still live");
            expect.equal(
              map.getSize().width,
              64,
              "and still usable, because nothing was released",
            );
          } finally {
            clearForcedStatuses();
          }

          map.close();
          expect.equal(map.isClosed, true, "a later release closed it");
        } finally {
          clearForcedStatuses();
          runtime.close();
        }
      },
    },
    {
      name: "reports a dropped handle rather than destroying it",
      spec: ["BND-044"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        try {
          const map = runtime.createMap({ width: 64, height: 64 });
          // A host drops a wrapper by losing its last reference, and no
          // collector is obliged to notice, so this takes the path the
          // finalizer takes instead of waiting for one. The library names the
          // leak on the process's error stream, so a line about a leaked Map
          // handle while this case runs is the case working.
          const mapState = handleStateOf(map);
          expect.ok(
            mapState.reportLeakAsCollected(),
            "the collection path reported the open map",
          );

          // A cleanup hook cannot know whether the owner thread still exists,
          // so destroying a thread-affine handle from one would be a use from
          // the wrong thread at best. The native map answering afterwards is
          // what shows nothing destroyed it.
          expect.equal(map.isClosed, false, "the map is still live");
          expect.equal(
            map.getSize().width,
            64,
            "and the native map still answers",
          );
          map.close();
          expect.equal(map.isClosed, true, "an explicit release closes it");
          expect.equal(
            mapState.reportLeakAsCollected(),
            false,
            "and a closed handle has nothing left to report",
          );

          // A runtime is thread-affine for the same reason and is watched the
          // same way, so the hook leaves it alone too.
          expect.ok(
            handleStateOf(runtime).reportLeakAsCollected(),
            "the collection path reported the open runtime",
          );
          runtime.pump(0);
          expect.absent(
            runtime.pollEvent(),
            "the native runtime still answers",
          );
        } finally {
          runtime.close();
        }
      },
    },
    {
      name: "reports a handle whose release failed, and closes it on a retry",
      spec: ["BND-048"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        try {
          const map = runtime.createMap({ width: 64, height: 64 });
          const state = handleStateOf(map);
          // Nothing a caller can do makes a destroy fail, so it is arranged.
          forceStatus(EP.mln_map_destroy, -5);
          try {
            expect.equal(
              expect.throws(() => map.close(), "a destroy that refuses").kind,
              "nativeError",
              "the failure reported through the call that attempted it",
            );
          } finally {
            clearForcedStatuses();
          }

          // The release did not happen, so the handle is still watched: a host
          // that gives up and drops the wrapper is told about the map it still
          // owns rather than losing it silently.
          expect.ok(
            state.reportLeakAsCollected(),
            "the leak channel still covers a handle whose release failed",
          );
          expect.equal(
            map.getSize().width,
            64,
            "and neither the failed release nor the report destroyed it",
          );

          map.close();
          expect.equal(map.isClosed, true, "a retried release closes it");
        } finally {
          clearForcedStatuses();
          runtime.close();
        }
      },
    },
    {
      name: "rejects an asset path containing an embedded NUL",
      spec: ["BND-024"],
      run({ maplibre, expect }) {
        const error = expect.throws(
          () =>
            maplibre.createRuntime({
              assetPath: `assets${String.fromCharCode(0)}/tiles`,
            }),
          "an embedded NUL cannot cross as a C string",
        );
        // The binding owns this one: a null-terminated C string would carry
        // only the bytes before the NUL.
        expect.equal(error.kind, "invalidInput", "the error kind");
      },
    },
    {
      name: "rejects a pump timeout that is not a duration",
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime) => {
          expect.throws(() => runtime.pump(Number.NaN), "a pump of NaN");
          expect.throws(() => runtime.pump(-5), "a negative pump");
        });
      },
    },
    {
      name: "parks until a wake source signals, and the source outlives it",
      spec: ["BND-089"],
      run({ maplibre, expect }) {
        const runtime = maplibre.createRuntime();
        const wake = runtime.acquireWakeSource();
        // A signal raised before the pump sets the flag, so the pump returns
        // without parking for the full timeout.
        wake.signal();
        const started = Date.now();
        runtime.pump(5_000);
        expect.ok(
          Date.now() - started < 2_000,
          "the pump returned on the wake",
        );

        runtime.close();
        // A wake source holds its own reference to the wake state, so
        // signalling after the runtime is gone succeeds and does nothing.
        wake.signal();
        wake.close();
        expect.ok(wake.isClosed, "the wake source closed");
      },
    },
    {
      name: "moves a wake source through a one-shot carrier",
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime) => {
          const wake = runtime.acquireWakeSource();
          const transfer = wake.transfer();
          const bytes = transfer.bytes;

          // The sender stops being an owner as soon as the token is issued.
          expect.ok(wake.isClosed, "the sender's wrapper closed");
          expect.throws(() => wake.signal(), "the sender cannot signal");

          const adopted = maplibre.adoptWakeSource(transfer);
          adopted.signal();
          // A host that copied the carrier's bytes still names a transfer that
          // is already claimed, so it cannot produce a second owner.
          expect.throws(
            () => maplibre.adoptWakeSource(bytes),
            "a copied carrier cannot adopt twice",
          );
          adopted.close();
        });
      },
    },
    {
      name: "releases a wake source whose transfer nobody adopts",
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime) => {
          const wake = runtime.acquireWakeSource();
          const transfer = wake.transfer();
          // Nothing owns the handle between issue and adoption, so a transfer
          // that is dropped has to release it rather than strand it.
          transfer.discard();
          expect.throws(() => transfer.bytes, "a discarded transfer is spent");
          // Discarding twice is a no-op, and the slot it held is free again.
          transfer.discard();
          const second = runtime.acquireWakeSource();
          second.close();
        });
      },
    },
    {
      name: "keeps event payload data valid after the next poll",
      spec: ["BND-082"],
      run({ maplibre, expect }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open();
          map.setStyleJson(EMPTY_STYLE);
          const loaded = pumpFor(runtime, RuntimeEventType.mapStyleLoaded);
          const event = expect.defined(loaded, "the style-loaded event");
          const message = event.message;
          const payload = event.payload;
          // Every later poll reuses the same native event storage, so a copy
          // that aliased it would change underneath the caller.
          for (let attempt = 0; attempt < 5; attempt += 1) {
            runtime.pump(0);
            runtime.pollEvent();
          }
          expect.equal(event.message, message, "the copied message");
          expect.equal(event.payload.kind, payload.kind, "the copied payload");
        });
      },
    },
    {
      name: "preserves an unknown event type by its raw value",
      spec: ["BND-083"],
      run({ expect }) {
        const unknown = RuntimeEventType.fromRawValue(9999);
        expect.equal(unknown.rawValue, 9999, "the preserved raw value");
        expect.equal(unknown.name, "unknown(9999)", "the unknown event's name");
      },
    },
  ],
};
