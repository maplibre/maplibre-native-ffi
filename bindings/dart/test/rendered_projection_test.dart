import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  test(
    'projection captures rendered camera and outlives session',
    () {
      final metal = DynamicLibrary.open(
        '/System/Library/Frameworks/Metal.framework/Metal',
      );
      final createDevice = metal
          .lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
            'MTLCreateSystemDefaultDevice',
          );
      final release = DynamicLibrary.open('/usr/lib/libobjc.A.dylib')
          .lookupFunction<
            Void Function(Pointer<Void>),
            void Function(Pointer<Void>)
          >('objc_release');
      final device = createDevice();
      expect(device, isNot(nullptr));
      addTearDown(() => release(device));
      final runtime = RuntimeHandle.create();
      addTearDown(runtime.close);
      final map = runtime.createMap(
        options: const MapOptions(width: 128, height: 64),
      );
      addTearDown(map.close);
      final session = map.attachRef().attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent: const RenderTargetExtent(width: 128, height: 64),
          context: MetalContextDescriptor(
            device: NativePointer(device.address),
          ),
        ),
      );
      addTearDown(session.close);
      expect(session.createProjection, throwsA(isA<InvalidStateException>()));
      map.setStyleJson(
        Uint8List.fromList(
          utf8.encode('{"version":8,"sources":{},"layers":[]}'),
        ),
      );
      runtime.pump();
      const coordinate = LatLng(37.78, -122.41);
      map.jumpTo(
        const CameraOptions(
          center: LatLng(37.7749, -122.4194),
          zoom: 12,
          bearing: 23,
          pitch: 40,
        ),
      );
      runtime.pump();
      final expected = map.pixelForLatLng(coordinate);
      expect(session.renderUpdate().result, RenderResult.rendered);
      map.jumpTo(const CameraOptions(center: LatLng(37.80, -122.45)));
      runtime.pump();
      final projection = session.createProjection();
      addTearDown(projection.close);
      _expectPoint(expected, projection.pixelForLatLng(coordinate));
      expect(
        (map.pixelForLatLng(coordinate).x - expected.x).abs(),
        greaterThan(1),
      );
      final frame = session.acquireMetalTextureFrame();
      try {
        final captured = session.createProjection();
        try {
          _expectPoint(expected, captured.pixelForLatLng(coordinate));
        } finally {
          captured.close();
        }
      } finally {
        frame.close();
      }
      session.resize(96, 48, scaleFactor: 2);
      expect(session.createProjection, throwsA(isA<InvalidStateException>()));
      runtime.pump();
      expect(session.renderUpdate().result, RenderResult.rendered);
      final resized = session.createProjection();
      try {
        _expectPoint(
          map.pixelForLatLng(coordinate),
          resized.pixelForLatLng(coordinate),
        );
      } finally {
        resized.close();
      }
      session.close();
      map.close();
      runtime.close();
      _expectPoint(expected, projection.pixelForLatLng(coordinate));
    },
    skip: Maplibre.supportedRenderBackends().contains(RenderBackendMask.metal)
        ? false
        : 'This test attaches a Metal render target.',
  );
}

void _expectPoint(ScreenPoint expected, ScreenPoint actual) {
  expect(actual.x, closeTo(expected.x, 1e-6));
  expect(actual.y, closeTo(expected.y, 1e-6));
}
