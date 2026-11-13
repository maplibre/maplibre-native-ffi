// The Web Worker that runs the demo's map: it loads the browser module, makes
// the map, and attaches a session that presents to the page's canvas, which
// the page transferred to it. The library starts its own tile worker Web
// Workers beside this one. protocol.ts describes the messages.

import {
  CameraUpdateMode,
  FrameDemandFlag,
  loadModule,
  type MapHandle,
  MapMode,
  RenderDriverKind,
  type RenderSessionHandle,
  RenderResult,
  RuntimeEventFlag,
  RuntimeEventType,
  RuntimeHandle,
  WebObject,
} from "@maplibre/native-ffi";

import fallbackFontFaces from "../../fallback-font-faces.json";
import type { FromWorker, ToWorker, View } from "./protocol.ts";
import { defaultStyle, styles } from "./styles.ts";

// The DOM's types describe the page; these are the worker's.
declare const self: {
  postMessage(message: FromWorker): void;
  onmessage: ((event: MessageEvent<ToWorker>) => void) | null;
  requestAnimationFrame?: (callback: () => void) => number;
};

const post = (message: FromWorker) => self.postMessage(message);

/** Runs `callback` before the next frame that the browser shows. */
function beforeNextFrame(callback: () => void): void {
  if (self.requestAnimationFrame !== undefined) {
    self.requestAnimationFrame(callback);
  } else {
    setTimeout(callback, 16);
  }
}

let map: MapHandle | undefined;
let session: RenderSessionHandle | undefined;
let scaleFactor = 1;
// The input that arrives while the map loads, which it takes once it's ready.
const pending: ToWorker[] = [];

self.onmessage = ({ data }) => {
  if (data.type === "start") {
    start(data).catch((error: unknown) => {
      console.error(error);
      post({
        type: "error",
        message: error instanceof Error ? error.message : String(error),
        fatal: true,
      });
    });
  } else if (session === undefined) {
    pending.push(data);
  } else {
    handle(data);
  }
};

function handle(message: ToWorker): void {
  switch (message.type) {
    case "start":
      break;
    case "resize":
      session?.discarding.resize({ ...message.size, scaleFactor });
      break;
    case "cameraDelta":
      map?.discarding.applyCameraDelta(message.delta);
      break;
    case "jumpTo":
      map?.discarding.updateCamera({
        mode: CameraUpdateMode.jump,
        camera: message.camera,
      });
      break;
    case "cancelCameraTransitions":
      map?.discarding.cancelCameraTransitions();
      break;
    case "setStyle":
      setStyle(message.styleId);
      break;
  }
}

function view(): View {
  const { view, bounds } = map!.getSnapshot();
  const { center, zoom, bearing, pitch, roll } = view.camera;
  return {
    camera: { center, zoom, bearing, pitch, roll },
    bounds: {
      minZoom: bounds.minZoom,
      maxZoom: bounds.maxZoom,
      minPitch: bounds.minPitch,
      maxPitch: bounds.maxPitch,
    },
  };
}

function setStyle(id: string): void {
  const style = styles.find((style) => style.id === id) ?? defaultStyle;
  if (style.url !== undefined) map?.discarding.setStyleUrl(style.url);
  else map?.discarding.setStyleJson(style.json);
}

// Counts the attribution reads, so a slow read doesn't report the
// attributions of a style that the map has replaced.
let attributionRead = 0;

function updateAttributions(): void {
  const read = ++attributionRead;
  map!
    .listStyleSources()
    .then((sources) => {
      if (read !== attributionRead) return;
      post({
        type: "attributions",
        attributions: sources.map((source) => source.attribution),
      });
    })
    .catch((error: unknown) => console.error(error));
}

async function start(
  message: Extract<ToWorker, { type: "start" }>,
): Promise<void> {
  scaleFactor = message.scaleFactor;
  await loadModule(message.module);
  const runtime = RuntimeHandle.create();
  map = runtime.createMap({
    initialExtent: { ...message.size, scaleFactor },
    mapMode: MapMode.continuous,
    eventMask:
      RuntimeEventFlag.mapStyleLoaded |
      RuntimeEventFlag.mapLoadingFailed |
      RuntimeEventFlag.mapRenderError |
      RuntimeEventFlag.mapSourceChanged |
      RuntimeEventFlag.mapCameraIsChanging |
      RuntimeEventFlag.mapCameraDidChange,
    // Fonts for the scripts that the styles' glyphs can't shape, which the
    // map loads only when a label needs one.
    fallbackFontFaces: JSON.stringify(fallbackFontFaces),
  });
  const id = map.id;
  runtime.discarding.setEventHandler({
    callback: (events) => {
      for (const event of events) {
        if (event.source !== id) continue;
        switch (event.type) {
          case RuntimeEventType.mapStyleLoaded:
            post({ type: "status", message: "" });
            updateAttributions();
            break;
          case RuntimeEventType.mapSourceChanged:
            // A source's attribution arrives with its TileJSON.
            updateAttributions();
            break;
          case RuntimeEventType.mapLoadingFailed:
            post({ type: "error", message: event.message, fatal: false });
            break;
          case RuntimeEventType.mapRenderError:
            post({ type: "error", message: event.message, fatal: true });
            break;
          case RuntimeEventType.mapCameraIsChanging:
            post({ type: "camera", view: view(), settled: false });
            break;
          case RuntimeEventType.mapCameraDidChange:
            post({ type: "camera", view: view(), settled: true });
            break;
        }
      }
    },
  });
  await map.ready;
  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: message.camera,
  });
  setStyle(message.styleId);

  const demandFrame = () =>
    session?.demandFrame({ flags: FrameDemandFlag.waitForUpdate });
  // The session makes the canvas's context, and renders and presents in the
  // library's own tasks on this thread. The call keeps its own reference to
  // the canvas, so the page's handle can go at once.
  const canvas = WebObject.of(message.canvas);
  const attaching = map.attach(
    {
      kind: "webCanvas",
      value: { logicalSize: message.size, canvas: canvas.handle },
    },
    undefined,
    {
      driver: RenderDriverKind.coreWorker,
      frameWake: {
        callback: () => {
          const results = attaching.drainFrameResults();
          if (results.length === 0) return;
          if (
            results.some(
              ({ disposition }) => disposition === RenderResult.failed,
            )
          ) {
            post({
              type: "error",
              message: "The map failed to render a frame.",
              fatal: true,
            });
          } else {
            beforeNextFrame(demandFrame);
          }
        },
      },
    },
  );
  canvas.close();
  await attaching.ready;
  session = attaching;
  demandFrame();
  post({ type: "ready", view: view() });
  for (const message of pending.splice(0)) handle(message);
}
