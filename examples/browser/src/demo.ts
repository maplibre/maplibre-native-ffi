// The demo page: a continuous map on a full-page canvas, which the pointer,
// the wheel, and the keyboard move. Each backend's page supplies how a session
// attaches to the canvas, and everything else is shared.

import {
  CameraUpdateMode,
  type CameraDelta,
  type CameraOptions,
  FrameDemandFlag,
  GesturePhase,
  loadModule,
  type LogicalSize,
  type MapHandle,
  MapMode,
  type RenderSessionHandle,
  RenderResult,
  RuntimeEventFlag,
  RuntimeEventType,
  RuntimeHandle,
  type ScreenPoint,
} from "@maplibre/native-ffi";

import type { Comparison } from "./compare.ts";

import "./demo.css";

const styleUrl = "https://tiles.openfreemap.org/styles/liberty";

/** How a backend's page attaches a session to the canvas. */
export interface Backend {
  readonly id: "webgl" | "webgpu";
  /**
   * Gets what the backend needs from the browser, and returns how a session
   * attaches to `canvas` once the module loads. Throws when the browser can't
   * run the backend.
   */
  prepare(canvas: HTMLCanvasElement): Promise<Attach>;
}

/**
 * Attaches a session that presents to the canvas. `wakes` deliver the
 * session's wakes to the page.
 */
export type Attach = (
  map: MapHandle,
  logicalSize: LogicalSize,
  wakes: Wakes,
) => RenderSessionHandle;

export interface Wakes {
  readonly frame: () => void;
  readonly driverWork: () => void;
}

const status = document.querySelector<HTMLElement>("#status")!;
const canvas = document.querySelector<HTMLCanvasElement>("#map")!;

// Once the page shows an error, later progress doesn't replace it.
let failed = false;

function showStatus(message: string): void {
  if (failed) return;
  status.textContent = message;
  status.hidden = message === "";
}

function showError(message: string): void {
  showStatus(message);
  status.classList.add("error");
  failed = true;
}

/** Runs the demo with `backend`, and reports a failure on the page. */
export function run(backend: Backend): void {
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
  if (!crossOriginIsolated) {
    // On a first visit, coi-serviceworker installs its worker and reloads the
    // page, which the worker then isolates. A page that is still here after a
    // while has no worker to wait for.
    showStatus("Preparing the page…");
    await new Promise((resolve) => setTimeout(resolve, 5000));
    throw new Error(
      "This page isn't cross-origin isolated, so it can't share memory with the map's threads. Reload to try again.",
    );
  }
  const attach = await backend.prepare(canvas);
  showStatus("Loading the library…");
  // Each backend's page serves its own build of the module beside it.
  await loadModule(new URL("maplibre_native_ffi.mjs", location.href));
  const runtime = await RuntimeHandle.create();

  const map = runtime.createMap({
    initialExtent: { ...logicalSize(), scaleFactor },
    mapMode: MapMode.continuous,
    eventMask:
      RuntimeEventFlag.mapStyleLoaded |
      RuntimeEventFlag.mapLoadingFailed |
      RuntimeEventFlag.mapRenderError |
      RuntimeEventFlag.mapCameraIsChanging |
      RuntimeEventFlag.mapCameraDidChange,
  });
  runtime.discarding.setEventHandler({
    callback: (events) => {
      for (const event of events) {
        if (event.source !== map.id) continue;
        switch (event.type) {
          case RuntimeEventType.mapStyleLoaded:
            showStatus("");
            break;
          case RuntimeEventType.mapLoadingFailed:
          case RuntimeEventType.mapRenderError:
            showError(event.message);
            break;
          case RuntimeEventType.mapCameraIsChanging:
            comparison?.nativeCameraChanged();
            break;
          case RuntimeEventType.mapCameraDidChange:
            comparison?.nativeCameraChanged();
            scheduleHashUpdate(map);
            break;
        }
      }
    },
  });
  await map.ready;
  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: cameraFromHash() ?? {
      center: { latitude: 48.8584, longitude: 2.2945 },
      zoom: 15.5,
      bearing: -30,
      pitch: 55,
    },
  });
  map.discarding.setStyleUrl(styleUrl);
  showStatus("Loading the style…");

  let session: RenderSessionHandle | undefined;
  // The binding calls both wakes on this thread. The session runs its
  // graphics work here too, when the page services it.
  const wakes: Wakes = {
    frame: () => drainFrameResults(),
    driverWork: () => session?.serviceDriverWork(),
  };

  function demandFrame(): void {
    session?.demandFrame({ flags: FrameDemandFlag.waitForUpdate });
  }

  function drainFrameResults(): void {
    const results = session?.drainFrameResults() ?? [];
    if (results.length === 0) return;
    if (
      results.some(({ disposition }) => disposition === RenderResult.failed)
    ) {
      showError("The map failed to render a frame.");
    } else {
      requestAnimationFrame(demandFrame);
    }
  }

  session = attach(map, sizeCanvas(), wakes);
  await session.ready;
  demandFrame();

  new ResizeObserver(() => {
    session?.discarding.resize(sizeCanvas());
  }).observe(canvas);

  addGestures(map);
  addCompareToggle(map);
  canvas.focus();
}

// The comparison with MapLibre GL JS, while the page shows it. The page loads
// GL JS only for the comparison, and keeps it in the URL as ?compare.

let comparison: Comparison | undefined;

function addCompareToggle(map: MapHandle): void {
  const toggle = document.querySelector<HTMLInputElement>("#compare")!;
  const update = async () => {
    history.replaceState(
      null,
      "",
      toggle.checked
        ? "?compare" + location.hash
        : location.pathname + location.hash,
    );
    updateBackendLinks();
    comparison?.remove();
    comparison = undefined;
    if (!toggle.checked) return;
    const { compare } = await import("./compare.ts");
    // The toggle may have changed while GL JS loaded.
    if (toggle.checked && comparison === undefined) {
      comparison = compare(map, canvas, styleUrl);
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

/**
 * Sizes the canvas's drawing buffer to its logical size at the map's scale
 * factor, and returns the logical size.
 */
function sizeCanvas(): { width: number; height: number } {
  const size = logicalSize();
  canvas.width = Math.round(size.width * scaleFactor);
  canvas.height = Math.round(size.height * scaleFactor);
  return size;
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

function scheduleHashUpdate(map: MapHandle): void {
  clearTimeout(hashUpdate);
  hashUpdate = setTimeout(() => {
    const { center, zoom, bearing, pitch } = map.getSnapshot().view.camera;
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

function addGestures(map: MapHandle): void {
  const pointers = new Map<number, ScreenPoint>();
  let rotating = false;
  // The gesture's motion over the window before its latest move, oldest first.
  let motion: Motion[] = [];
  // The latest pinch's center, which its zoom and turn keep in place.
  let anchor: ScreenPoint | undefined;

  const apply = (delta: CameraDelta) => map.discarding.applyCameraDelta(delta);

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
