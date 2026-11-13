// The demo page: a continuous map on a full-page canvas, which the pointer,
// the wheel, and the keyboard move. A Web Worker runs the map
// (map-worker.ts) and presents to the canvas, which the page transfers to it.
// Each backend's page serves the browser module of its backend beside it, and
// everything else is shared.

import {
  type CameraDelta,
  type CameraOptions,
  GesturePhase,
  type ScreenPoint,
} from "@maplibre/native-ffi";

import type { Comparison } from "./compare.ts";
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
export class RemoteMap {
  readonly #worker: Worker;
  /** The camera and its limits, as the map last reported them. */
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

  applyCameraDelta(delta: CameraDelta): void {
    this.#post({ type: "cameraDelta", delta });
  }

  jumpTo(camera: CameraOptions): void {
    this.#post({ type: "jumpTo", camera });
  }

  cancelCameraTransitions(): void {
    this.#post({ type: "cancelCameraTransitions" });
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
        addCompareToggle(map);
        break;
      case "camera":
        map.view = data.view;
        comparison?.nativeCameraChanged();
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
  comparison?.setStyle(style);
}

// The comparison with MapLibre GL JS, while the page shows it. The page loads
// GL JS only for the comparison, and keeps it in the URL as ?compare.

let comparison: Comparison | undefined;

function addCompareToggle(map: RemoteMap): void {
  const toggle = document.querySelector<HTMLInputElement>("#compare")!;
  const update = async () => {
    setSearchParam("compare", toggle.checked ? "" : null);
    comparison?.remove();
    comparison = undefined;
    if (!toggle.checked) return;
    const { compare } = await import("./compare.ts");
    // The toggle may have changed while GL JS loaded.
    if (toggle.checked && comparison === undefined) {
      comparison = compare(map, canvas, style);
    }
  };
  const change = () =>
    update().catch((error: unknown) => {
      console.error(error);
      showError(error instanceof Error ? error.message : String(error));
    });
  toggle.checked = new URLSearchParams(location.search).has("compare");
  toggle.disabled = false;
  toggle.addEventListener("change", change);
  if (toggle.checked) void change();
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

// Gestures. A drag pans, a right or Ctrl drag rotates and pitches, two
// pointers pinch, rotate, and pan, the wheel zooms, and a double click zooms
// in, or out with Shift. A drag or pinch keeps moving after release at the
// speed it moved just before, and a press stops it.

/** How far a gesture has moved the camera since it began, at `time`. */
interface Motion {
  readonly time: number;
  readonly pan: ScreenPoint;
  readonly zoom: number;
  readonly bearing: number;
  readonly pitch: number;
}

// The browser reports no pointer velocity, so the release velocities average
// the gesture's motion over this window.
const velocityWindowMs = 100;

function addGestures(map: RemoteMap): void {
  const pointers = new Map<number, ScreenPoint>();
  let rotating = false;
  // The gesture's motion over the window before its latest move, oldest first.
  let motion: Motion[] = [];
  // The latest pinch's center, which its zoom and turn keep in place.
  let anchor: ScreenPoint | undefined;

  const apply = (delta: CameraDelta) => map.applyCameraDelta(delta);

  const point = (event: MouseEvent): ScreenPoint => {
    const bounds = canvas.getBoundingClientRect();
    return { x: event.clientX - bounds.left, y: event.clientY - bounds.top };
  };

  // Drops the motion before the window that ends at `time`, but keeps where
  // the window starts.
  const trim = (time: number) => {
    while (motion.length > 1 && motion[1]!.time <= time - velocityWindowMs) {
      motion.shift();
    }
  };

  const move = (event: PointerEvent, delta: CameraDelta) => {
    apply({ ...delta, gesturePhase: GesturePhase.update });
    const last = motion.at(-1)!;
    motion.push({
      time: event.timeStamp,
      pan: {
        x: last.pan.x + (delta.offset?.x ?? 0),
        y: last.pan.y + (delta.offset?.y ?? 0),
      },
      zoom: last.zoom + Math.log2(delta.scale ?? 1),
      bearing: last.bearing + (delta.bearing ?? 0),
      pitch: last.pitch + (delta.pitch ?? 0),
    });
    trim(event.timeStamp);
  };

  canvas.addEventListener("pointerdown", (event) => {
    canvas.setPointerCapture(event.pointerId);
    if (pointers.size === 0) {
      rotating =
        event.pointerType === "mouse" && (event.button === 2 || event.ctrlKey);
      motion = [
        {
          time: event.timeStamp,
          pan: { x: 0, y: 0 },
          zoom: 0,
          bearing: 0,
          pitch: 0,
        },
      ];
      anchor = undefined;
      apply({ gesturePhase: GesturePhase.begin });
    }
    pointers.set(event.pointerId, point(event));
  });

  canvas.addEventListener("pointermove", (event) => {
    const previous = pointers.get(event.pointerId);
    if (previous === undefined) return;
    const current = point(event);
    const others = [...pointers].filter(([id]) => id !== event.pointerId);
    pointers.set(event.pointerId, current);

    const dx = current.x - previous.x;
    const dy = current.y - previous.y;
    const other = others[0]?.[1];
    if (other !== undefined) {
      // The pointers' midpoint moves by half of this pointer's movement, and
      // the span between them scales and turns about the other pointer.
      const before = { x: previous.x - other.x, y: previous.y - other.y };
      const after = { x: current.x - other.x, y: current.y - other.y };
      const scale =
        Math.hypot(after.x, after.y) / Math.hypot(before.x, before.y);
      const turn =
        (Math.atan2(after.y, after.x) - Math.atan2(before.y, before.x)) *
        (180 / Math.PI);
      anchor = { x: (current.x + other.x) / 2, y: (current.y + other.y) / 2 };
      move(event, {
        offset: { x: dx / 2, y: dy / 2 },
        scale: Number.isFinite(scale) && scale > 0 ? scale : 1,
        bearing: -wrapDegrees(turn),
        anchor,
      });
    } else if (rotating) {
      move(event, { bearing: dx * 0.5, pitch: -dy * 0.5 });
    } else {
      move(event, { offset: { x: dx, y: dy } });
    }
  });

  canvas.addEventListener("pointerup", (event) => {
    if (!pointers.delete(event.pointerId) || pointers.size > 0) return;
    trim(event.timeStamp);
    const start = motion[0]!;
    const end = motion.at(-1)!;
    const seconds =
      Math.min(event.timeStamp - start.time, velocityWindowMs) / 1000;
    const speed = (from: number, to: number) =>
      seconds > 0 ? (to - from) / seconds : 0;
    apply({
      momentum: {
        pan: {
          velocity: {
            x: speed(start.pan.x, end.pan.x),
            y: speed(start.pan.y, end.pan.y),
          },
        },
        zoom: { velocity: speed(start.zoom, end.zoom), anchor },
        // The snap settles the bearing on north even when the gesture didn't
        // turn the map, so only a turn asks for it.
        bearing: {
          velocity: speed(start.bearing, end.bearing),
          anchor,
          snap: end.bearing === 0 ? undefined : {},
        },
        pitch: { velocity: speed(start.pitch, end.pitch) },
      },
      gesturePhase: GesturePhase.end,
    });
  });

  canvas.addEventListener("pointercancel", (event) => {
    if (pointers.delete(event.pointerId) && pointers.size === 0) {
      apply({ gesturePhase: GesturePhase.cancel });
    }
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
      const rate = event.ctrlKey ? 1 / 100 : 1 / 300;
      apply({ scale: 2 ** (-pixels * rate), anchor: point(event) });
    },
    { passive: false },
  );

  canvas.addEventListener("dblclick", (event) => {
    apply({
      scale: event.shiftKey ? 0.5 : 2,
      anchor: point(event),
      animation: { durationMs: 300 },
    });
  });

  canvas.addEventListener("keydown", (event) => {
    const step = 100;
    const animation = { durationMs: 300 };
    const deltas: Record<string, CameraDelta> = {
      ArrowLeft: { offset: { x: step, y: 0 }, animation },
      ArrowRight: { offset: { x: -step, y: 0 }, animation },
      ArrowUp: { offset: { x: 0, y: step }, animation },
      ArrowDown: { offset: { x: 0, y: -step }, animation },
      "+": { scale: 2, animation },
      "=": { scale: 2, animation },
      "-": { scale: 0.5, animation },
    };
    const delta = deltas[event.key];
    if (delta === undefined) return;
    event.preventDefault();
    apply(delta);
  });
}

/** An angle in degrees, wrapped to [-180, 180). */
function wrapDegrees(degrees: number): number {
  return ((((degrees + 180) % 360) + 360) % 360) - 180;
}
