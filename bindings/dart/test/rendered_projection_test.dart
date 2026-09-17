import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  test(
    'projection captures rendered camera and outlives session',
    () async {
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
      final map = await runtime.createMap(
        options: const MapOptions(width: 128, height: 64),
      );
      addTearDown(map.close);
      final attachment = map.attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent: const RenderTargetExtent(width: 128, height: 64),
          context: MetalContextDescriptor(
            device: NativePointer(device.address),
          ),
        ),
        options: const RenderSessionAttachOptions(
          driver: RenderDriver.coreWorker,
        ),
      );
      final session = attachment.session;
      addTearDown(() {
        if (!session.isClosed) {
          session.abandon();
          session.close();
        }
      });
      await attachment.completed;
      expect(session.createProjection, throwsA(isA<InvalidStateException>()));
      await map.setStyleJson(
        Uint8List.fromList(
          utf8.encode('{"version":8,"sources":{},"layers":[]}'),
        ),
      );
      await runtime.barrier();
      const coordinate = LatLng(37.78, -122.41);
      await map.updateCamera(
        const CameraOptions(
          center: LatLng(37.7749, -122.4194),
          zoom: 12,
          bearing: 23,
          pitch: 40,
        ),
      );
      await runtime.barrier();
      final expected = await map.pixelForLatLng(coordinate);
      session.requestFrame(const FrameDemand(renderIfNeeded: false, token: 1));
      await session.barrier();
      expect(
        session.drainFrameResults().single.disposition,
        RenderResult.rendered,
      );
      await map.updateCamera(
        const CameraOptions(center: LatLng(37.80, -122.45)),
      );
      await runtime.barrier();
      final projection = session.createProjection();
      addTearDown(projection.close);
      _expectPoint(expected, projection.pixelForLatLng(coordinate));
      expect(
        ((await map.pixelForLatLng(coordinate)).x - expected.x).abs(),
        greaterThan(1),
      );
      final frame = session.acquireFrame()!;
      try {
        final captured = session.createProjection();
        try {
          _expectPoint(expected, captured.pixelForLatLng(coordinate));
        } finally {
          captured.close();
        }
      } finally {
        frame.release();
      }
      await session.resize(const RenderTargetExtent(width: 96, height: 48));
      expect(session.createProjection, throwsA(isA<InvalidStateException>()));
      await runtime.barrier();
      session.requestFrame(const FrameDemand(renderIfNeeded: false, token: 1));
      await session.barrier();
      expect(
        session.drainFrameResults().single.disposition,
        RenderResult.rendered,
      );
      final resized = session.createProjection();
      try {
        _expectPoint(
          await map.pixelForLatLng(coordinate),
          resized.pixelForLatLng(coordinate),
        );
      } finally {
        resized.close();
      }
      await session.detach();
      session.close();
      await map.close();
      await runtime.close();
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
