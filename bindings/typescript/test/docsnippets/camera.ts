// Snippets for the docs site's Camera page. They are type-checked with the
// tests, which never run them.

import {
  type Camera,
  CameraTransitionOutcome,
  CameraUpdateMode,
  GesturePhase,
  type MapHandle,
  MapProjectionHandle,
  type ScreenPoint,
} from "../../src/index.ts";

function useCamera(_camera?: Camera): void {}

function flyEnded(_completed: boolean): void {}

// #region gesture
function beginGesture(map: MapHandle): void {
  map.discarding.cancelTransitions();
  map.discarding.applyCameraDelta({ gesturePhase: GesturePhase.begin });
}

function applyDrag(map: MapHandle, movement: ScreenPoint): void {
  map.discarding.applyCameraDelta({
    offset: movement,
    gesturePhase: GesturePhase.update,
  });
}

function applyPinch(map: MapHandle, scale: number, center: ScreenPoint): void {
  map.discarding.applyCameraDelta({
    scale,
    anchor: center,
    gesturePhase: GesturePhase.update,
  });
}

function endGesture(map: MapHandle): void {
  map.discarding.applyCameraDelta({ gesturePhase: GesturePhase.end });
}
// #endregion gesture

export async function camera(map: MapHandle): Promise<void> {
  // #region read
  const snapshot = map.getSnapshot();
  useCamera(snapshot.view.camera);

  const result = await map.getCamera();
  useCamera(result.camera);
  // #endregion read

  // #region jump
  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: { center: { latitude: 48.8566, longitude: 2.3522 }, zoom: 12 },
  });
  // #endregion jump

  // #region animate
  map.discarding.updateCamera({
    mode: CameraUpdateMode.fly,
    camera: { center: { latitude: 40.7128, longitude: -74.006 }, zoom: 10 },
    animation: {
      durationMs: 3000,
      // Runs on this thread, after the animation ends.
      endHandler: (end) => {
        flyEnded(end.outcome === CameraTransitionOutcome.completed);
      },
    },
  });
  // #endregion animate

  // #region fit
  const bounds = {
    southwest: { latitude: 45.8, longitude: 5.9 },
    northeast: { latitude: 47.8, longitude: 10.5 },
  };
  const fitted = await map.cameraForLatLngBounds(bounds);

  map.discarding.updateCamera({
    mode: CameraUpdateMode.ease,
    camera: fitted,
    animation: { durationMs: 500 },
  });
  // #endregion fit

  // #region limit
  map.discarding.setBounds({ bounds, minZoom: 5, maxZoom: 16 });
  // #endregion limit

  beginGesture(map);
  applyDrag(map, { x: 12, y: -4 });
  applyPinch(map, 1.1, { x: 200, y: 150 });
  endGesture(map);

  // #region convert
  const projection = MapProjectionHandle.create(map.getSnapshot().view);

  const coordinate = projection.latLngForPixel({ x: 100, y: 200 });
  const point = projection.pixelForLatLng(coordinate);

  projection.close();
  // #endregion convert
  void point;
}
