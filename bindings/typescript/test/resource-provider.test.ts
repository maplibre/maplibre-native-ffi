// Resource providers and transforms: native code answers each request at once
// and queues a copy with its handle, which arrives later as an owner that the
// host answers from the event loop.

import assert from "node:assert/strict";

import {
  MaplibreError,
  ResourceKind,
  ResourceKindFlag,
  type ResourceProvider,
  type ResourceRequestHandle,
  type ResourceRequestInfo,
  ResourceResponseStatus,
  type ResourceTransformRequestHandle,
  RuntimeEventType,
  Status,
  UrlMatchFlag,
} from "../src/index.ts";
import { rootsOf } from "../src/internal/owners.ts";
import {
  headerTransforms,
  openRuntime,
  setHeaderTransform,
  styleResponse,
  styleRoute,
} from "./support/fixture.ts";
import { bounded, callbackArrival, teardown, test } from "./support/harness.ts";
import { capabilities } from "./support/host-capabilities.ts";
import { captureReportedErrors } from "./support/internals.ts";
import { emptyStyle } from "./support/styles.ts";

function isStatus(status: number) {
  return (error: unknown) =>
    error instanceof MaplibreError && error.status === status;
}

/** A provider whose one route matches nothing, so every request loads. */
const nothingClaimed: ResourceProvider = {
  routes: [{ kinds: ResourceKindFlag.unknown, url: "none://" }],
  callback: (_request, handle) => handle.close(),
};

/** A Promise for the next request that a provider hands over. */
function handoff() {
  let hand!: (value: [ResourceRequestInfo, ResourceRequestHandle]) => void;
  const handed = new Promise<[ResourceRequestInfo, ResourceRequestHandle]>(
    (resolve) => (hand = resolve),
  );
  return {
    handed,
    provider: (url: string) => ({
      routes: [styleRoute(url)],
      callback: (request: ResourceRequestInfo, handle: ResourceRequestHandle) =>
        hand([request, handle]),
    }),
  };
}

test(
  "a provider answers the requests its routes match after its callback returns",
  async () => {
    const origin = await capabilities.httpOrigin!({
      "/style.json": JSON.stringify(emptyStyle()),
    });
    teardown(() => origin.close());
    const claimed = "custom://claimed-style.json";
    const requests: string[] = [];
    let hand!: (handle: ResourceRequestHandle) => void;
    const handed = new Promise<ResourceRequestHandle>(
      (resolve) => (hand = resolve),
    );
    const fixture = await openRuntime({
      provider: {
        // The pattern alone names no kinds, so it matches every one.
        routes: [{ url: "custom://**", flags: UrlMatchFlag.glob }],
        callback(request, handle) {
          requests.push(request.requestedUrl);
          assert.equal(request.kind, ResourceKind.style);
          hand(handle);
        },
      },
    });

    // A request that no route matches passes through to the network.
    const passed = await fixture.openMap();
    await bounded(passed.setStyleUrl(`${origin.url}/style.json`), "a command");
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, passed.id);
    assert.deepEqual(
      (await origin.requests()).map((request) => request.path),
      ["/style.json"],
    );

    // A claimed request arrives as a handle that answers it after the
    // provider callback returned.
    const map = await fixture.openMap();
    await bounded(map.setStyleUrl(claimed), "a command");
    const handle = await callbackArrival(handed, "the provider");
    assert.deepEqual(requests, [claimed]);
    assert.equal(handle.isCancelled(), false);

    // A response that native code refuses, such as a message with an
    // embedded NUL, leaves the request open for a valid one.
    assert.throws(
      () =>
        handle.complete({
          status: ResourceResponseStatus.error,
          errorMessage: "bad\u0000message",
          etag: "",
        }),
      isStatus(Status.invalidArgument),
    );
    handle.complete(styleResponse());
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);

    // The accepted answer consumed the handle, so closing it does nothing and
    // native code refuses a second answer or any other call on it.
    assert.equal(handle.isClosed, true);
    handle.close();
    assert.throws(
      () =>
        handle.complete({
          status: ResourceResponseStatus.noContent,
          errorMessage: "",
          etag: "",
        }),
      isStatus(Status.invalidState),
    );
    assert.throws(() => handle.passThrough(), isStatus(Status.invalidState));
    assert.throws(() => handle.isCancelled(), isStatus(Status.invalidState));
  },
  { needs: ["httpOrigin"] },
);

test("a provider callback error is reported and fails the request", async () => {
  const errors = captureReportedErrors();
  const url = "custom://throwing-style.json";
  const fixture = await openRuntime({
    provider: {
      routes: [styleRoute(url)],
      callback() {
        throw new Error("provider failed");
      },
    },
  });
  const map = await fixture.openMap();

  await bounded(map.setStyleUrl(url), "a command");
  // The binding closes the handle that the throwing callback abandoned, which
  // fails the request instead of leaving the style load waiting.
  await fixture.awaitEventType(RuntimeEventType.mapLoadingFailed, map.id);
  assert.equal(errors.length, 1);
  assert.match(errors[0]!, /provider failed/);
});

