import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void main() {
  final camera = CameraOptions(
    center: LatLng(latitude: 37.7749, longitude: -122.4194),
    zoom: 12,
  );
  final sameCamera = CameraOptions(
    center: LatLng(latitude: 37.7749, longitude: -122.4194),
    zoom: 12,
  );

  assert(camera == sameCamera);
}
