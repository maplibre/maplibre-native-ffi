// Snippets for the docs site's Camera page. They are type-checked with the
// tests, which never run them.

import {
  type Camera,
  CameraTransitionOutcome,
  CameraUpdateMode,
  type GestureRecognizerHandle,
  type GestureResult,
  type MapHandle,
  MapProjectionHandle,
  PointerKind,
  type ScreenPoint,
} from "../../src/index.ts";

function useCamera(_camera?: Camera): void {}

function flyEnded(_completed: boolean): void {}

// #region gesture
function createRecognizer(map: MapHandle): GestureRecognizerHandle {
  return map.gestureRecognizerCreate();
}

function scheduleTick(_result: GestureResult): void {
  // On the next frame, or at result.nextTickMs.
}

function pointerDown(
  recognizer: GestureRecognizerHandle,
  id: number,
  position: ScreenPoint,
  timeMs: number,
): void {
  const result = recognizer.handle(
    {
      kind: "pointerDown",
      value: { id, kind: PointerKind.touch, position },
    },
    timeMs,
  );
  scheduleTick(result);
}

function pointerMove(
  recognizer: GestureRecognizerHandle,
  id: number,
  position: ScreenPoint,
  timeMs: number,
): void {
  const result = recognizer.handle(
    { kind: "pointerMove", value: { id, position } },
    timeMs,
  );
  scheduleTick(result);
}

function tick(
  recognizer: GestureRecognizerHandle,
  timeMs: number,
): void {
  scheduleTick(recognizer.tick(timeMs));
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
    camera: {
      center: { latitude: 48.8566, longitude: 2.3522 },
      zoom: 12,
    },
  });
  // #endregion jump

  // #region animate
  map.discarding.updateCamera({
    mode: CameraUpdateMode.fly,
    camera: {
      center: { latitude: 40.7128, longitude: -74.006 },
      zoom: 10,
    },
    animation: {
      durationMs: 3000,
      // Runs on this thread, after the animation ends.
      endHandler: {
        callback: (end) => {
          flyEnded(end.outcome === CameraTransitionOutcome.completed);
        },
      },
    },
  });
  // #endregion animate

  // #region fit
  const bounds = {
    southwest: { latitude: 45.8, longitude: 5.9 },
    northeast: { latitude: 47.8, longitude: 10.5 },
  };
  map.discarding.fitCameraToLatLngBounds(bounds, {
    mode: CameraUpdateMode.ease,
    animation: { durationMs: 500 },
  });
  // #endregion fit

  // #region limit
  map.discarding.setBounds({ bounds, minZoom: 5, maxZoom: 16 });
  // #endregion limit

  const recognizer = createRecognizer(map);
  pointerDown(recognizer, 1, { x: 100, y: 100 }, 0);
  pointerMove(recognizer, 1, { x: 112, y: 96 }, 16);
  tick(recognizer, 32);
  recognizer.release();

  // #region convert
  const projection = MapProjectionHandle.create(
    map.getSnapshot().view,
  );

  const coordinate = projection.latLngForPixel({ x: 100, y: 200 });
  const point = projection.pixelForLatLng(coordinate);

  projection.release();
  // #endregion convert
  void point;
}