test("a provider whose callback getter throws fails the request", async () => {
  const errors = captureReportedErrors();
  const url = "custom://throwing-getter-style.json";
  let armed = false;
  const fixture = await openRuntime({
    provider: {
      routes: [styleRoute(url)],
      get callback(): ResourceProvider["callback"] {
        if (armed) {
          throw new Error("getter failed");
        }
        return (_request, handle) => handle.close();
      },
    },
  });
  const map = await fixture.openMap();
  armed = true;

  await bounded(map.setStyleUrl(url), "a command");
  // The adapter adopts the handle before it reads the callback, so the getter
  // that throws still leaves the request to be released.
  await fixture.awaitEventType(RuntimeEventType.mapLoadingFailed, map.id);
  assert.equal(errors.length, 1);
  assert.match(errors[0]!, /getter failed/);
});

test(
  "a transform answers its request after its callback returns",
  async () => {
    const origin = await capabilities.httpOrigin!({
      "/style.json": JSON.stringify(emptyStyle()),
    });
    teardown(() => origin.close());
    const requested = "custom://requested/style.json";
    // A provider whose one route matches nothing leaves the style to native
    // loading, which waits for the transform's answer.
    const fixture = await openRuntime({ provider: nothingClaimed });
    let transform!: (request: ResourceTransformRequestHandle) => void;
    const transformed = new Promise<ResourceTransformRequestHandle>(
      (resolve) => (transform = resolve),
    );
    const urls: string[] = [];
    await bounded(
      fixture.runtime.setResourceTransform((kind, url, request) => {
        if (kind === ResourceKind.style) {
          urls.push(url);
          transform(request);
        } else {
          request.close();
        }
      }),
      "the resource transform",
    );

    const map = await fixture.openMap();
    await bounded(map.setStyleUrl(requested), "a command");
    // The request waits for its URL until the host answers from the event
    // loop, after the transform callback returned.
    const request = await callbackArrival(transformed, "the transform");
    assert.deepEqual(urls, [requested]);
    request.complete(`${origin.url}/style.json`);
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
    assert.deepEqual(
      (await origin.requests()).map((entry) => entry.path),
      ["/style.json"],
    );
    await bounded(fixture.runtime.clearResourceTransform(), "the clear");
  },
  { needs: ["httpOrigin"] },
);

test(
  "a style URL reaches a local origin with a rule-transformed header without a pump",
  async () => {
    const origin = await capabilities.httpOrigin!({
      "/style.json": JSON.stringify(emptyStyle()),
    });
    teardown(() => origin.close());
    const fixture = await openRuntime({ provider: nothingClaimed });
    // OpenHarmony refuses header transforms, so the request arrives there
    // with no transformed header.
    await setHeaderTransform(fixture.runtime, {
      rules: [
        {
          flags: UrlMatchFlag.glob,
          url: `${origin.url}/**`,
          headers: [{ name: "x-maplibre-test", value: "transformed" }],
        },
      ],
    });

    // The file source fetches on native threads; this realm only awaits the
    // command and the event.
    const map = await fixture.openMap();
    await bounded(map.setStyleUrl(`${origin.url}/style.json`), "a command");
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
    const requests = await origin.requests();
    assert.equal(requests.length, 1);
    assert.equal(requests[0]!.path, "/style.json");
    assert.equal(
      requests[0]!.headers["x-maplibre-test"],
      headerTransforms ? "transformed" : undefined,
    );
  },
  { needs: ["httpOrigin"] },
);

test("a cancel registration on a cancelled request is refused and not rooted", async () => {
  const url = "custom://late-cancel-style.json";
  const { handed, provider } = handoff();
  const fixture = await openRuntime({ provider: provider(url) });
  const map = await fixture.openMap();
  await bounded(map.setStyleUrl(url), "a command");
  const [, handle] = await callbackArrival(handed, "the provider");
  teardown(() => handle.close());

  await bounded(map.close(), "map close");
  await bounded(fixture.runtime.close(), "runtime close");
  assert.equal(handle.isCancelled(), true);

  // Native reports the cancellation through the registration's output
  // instead of storing the callback, so the binding keeps no root for it.
  assert.equal(
    handle.setCancelCallback(() => {}),
    true,
  );
  assert.equal(rootsOf(handle).roots.size, 0);
});

test("a cancel callback runs once for a request that teardown discards", async () => {
  const url = "custom://cancelled-style.json";
  const { handed, provider } = handoff();
  const fixture = await openRuntime({ provider: provider(url) });
  const map = await fixture.openMap();
  await bounded(map.setStyleUrl(url), "a command");
  const [, handle] = await callbackArrival(handed, "the provider");
  teardown(() => handle.close());
  const cancels: boolean[] = [];
  assert.equal(
    handle.setCancelCallback(() => cancels.push(handle.isCancelled())),
    false,
  );
  assert.equal(rootsOf(handle).roots.size, 1);
  // A request takes one cancel registration.
  assert.throws(
    () => handle.setCancelCallback(() => {}),
    isStatus(Status.invalidState),
  );

  await bounded(map.close(), "map close");
  await bounded(fixture.runtime.close(), "runtime close");
  // The cancel and its retirement arrive before the runtime's close
  // completion, through the same queue.
  assert.deepEqual(cancels, [true]);
  assert.equal(rootsOf(handle).roots.size, 0);
  assert.throws(
    () => handle.complete(styleResponse()),
    isStatus(Status.invalidState),
  );
});
