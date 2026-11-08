// The comparison with MapLibre GL JS: a GL JS map over the demo's map, with
// the same style and camera. A divider clips the GL JS map to its right, so
// each side shows one map, and each map takes the input on its own side.
//
// The map that last took input leads, and the other jumps to its camera. A
// follower's camera changes don't move the leader, so the two maps don't echo
// each other's changes.

import { CameraUpdateMode, type MapHandle } from "@maplibre/native-ffi";
import { getVersion, Map as GlMap, setWorkerUrl } from "maplibre-gl";

import "maplibre-gl/dist/maplibre-gl.css";
import workerUrl from "maplibre-gl/dist/maplibre-gl-worker.mjs?url";

// The bundle moves GL JS away from its worker, so the page tells it where the
// worker is.
setWorkerUrl(workerUrl);

export interface Comparison {
  /** Moves the GL JS map to the native map's camera, if the native map leads. */
  readonly nativeCameraChanged: () => void;
  /** Removes the GL JS map and the divider. */
  readonly remove: () => void;
}

/**
 * Shows a GL JS map with `styleUrl` over the native map's `canvas`, and keeps
 * the two cameras together.
 */
export function compare(
  map: MapHandle,
  canvas: HTMLCanvasElement,
  styleUrl: string,
): Comparison {
  const container = document.createElement("div");
  container.id = "gl-map";
  const divider = document.createElement("div");
  divider.className = "divider";
  divider.tabIndex = 0;
  divider.role = "slider";
  divider.ariaLabel = "Divider between the maps";
  divider.ariaValueMin = "0";
  divider.ariaValueMax = "100";
  const nativeLabel = label("compare-label native", "MapLibre Native FFI");
  const glLabel = label("compare-label gl", `MapLibre GL JS ${getVersion()}`);
  canvas.after(container, divider, nativeLabel, glLabel);

  const { view, bounds } = map.getSnapshot();
  const { center, zoom, bearing, pitch, roll } = view.camera;
  const gl = new GlMap({
    container,
    style: styleUrl,
    center: [center.longitude, center.latitude],
    zoom,
    bearing,
    pitch,
    roll,
    minZoom: bounds.minZoom,
    maxZoom: bounds.maxZoom,
    minPitch: bounds.minPitch,
    maxPitch: bounds.maxPitch,
    // The page shows the style's attribution for both maps.
    attributionControl: false,
  });
  gl.on("error", ({ error }) => console.error("MapLibre GL JS:", error));

  let leader: "native" | "gl" = "native";
  const listening = new AbortController();
  const { signal } = listening;

  // Input on a map makes it lead, and stops the other map's motion, such as
  // the momentum after a drag. The capture phase sees the input before either
  // map's own handlers do.
  for (const type of ["pointerdown", "wheel", "keydown"]) {
    canvas.addEventListener(
      type,
      () => {
        leader = "native";
        gl.stop();
      },
      { capture: true, signal },
    );
    container.addEventListener(
      type,
      () => {
        leader = "gl";
        map.discarding.cancelCameraTransitions();
      },
      { capture: true, signal },
    );
  }

  gl.on("move", () => {
    if (leader !== "gl") return;
    const { lng, lat } = gl.getCenter();
    map.discarding.updateCamera({
      mode: CameraUpdateMode.jump,
      camera: {
        center: { latitude: lat, longitude: lng },
        zoom: gl.getZoom(),
        bearing: gl.getBearing(),
        pitch: gl.getPitch(),
        roll: gl.getRoll(),
      },
    });
  });

  addDividerHandlers(divider, signal);

  return {
    nativeCameraChanged() {
      if (leader !== "native") return;
      const { center, zoom, bearing, pitch, roll } =
        map.getSnapshot().view.camera;
      gl.jumpTo({
        center: [center.longitude, center.latitude],
        zoom,
        bearing,
        pitch,
        roll,
      });
    },
    remove() {
      listening.abort();
      gl.remove();
      for (const element of [container, divider, nativeLabel, glLabel]) {
        element.remove();
      }
    },
  };
}

// Where the divider sits, as a fraction of the page's width. It stays when the
// comparison closes, for the next time it opens.
let split = 0.5;

function setSplit(divider: HTMLElement, fraction: number): void {
  split = Math.min(1, Math.max(0, fraction));
  document.documentElement.style.setProperty("--split", `${split * 100}%`);
  divider.ariaValueNow = String(Math.round(split * 100));
}

/** Lets a drag or the arrow keys move the divider. */
function addDividerHandlers(divider: HTMLElement, signal: AbortSignal): void {
  setSplit(divider, split);
  divider.addEventListener(
    "pointerdown",
    (event) => {
      divider.setPointerCapture(event.pointerId);
    },
    { signal },
  );
  divider.addEventListener(
    "pointermove",
    (event) => {
      if (!divider.hasPointerCapture(event.pointerId)) return;
      setSplit(divider, event.clientX / innerWidth);
    },
    { signal },
  );
  divider.addEventListener(
    "keydown",
    (event) => {
      const step = event.shiftKey ? 0.1 : 0.02;
      const steps: Record<string, number> = {
        ArrowLeft: split - step,
        ArrowRight: split + step,
        Home: 0,
        End: 1,
      };
      const next = steps[event.key];
      if (next === undefined) return;
      event.preventDefault();
      setSplit(divider, next);
    },
    { signal },
  );
}

function label(className: string, text: string): HTMLElement {
  const element = document.createElement("div");
  element.className = className;
  element.textContent = text;
  return element;
}
