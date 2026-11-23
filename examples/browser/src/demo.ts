// The demo page: a continuous map on a full-page canvas, which the pointer,
// the wheel, and the keyboard move. A Web Worker runs the map
// (map-worker.ts) and presents to the canvas, which the page transfers to it.
// Each backend's page serves the browser module of its backend beside it, and
// everything else is shared.

import {
  type CameraOptions,
  type GestureInput,
  GestureMode,
  PointerKind,
  type ScreenPoint,
} from "@maplibre/native-ffi";

import { addCollapse, addStylePicker, showAttributions } from "./panel.ts";
import type { FromWorker, ToWorker, View } from "./protocol.ts";
import { defaultStyle, type Style, styles } from "./styles.ts";

import "./demo.css";

/** A backend's page. */
export interface Backend {
  readonly id: "webgl" | "webgpu";
  /** Throws when the browser can't run the backend. */
  check(): Promise<void>;
}

/** The map in the Web Worker, as the page reaches it. */
class RemoteMap {
  readonly #worker: Worker;
  /** The camera, as the map last reported it. */
  view: View | undefined;

  constructor(worker: Worker) {
    this.#worker = worker;
  }

  #post(message: ToWorker, transfer: Transferable[] = []): void {
    this.#worker.postMessage(message, transfer);
  }

  start(message: Extract<ToWorker, { type: "start" }>): void {
    this.#post(message, [message.canvas]);
  }

  resize(size: { width: number; height: number }): void {
    this.#post({ type: "resize", size });
  }

  /** Sends the inputs of `event` to the map's gesture recognizer. */
  gesture(event: Event, ...inputs: GestureInput[]): void {
    const time = performance.timeOrigin + event.timeStamp;
    this.#post({ type: "gesture", inputs, time });
  }

  setStyle(style: Style): void {
    this.#post({ type: "setStyle", styleId: style.id });
  }
}

const status = document.querySelector<HTMLElement>("#status")!;
const canvas = document.querySelector<HTMLCanvasElement>("#map")!;

// Once the page shows a fatal error, nothing replaces it. A style's error
// lasts until the user picks another style.
let failed = false;

function showStatus(message: string): void {
  if (failed) return;
  status.textContent = message;
  status.dataset.kind = "busy";
  status.hidden = message === "";
}

function showError(message: string, { fatal = true } = {}): void {
  if (failed) return;
  status.textContent = message;
  status.dataset.kind = "error";
  status.hidden = false;
  failed = fatal;
}

// The style, as ?style=, so a link or a switch of backend keeps it.
let style = styleFromUrl();

function styleFromUrl(): Style {
  const id = new URLSearchParams(location.search).get("style");
  return styles.find((style) => style.id === id) ?? defaultStyle;
}

/** Sets a search parameter of the page's URL, or removes it for `null`. */
function setSearchParam(name: string, value: string | null): void {
  const url = new URL(location.href);
  if (value === null) url.searchParams.delete(name);
  else url.searchParams.set(name, value);
  history.replaceState(null, "", url);
  updateBackendLinks();
}

/** Matches the page to the style: its theme, and the canvas before a frame. */
function showStyleTheme(style: Style): void {
  document.documentElement.dataset.theme = style.dark ? "dark" : "light";
  canvas.style.backgroundColor = style.swatch.land;
}

/** Runs the demo with `backend`, and reports a failure on the page. */
export function run(backend: Backend): void {
  showStyleTheme(style);
  addCollapse();
  addStylePicker(style, pickStyle);
  updateBackendLinks();
  for (const link of document.querySelectorAll<HTMLAnchorElement>(
    "nav a[data-backend]",
  )) {
    if (link.dataset.backend === backend.id) {
      link.setAttribute("aria-current", "page");
    }
  }
  start(backend).catch((error: unknown) => {
    console.error(error);
    showError(error instanceof Error ? error.message : String(error));
  });
}

