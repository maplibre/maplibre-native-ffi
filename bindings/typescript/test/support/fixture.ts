// The runtime and map fixture that most cases build on.
//
// Each handle a fixture opens is closed by a teardown, so a failing case never
// leaks a runtime into the cases after it.

import assert from "node:assert/strict";

import {
  AwaitableWake,
  type HttpHeaderTransform,
  MaplibreError,
  type MapHandle,
  type MapOptions,
  ResourceKindFlag,
  type ResourceProvider,
  type ResourceResponse,
  ResourceResponseStatus,
  type ResourceRoute,
  type RuntimeEvent,
  type RuntimeEventType,
  type RuntimeHandle,
  type RuntimeOptions,
  runtimeCreate,
  Status,
} from "../../src/index.ts";
import { bounded, closeIfLive, nextWake, teardown } from "./harness.ts";
import { capabilities } from "./host-capabilities.ts";
import { emptyStyle } from "./styles.ts";

/** Bytes of a UTF-8 string. */
export function utf8(text: string): Uint8Array {
  return new TextEncoder().encode(text);
}

/**
 * Whether the C API accepts HTTP header transforms on this platform. The
 * HTTP client of OpenHarmony, the only platform of ArkTS, cannot keep a
 * transformed header from following a cross-origin redirect, so runtime.h
 * documents that the C API refuses header transforms there.
 */
export const headerTransforms = capabilities.host !== "arkts";

/**
 * Sets a header transform, or checks the documented refusal where the
 * platform has none.
 */
export async function setHeaderTransform(
  runtime: RuntimeHandle,
  transform: HttpHeaderTransform,
): Promise<void> {
  const call = runtime.setHttpHeaderTransform(transform);
  if (headerTransforms) {
    await bounded(call, "a header transform");
  } else {
    await assert.rejects(
      call,
      (error) =>
        error instanceof MaplibreError && error.status === Status.unsupported,
    );
  }
}

/** Answers every request with an error, so no case reaches the network. */
export const denyingProvider: ResourceProvider = {
  routes: [],
  callback(_request, handle) {
    handle.complete({
      status: ResourceResponseStatus.error,
      errorMessage: "the test fixture serves no resources",
      etag: "",
    });
  },
};

/** A route that claims one style URL. */
export function styleRoute(url: string): ResourceRoute {
  return { kinds: ResourceKindFlag.style, url };
}

export function styleResponse(style = emptyStyle()): ResourceResponse {
  return {
    status: ResourceResponseStatus.ok,
    bytes: utf8(style),
    errorMessage: "",
    etag: "",
  };
}

/**
 * The events of one runtime, which its event handler queues. A wait holds
 * the event loop through an AwaitableWake, as a pending binding Promise does.
 */
export class EventStream {
  readonly #pending: RuntimeEvent[] = [];
  readonly #wake = new AwaitableWake();

  /** The handler to register, which queues each batch's events. */
  readonly handler = (events: readonly RuntimeEvent[]) => {
    this.#pending.push(...events);
    this.#wake.wake();
  };

  /** Receives events until one matches, discarding earlier events. */
  async awaitEvent(
    matches: (event: RuntimeEvent) => boolean,
    what: string,
  ): Promise<RuntimeEvent> {
    for (;;) {
      for (
        let event = this.#pending.shift();
        event;
        event = this.#pending.shift()
      ) {
        if (matches(event)) {
          return event;
        }
      }
      await nextWake(this.#wake, what);
    }
  }

  awaitEventType(type: RuntimeEventType, source?: bigint) {
    return this.awaitEvent(
      (event) =>
        event.type === type &&
        (source === undefined || event.source === source),
      `event type ${type}`,
    );
  }
}

/** Registers an event stream as the runtime's event handler. */
export async function listen(runtime: RuntimeHandle): Promise<EventStream> {
  const events = new EventStream();
  await bounded(runtime.setEventHandler(events.handler), "the event handler");
  return events;
}

export interface RuntimeFixture {
  readonly runtime: RuntimeHandle;
  readonly events: EventStream;
  awaitEvent(
    matches: (event: RuntimeEvent) => boolean,
    what: string,
  ): Promise<RuntimeEvent>;
  awaitEventType(
    type: RuntimeEventType,
    source?: bigint,
  ): Promise<RuntimeEvent>;
  /** Creates a map that a teardown closes. */
  openMap(options?: MapOptions): Promise<MapHandle>;
  /** Creates a map and commits `style` to it. */
  openStyledMap(style?: string, options?: MapOptions): Promise<MapHandle>;
}

export const extent = { width: 64, height: 64, scaleFactor: 1 };

/**
 * Creates a runtime that a teardown closes, with its events on an event
 * stream and the denying provider unless `provider` names another.
 */
export async function openRuntime(
  options: { runtime?: RuntimeOptions; provider?: ResourceProvider } = {},
): Promise<RuntimeFixture> {
  const runtime = runtimeCreate(options.runtime);
  teardown(() => closeIfLive(() => runtime.close()));
  const events = await listen(runtime);
  await bounded(
    runtime.setResourceProvider(options.provider ?? denyingProvider),
    "the resource provider",
  );
  const fixture: RuntimeFixture = {
    runtime,
    events,
    awaitEvent: (matches, what) => events.awaitEvent(matches, what),
    awaitEventType: (type, source) => events.awaitEventType(type, source),
    async openMap(mapOptions = { initialExtent: extent }) {
      const { map, ready } = runtime.createMap(mapOptions);
      teardown(() => closeIfLive(() => map.close()));
      await bounded(ready, "map creation");
      return map;
    },
    async openStyledMap(style = emptyStyle(), mapOptions) {
      const map = await fixture.openMap(mapOptions);
      await bounded(map.setStyleJson(style), "a command");
      return map;
    },
  };
  return fixture;
}
