import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void main() {
  const camera = CameraOptions(center: LatLng(37.7749, -122.4194), zoom: 12);
  const sameCamera = CameraOptions(
    center: LatLng(37.7749, -122.4194),
    zoom: 12,
  );

  assert(camera == sameCamera);
}
