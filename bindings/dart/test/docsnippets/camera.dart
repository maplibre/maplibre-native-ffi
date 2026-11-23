// Snippets for the docs site's Camera page. They are analyzed with the tests,
// which never run them.

// ignore_for_file: unused_local_variable

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void useCamera(Camera camera) {}

void flyEnded(bool completed) {}

// #region gesture
GestureRecognizerHandle createRecognizer(MapHandle map) =>
    map.gestureRecognizerCreate();

void scheduleTick(GestureResult result) {
  // On the next frame, or at result.nextTickMs.
}

void pointerDown(
  GestureRecognizerHandle recognizer,
  int id,
  ScreenPoint position,
  double timeMs,
) {
  final result = recognizer.handle(
    GestureInput.pointerDown(
      GesturePointerDown(
        id: id,
        kind: PointerKind.touch,
        position: position,
      ),
    ),
    timeMs,
  );
  scheduleTick(result);
}

void pointerMove(
  GestureRecognizerHandle recognizer,
  int id,
  ScreenPoint position,
  double timeMs,
) {
  final result = recognizer.handle(
    GestureInput.pointerMove(
      GesturePointer(id: id, position: position),
    ),
    timeMs,
  );
  scheduleTick(result);
}

void tick(GestureRecognizerHandle recognizer, double timeMs) {
  scheduleTick(recognizer.tick(timeMs));
}
// #endregion gesture

Future<void> camera(MapHandle map) async {
  // #region read
  final snapshot = map.getSnapshot();
  useCamera(snapshot.view.camera);

  final result = await map.getCamera();
  useCamera(result.camera);
  // #endregion read

  // #region jump
  map.discarding.updateCamera(
    const CameraUpdate(
      mode: CameraUpdateMode.jump,
      camera: CameraOptions(
        center: LatLng(latitude: 48.8566, longitude: 2.3522),
        zoom: 12,
      ),
    ),
  );
  // #endregion jump

  // #region animate
  map.discarding.updateCamera(
    CameraUpdate(
      mode: CameraUpdateMode.fly,
      camera: const CameraOptions(
        center: LatLng(latitude: 40.7128, longitude: -74.006),
        zoom: 10,
      ),
      animation: AnimationOptions(
        durationMs: 3000,
        // Runs in this isolate, after the animation ends.
        endHandler: CameraTransitionHandler(
          callback: (end) {
            flyEnded(
              end.outcome == CameraTransitionOutcome.completed,
            );
          },
        ),
      ),
    ),
  );
  // #endregion animate

  // #region fit
  const bounds = LatLngBounds(
    southwest: LatLng(latitude: 45.8, longitude: 5.9),
    northeast: LatLng(latitude: 47.8, longitude: 10.5),
  );
  map.discarding.fitCameraToLatLngBounds(
    bounds,
    const CameraFitUpdate(
      mode: CameraUpdateMode.ease,
      animation: AnimationOptions(durationMs: 500),
    ),
  );
  // #endregion fit

  // #region limit
  map.discarding.setBounds(
    const BoundOptions(bounds: bounds, minZoom: 5, maxZoom: 16),
  );
  // #endregion limit

  final recognizer = createRecognizer(map);
  pointerDown(recognizer, 1, const ScreenPoint(x: 100, y: 100), 0);
  pointerMove(recognizer, 1, const ScreenPoint(x: 112, y: 96), 16);
  tick(recognizer, 32);
  recognizer.release();

  // #region convert
  final projection = MapProjectionHandle.create(
    map.getSnapshot().view,
  );

  final coordinate = projection.latLngForPixel(
    const ScreenPoint(x: 100, y: 200),
  );
  final point = projection.pixelForLatLng(coordinate);

  projection.release();
  // #endregion convert
}