async function start(backend: Backend): Promise<void> {
  await backend.check();
  showStatus("Loading the library…");
  const worker = new Worker(new URL("./map-worker.ts", import.meta.url), {
    type: "module",
  });
  const map = new RemoteMap(worker);
  worker.addEventListener("error", (event) => {
    showError(event.message || "The map's Web Worker failed.");
  });
  worker.addEventListener("message", ({ data }: MessageEvent<FromWorker>) => {
    switch (data.type) {
      case "status":
        showStatus(data.message);
        break;
      case "error":
        showError(data.message, { fatal: data.fatal });
        break;
      case "attributions":
        showAttributions(data.attributions);
        break;
      case "ready":
        map.view = data.view;
        styledMap = map;
        addGestures(map);
        break;
      case "camera":
        map.view = data.view;
        if (data.settled) scheduleHashUpdate(map);
        break;
    }
  });
  showStyleTheme(style);
  showAttributions([]);
  showStatus(`Loading ${style.name}…`);
  // The worker's session sizes the canvas's drawing buffer to the map's size
  // at its scale factor; the page sizes the element.
  map.start({
    type: "start",
    // Each backend's page serves its own build of the module beside it.
    module: new URL("maplibre_native_ffi.js", location.href).href,
    canvas: canvas.transferControlToOffscreen(),
    size: logicalSize(),
    scaleFactor,
    camera: cameraFromHash() ?? {
      center: { latitude: 48.8584, longitude: 2.2945 },
      zoom: 15.5,
      bearing: -30,
      pitch: 55,
    },
    styleId: style.id,
  });
  new ResizeObserver(() => map.resize(logicalSize())).observe(canvas);
  canvas.focus();
}

// The map, once it shows on the canvas. The user can pick a style before then,
// and the map takes the latest pick.
let styledMap: RemoteMap | undefined;

function pickStyle(picked: Style): void {
  style = picked;
  setSearchParam("style", style === defaultStyle ? null : style.id);
  showStyleTheme(style);
  if (styledMap === undefined) return;
  showAttributions([]);
  // This replaces the last style's error, if it had one.
  showStatus(`Loading ${style.name}…`);
  styledMap.setStyle(style);
}

/** Points the backend links at this page's view, so a switch keeps it. */
function updateBackendLinks(): void {
  for (const link of document.querySelectorAll<HTMLAnchorElement>(
    "nav a[data-backend]",
  )) {
    link.search = location.search;
    link.hash = location.hash;
  }
}

// The map's scale factor, which stays fixed for its life.
const scaleFactor = devicePixelRatio;

/** The canvas's size in CSS pixels, which the map takes as logical pixels. */
function logicalSize(): { width: number; height: number } {
  return {
    width: Math.max(1, Math.round(canvas.clientWidth)),
    height: Math.max(1, Math.round(canvas.clientHeight)),
  };
}

// The camera, as #zoom/latitude/longitude/bearing/pitch, so a link or a
// switch of backend keeps the view.

function cameraFromHash(): CameraOptions | undefined {
  const parts = location.hash.slice(1).split("/").map(Number);
  const [zoom, latitude, longitude, bearing = 0, pitch = 0] = parts;
  if (
    parts.length < 3 ||
    parts.some((part) => !Number.isFinite(part)) ||
    zoom === undefined ||
    latitude === undefined ||
    longitude === undefined
  ) {
    return undefined;
  }
  return { center: { latitude, longitude }, zoom, bearing, pitch };
}

let hashUpdate: ReturnType<typeof setTimeout> | undefined;

function scheduleHashUpdate(map: RemoteMap): void {
  clearTimeout(hashUpdate);
  hashUpdate = setTimeout(() => {
    if (map.view === undefined) return;
    const { center, zoom, bearing, pitch } = map.view.camera;
    const hash = [
      zoom.toFixed(2),
      center.latitude.toFixed(5),
      center.longitude.toFixed(5),
      ...(bearing === 0 && pitch === 0
        ? []
        : [bearing.toFixed(1), pitch.toFixed(1)]),
    ].join("/");
    history.replaceState(null, "", `#${hash}`);
    updateBackendLinks();
  }, 250);
}

// Gestures. The map's gesture recognizer turns the canvas's input into camera
// movement: a drag pans, a right or Ctrl drag rotates and pitches, two fingers
// pinch, rotate, and pitch, the wheel and the keys zoom and pan, and a double
// click zooms in, or out with Shift. A flick keeps the map gliding.

