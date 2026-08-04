/**
 * The loaded library, runtimes, wake sources, and event polling.
 */

import { RuntimeEventType } from "../events.ts";
import { NetworkStatus } from "../maplibre.ts";
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
      spec: ["BND-088", "BND-089"],
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
