// Snippets for the docs site's Camera page. They are analyzed with the tests,
// which never run them.

// ignore_for_file: unused_local_variable

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void useCamera(Camera camera) {}

void flyEnded(bool completed) {}

// #region gesture
void beginGesture(MapHandle map) {
  map.discarding.cancelTransitions();
  map.discarding.applyCameraDelta(
    const CameraDelta(gesturePhase: GesturePhase.begin),
  );
}

void applyDrag(MapHandle map, ScreenPoint movement) {
  map.discarding.applyCameraDelta(
    CameraDelta(offset: movement, gesturePhase: GesturePhase.update),
  );
}

void applyPinch(MapHandle map, double scale, ScreenPoint center) {
  map.discarding.applyCameraDelta(
    CameraDelta(
      scale: scale,
      anchor: center,
      gesturePhase: GesturePhase.update,
    ),
  );
}

void endGesture(MapHandle map) {
  map.discarding.applyCameraDelta(
    const CameraDelta(gesturePhase: GesturePhase.end),
  );
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
        endHandler: (end) {
          flyEnded(end.outcome == CameraTransitionOutcome.completed);
        },
      ),
    ),
  );
  // #endregion animate

  // #region fit
  const bounds = LatLngBounds(
    southwest: LatLng(latitude: 45.8, longitude: 5.9),
    northeast: LatLng(latitude: 47.8, longitude: 10.5),
  );
  final fitted = await map.cameraForLatLngBounds(bounds);

  map.discarding.updateCamera(
    CameraUpdate(
      mode: CameraUpdateMode.ease,
      camera: fitted,
      animation: const AnimationOptions(durationMs: 500),
    ),
  );
  // #endregion fit

  // #region limit
  map.discarding.setBounds(
    const BoundOptions(bounds: bounds, minZoom: 5, maxZoom: 16),
  );
  // #endregion limit

  beginGesture(map);
  applyDrag(map, const ScreenPoint(x: 12, y: -4));
  applyPinch(map, 1.1, const ScreenPoint(x: 200, y: 150));
  endGesture(map);

  // #region convert
  final projection = MapProjectionHandle.create(map.getSnapshot().view);

  final coordinate = projection.latLngForPixel(
    const ScreenPoint(x: 100, y: 200),
  );
  final point = projection.pixelForLatLng(coordinate);

  projection.close();
  // #endregion convert
}