function addGestures(map: RemoteMap): void {
  // The pointers that the recognizer tracks.
  const pointers = new Set<number>();

  const point = (event: MouseEvent): ScreenPoint => {
    const bounds = canvas.getBoundingClientRect();
    return { x: event.clientX - bounds.left, y: event.clientY - bounds.top };
  };

  // What the held keys do: Ctrl rotates, and Shift zooms out.
  const modes = (event: MouseEvent | KeyboardEvent): GestureInput => ({
    kind: "modes",
    value:
      (event.ctrlKey ? GestureMode.rotate : 0) |
      (event.shiftKey ? GestureMode.zoomOut : 0),
  });

  canvas.addEventListener("pointerdown", (event) => {
    // The middle button does nothing, and the right one rotates.
    if (event.button !== 0 && event.button !== 2) return;
    canvas.setPointerCapture(event.pointerId);
    pointers.add(event.pointerId);
    const kind =
      event.pointerType === "touch"
        ? PointerKind.touch
        : event.pointerType === "pen"
          ? PointerKind.pen
          : PointerKind.mouse;
    map.gesture(event, modes(event), {
      kind: "pointerDown",
      value: {
        id: event.pointerId,
        kind,
        position: point(event),
        modes: event.button === 2 ? GestureMode.rotate : 0,
      },
    });
  });

  canvas.addEventListener("pointermove", (event) => {
    if (!pointers.has(event.pointerId)) return;
    map.gesture(event, {
      kind: "pointerMove",
      value: { id: event.pointerId, position: point(event) },
    });
  });

  canvas.addEventListener("pointerup", (event) => {
    if (!pointers.delete(event.pointerId)) return;
    map.gesture(event, {
      kind: "pointerUp",
      value: { id: event.pointerId, position: point(event) },
    });
  });

  // A pointer that the canvas loses without a release ends with no tap or
  // glide. One that it released is already gone from `pointers`.
  const cancelPointer = (event: PointerEvent) => {
    if (!pointers.delete(event.pointerId)) return;
    map.gesture(event, {
      kind: "pointerCancel",
      value: { id: event.pointerId },
    });
  };
  canvas.addEventListener("pointercancel", cancelPointer);
  canvas.addEventListener("lostpointercapture", cancelPointer);

  canvas.addEventListener("blur", (event) => {
    const ids = [...pointers];
    pointers.clear();
    map.gesture(
      event,
      { kind: "modes", value: 0 },
      ...ids.map((id): GestureInput => ({
        kind: "pointerCancel",
        value: { id },
      })),
    );
  });
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());

  canvas.addEventListener(
    "wheel",
    (event) => {
      event.preventDefault();
      // A trackpad pinch arrives as a wheel event with Ctrl held, in smaller
      // steps than a mouse wheel's.
      const pixels =
        event.deltaMode === WheelEvent.DOM_DELTA_LINE
          ? event.deltaY * 40
          : event.deltaMode === WheelEvent.DOM_DELTA_PAGE
            ? event.deltaY * canvas.clientHeight
            : event.deltaY;
      map.gesture(event, {
        kind: "delta",
        value: {
          position: point(event),
          zoom: -pixels / (event.ctrlKey ? 100 : 300),
        },
      });
    },
    { passive: false },
  );

  const keys: Record<string, { pan?: ScreenPoint; zoom?: number }> = {
    ArrowLeft: { pan: { x: 100, y: 0 } },
    ArrowRight: { pan: { x: -100, y: 0 } },
    ArrowUp: { pan: { x: 0, y: 100 } },
    ArrowDown: { pan: { x: 0, y: -100 } },
    "+": { zoom: 1 },
    "=": { zoom: 1 },
    "-": { zoom: -1 },
  };
  const keyChanged = (event: KeyboardEvent) => {
    if (event.key === "Control" || event.key === "Shift") {
      map.gesture(event, modes(event));
    }
  };
  canvas.addEventListener("keyup", keyChanged);
  canvas.addEventListener("keydown", (event) => {
    keyChanged(event);
    const key = keys[event.key];
    if (key === undefined) return;
    event.preventDefault();
    const center = { x: canvas.clientWidth / 2, y: canvas.clientHeight / 2 };
    map.gesture(event, {
      kind: "delta",
      value: { position: center, discrete: true, ...key },
    });
  });
}
