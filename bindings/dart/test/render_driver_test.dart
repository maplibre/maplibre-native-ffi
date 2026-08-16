import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

typedef _CreateDeviceNative = Pointer<Void> Function();
typedef _CreateDevice = Pointer<Void> Function();
typedef _ReleaseObjectNative = Void Function(Pointer<Void>);
typedef _ReleaseObject = void Function(Pointer<Void>);

final class _MetalTestContext {
  _MetalTestContext._(this._library, this.device);

  static _MetalTestContext? create() {
    if (!Platform.isMacOS) return null;
    final prefix = File(
      '.dart_tool/maplibre_native_install_dir',
    ).readAsStringSync().trim();
    final library = DynamicLibrary.open(
      '$prefix/lib/libmaplibre_native_ffi_dart_test_support.dylib',
    );
    final create = library.lookupFunction<_CreateDeviceNative, _CreateDevice>(
      'mln_dart_test_metal_device_create',
    );
    final device = create();
    return device == nullptr ? null : _MetalTestContext._(library, device);
  }

  final DynamicLibrary _library;
  final Pointer<Void> device;

  void close() => _library.lookupFunction<_ReleaseObjectNative, _ReleaseObject>(
    'mln_dart_test_metal_object_release',
  )(device);
}

Future<void> _serviceAndComplete(
  RenderSessionHandle session,
  Future<void> operation,
) async {
  expect(session.serviceDriverWork(), greaterThan(0));
  await operation;
}

void main() {
  test('transferred WebGL canvases select dedicated ownership', () {
    const descriptor = WebGLContextDescriptor.transferredCanvas('#map');
    expect(descriptor.ownership, OpenGLContextOwnership.dedicated);
  });

  test(
    'public core-worker render workflow renders, reads, and queries',
    () async {
      final context = _MetalTestContext.create();
      if (context == null) return;

      final runtime = await RuntimeHandle.create();
      final map = await MapHandle.create(
        runtime,
        options: const MapOptions(width: 32, height: 32),
      );
      RenderSessionHandle? session;
      try {
        final style = Uint8List.fromList(
          utf8.encode('''
{"version":8,"sources":{"points":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","properties":{"name":"origin"},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"points","type":"circle","source":"points","paint":{"circle-radius":8,"circle-color":"#ff0000"}}]}
'''),
        );
        map.setStyleJson(style);
        await runtime.barrier();

        final attachment = map.attachMetalOwnedTexture(
          MetalOwnedTextureDescriptor(
            extent: const RenderTargetExtent(width: 32, height: 32),
            context: MetalContextDescriptor(
              device: NativePointer(context.device.address),
            ),
          ),
          options: const RenderSessionAttachOptions(
            driver: RenderDriver.coreWorker,
            requestedTextureRingDepth: 2,
          ),
        );
        session = attachment.session;
        await attachment.completed;
        expect(session.capabilities.textureRingDepth, inInclusiveRange(1, 3));

        session.requestFrame(
          const FrameDemand(renderIfNeeded: false, token: 17),
        );
        await session.barrier();
        final results = session.drainFrameResults();
        expect(results, hasLength(1));
        expect(results.single.token, 17);
        expect(results.single.disposition, same(RenderResult.rendered));
        expect(results.single.frameGeneration, greaterThan(0));

        final frame = session.acquireFrame();
        expect(frame.result.frameGeneration, results.single.frameGeneration);
        expect(frame.producerSync.kind, 0);
        expect(frame.metalTexture.unsafeTexture.isNull, isFalse);
        await frame.release();

        final image = await session.readPremultipliedRgba8();
        expect(image.info.width, 32);
        expect(image.info.height, 32);
        expect(image.bytes, hasLength(image.info.byteLength));

        final query = await session.queryRenderedFeatures(
          const RenderedQueryPoint(ScreenPoint(16, 16)),
        );
        for (final hit in query) {
          expect(
            jsonDecode(utf8.decode(hit.feature)),
            isA<Map<Object?, Object?>>(),
          );
        }

        await session.detach();
        session.close();
        session = null;
      } finally {
        if (session != null) {
          try {
            session.abandon();
            session.close();
          } catch (_) {}
        }
        await map.close();
        await runtime.close();
        context.close();
      }
    },
  );

  test('caller driver services work and abandons cleanly', () async {
    final context = _MetalTestContext.create();
    if (context == null) return;

    final runtime = await RuntimeHandle.create();
    final map = await MapHandle.create(
      runtime,
      options: const MapOptions(width: 16, height: 16),
    );
    RenderSessionHandle? session;
    try {
      final attachment = map.attachMetalOwnedTexture(
        MetalOwnedTextureDescriptor(
          extent: const RenderTargetExtent(width: 16, height: 16),
          context: MetalContextDescriptor(
            device: NativePointer(context.device.address),
          ),
        ),
        options: const RenderSessionAttachOptions(
          driver: RenderDriver.callerGraphicsThread,
          requestedTextureRingDepth: 2,
        ),
      );
      session = attachment.session;
      final attachWorkReady = session.driverWorkReady.first;
      await attachWorkReady.timeout(const Duration(seconds: 5));
      await _serviceAndComplete(session, attachment.completed);
      expect(session.capabilities.driver, RenderDriver.callerGraphicsThread);

      map.setStyleJson(
        Uint8List.fromList(
          utf8.encode('{"version":8,"sources":{},"layers":[]}'),
        ),
      );
      await runtime.barrier();

      final frameResultsReady = session.frameResultsReady.first;
      session.requestFrame(const FrameDemand(renderIfNeeded: false, token: 23));
      expect(session.serviceDriverWork(), greaterThan(0));
      await frameResultsReady.timeout(const Duration(seconds: 5));
      final result = session.drainFrameResults().single;
      expect(result.token, 23);
      expect(result.disposition, same(RenderResult.rendered));

      final frame = session.acquireFrame();
      final release = frame.release();
      await _serviceAndComplete(session, release);
      expect(() => frame.result, throwsA(isA<MaplibreException>()));

      final abandoned = session.abandon();
      expect(abandoned.quarantinedResourceCount, greaterThanOrEqualTo(0));
      session.close();
      session = null;
    } finally {
      if (session != null) {
        try {
          session.abandon();
          session.close();
        } catch (_) {}
      }
      await map.close();
      await runtime.close();
      context.close();
    }
  });
}
