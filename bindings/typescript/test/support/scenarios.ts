// Work that the suite runs both in its own realm and in spawned realms, so a
// case can compare what each realm receives.

import {
  CameraUpdateMode,
  logSetCallback,
  type RenderSessionHandle,
  type ResourceRequestHandle,
  type RuntimeHandle,
  RuntimeEventType,
  runtimeCreate,
  UrlMatchFlags,
} from "../../src/index.ts";
import { openRuntime, styleResponse, styleRoute } from "./fixture.ts";
import { renderExtent, requestFrame, TestSession } from "./graphics.ts";
import { bounded, callbackArrival } from "./harness.ts";
import { backgroundStyle, emptyStyle } from "./styles.ts";

/** Loads a style, waits for its event, and closes everything it opened. */
export async function loadStyle(): Promise<void> {
  const fixture = await openRuntime();
  const map = await fixture.openStyledMap();
  await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
}

/**
 * Serves `scheme://style.json` through a routed provider with a custom
 * geometry source, renders until the source fetches a tile, and returns every
 * request URL and tile that this realm's callbacks received.
 */
export async function routeRequests(
  scheme: string,
): Promise<{ requests: string[]; tiles: string[] }> {
  const requests: string[] = [];
  const tiles: string[] = [];
  const fixture = await openRuntime({
    provider: {
      routes: [{ flags: UrlMatchFlags.glob, url: `${scheme}://**` }],
      callback(request, handle) {
        requests.push(request.requestedUrl ?? "");
        handle.complete(styleResponse());
      },
    },
  });
  const map = await fixture.openMap({ initialExtent: renderExtent });
  await bounded(map.setStyleUrl(`${scheme}://style.json`), "a command");
  await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded, map.id);
  await bounded(
    map.addCustomGeometrySource("custom", {
      fetchTile: (tile) =>
        tiles.push(`${scheme}/${tile.z}/${tile.x}/${tile.y}`),
    }),
    "a command",
  );
  await bounded(
    map.addStyleLayerJson(
      JSON.stringify({ id: "fill", type: "fill", source: "custom" }),
    ),
    "a command",
  );
  const worker = await TestSession.attach(map);
  for (let frame = 0; frame < 32 && tiles.length === 0; frame++) {
    await worker.renderFrame();
  }
  return { requests, tiles };
}

/**
 * Keeps native threads posting to this realm: commands, camera animations
 * whose end handlers run, style replacements that raise events, and queries.
 * `started` runs after the first round; the burst never ends on its own.
 */
export async function burst(started: () => void): Promise<never> {
  const runtime: RuntimeHandle = runtimeCreate();
  await runtime.setEventHandler(() => {});
  const { map, ready } = runtime.createMap({ initialExtent: renderExtent });
  await ready;
  let ended = 0;
  for (let round = 0; ; round++) {
    const pending: Promise<unknown>[] = [
      map.setStyleJson(emptyStyle()),
      map.updateCamera({
        mode: CameraUpdateMode.ease,
        camera: { zoom: round % 8 },
        animation: { durationMs: 1, endHandler: () => ended++ },
      }),
      map.addCustomGeometrySource(`source-${round}`, { fetchTile() {} }),
      map.getCamera(),
    ];
    if (round === 0) {
      started();
    }
    await bounded(Promise.all(pending), "a burst round");
  }
}

// What liveNativeWork leaves open stays reachable until the realm ends. A map
// that the collector disposed would drop its pending request, and the wait for
// that request would never end.
const liveWork: object[] = [];

/**
 * Leaves native work live for the realm's end: a core-worker session in the
 * middle of frames, a provider request that was never answered, and callback
 * registrations of every delivery kind. PLAT-02 ends the process or the realm
 * right after it returns. Returns the session.
 */
export async function liveNativeWork(): Promise<RenderSessionHandle> {
  logSetCallback(() => {});
  const runtime = runtimeCreate();
  await runtime.setEventHandler(() => {});
  let hold!: (handle: ResourceRequestHandle) => void;
  const held = new Promise<ResourceRequestHandle>(
    (resolve) => (hold = resolve),
  );
  await runtime.setResourceProvider({
    routes: [styleRoute("exit://style.json")],
    callback: (_request, handle) => hold(handle),
  });
  await runtime.setResourceTransform((_kind, _url, request) => request.close());
  const { map: pending, ready: pendingReady } = runtime.createMap({
    initialExtent: renderExtent,
  });
  liveWork.push(runtime, pending);
  await pendingReady;
  await bounded(pending.setStyleUrl("exit://style.json"), "a command");
  // The provider callback holds nothing alive, so a bounded wait does.
  liveWork.push(await callbackArrival(held, "the held request"));

  const { map, ready } = runtime.createMap({ initialExtent: renderExtent });
  await ready;
  await bounded(map.setStyleJson(backgroundStyle()), "a command");
  await bounded(
    map.addCustomGeometrySource("custom", { fetchTile() {} }),
    "a command",
  );
  const worker = await TestSession.attach(map);
  liveWork.push(map, worker);
  await worker.renderFrame();
  for (let token = 100n; token < 104n; token++) {
    requestFrame(worker.session, token);
  }
  return worker.session;
}
