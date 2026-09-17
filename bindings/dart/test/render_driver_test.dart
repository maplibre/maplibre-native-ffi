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

final String? _metalTestSkipReason = !Platform.isMacOS
    ? 'requires a macOS Metal host'
    : !Maplibre.supportedRenderBackends().contains(RenderBackendMask.metal)
    ? 'requires a native library built with the Metal renderer'
    : null;

final class _MetalTestContext {
  _MetalTestContext._(this._objectiveCRuntime, this.device);

  static _MetalTestContext? create() {
    final metal = DynamicLibrary.open(
      '/System/Library/Frameworks/Metal.framework/Metal',
    );
    final create = metal.lookupFunction<_CreateDeviceNative, _CreateDevice>(
      'MTLCreateSystemDefaultDevice',
    );
    final objectiveCRuntime = DynamicLibrary.open('/usr/lib/libobjc.A.dylib');
    final device = create();
    return device == nullptr
        ? null
        : _MetalTestContext._(objectiveCRuntime, device);
  }

  final DynamicLibrary _objectiveCRuntime;
  final Pointer<Void> device;

  void close() =>
      _objectiveCRuntime.lookupFunction<_ReleaseObjectNative, _ReleaseObject>(
        'objc_release',
      )(device);
}

void main() {
  test(
    'public core-worker render workflow renders, reads, and queries',
    () => _withMetalMap(
      size: 32,
      body: (runtime, map, context, track) async {
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
        final session = attachment.session;
        track(session);
        await attachment.completed;
        expect(session.capabilities.textureRingDepth, 2);
        expect(session.snapshot().state, RenderSessionState.attached);

        session.requestFrame(
          const FrameDemand(renderIfNeeded: false, token: 17),
        );
        await session.barrier();
        final results = session.drainFrameResults();
        expect(results, hasLength(1));
        expect(results.single.token, 17);
        expect(results.single.disposition, RenderResult.rendered);
        expect(results.single.frameGeneration, greaterThan(0));

        final frame = session.acquireFrame()!;
        expect(frame.result.frameGeneration, results.single.frameGeneration);
        expect(frame.producerSync.kind, const GpuSync.cpuComplete().kind);
        expect(frame.metalTexture.unsafeTexture.isNull, isFalse);
        frame.release();
        // The ring holds one rendered slot, so a second lease finds none.
        expect(session.acquireFrame(), isNull);
        expect(session.drainFrameResults(), isEmpty);

        final image = await session.readPremultipliedRgba8();
        expect(image.info.width, 32);
        expect(image.info.height, 32);
        expect(image.bytes, hasLength(image.info.byteLength));

        const queryPoint = RenderedQueryPoint(ScreenPoint(16, 16));
        final deadline = DateTime.now().add(const Duration(seconds: 10));
        var query = await session.queryRenderedFeatures(queryPoint);
        while (query.isEmpty) {
          if (DateTime.now().isAfter(deadline)) {
            fail('the rendered circle never became queryable');
          }
          session.requestFrame();
          await session.barrier();
          query = await session.queryRenderedFeatures(queryPoint);
        }
        final hit = query.single;
        expect(hit.sourceId, 'points');
        expect(
          (jsonDecode(utf8.decode(hit.feature))
              as Map<Object?, Object?>)['properties'],
          containsPair('name', 'origin'),
        );

        // BND-183: the session keeps the scale factor it attached with, and
        // the rejection is synchronous. A resize that keeps the scale factor
        // publishes a new extent generation.
        expect(
          () => session.resize(
            const RenderTargetExtent(width: 32, height: 32, scaleFactor: 2),
          ),
          throwsA(isA<InvalidArgumentException>()),
        );
        final extentBeforeResize = session.snapshot().extentGeneration;
        await session.resize(const RenderTargetExtent(width: 48, height: 24));
        expect(
          session.snapshot().extentGeneration,
          greaterThan(extentBeforeResize),
        );

        await session.detach();
      },
    ),
    skip: _metalTestSkipReason,
  );

  test(
    'caller driver services work and abandons cleanly',
    () => _withMetalMap(
      size: 16,
      body: (runtime, map, context, track) async {
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
        final session = attachment.session;
        track(session);
        final attachWorkReady = session.driverWorkReady.first;
        await attachWorkReady.timeout(const Duration(seconds: 5));
        expect(session.serviceDriverWork(), greaterThan(0));
        await attachment.completed;
        expect(session.capabilities.driver, RenderDriver.callerGraphicsThread);

        map.setStyleJson(
          Uint8List.fromList(
            utf8.encode('{"version":8,"sources":{},"layers":[]}'),
          ),
        );
        await runtime.barrier();

        final frameResultsReady = session.frameResultsReady.first;
        session.requestFrame(
          const FrameDemand(renderIfNeeded: false, token: 23),
        );
        expect(session.serviceDriverWork(), greaterThan(0));
        await frameResultsReady.timeout(const Duration(seconds: 5));
        final result = session.drainFrameResults().single;
        expect(result.token, 23);
        expect(result.disposition, RenderResult.rendered);

        final frame = session.acquireFrame()!;
        frame.release();
        expect(session.serviceDriverWork(), greaterThan(0));
        expect(() => frame.result, throwsA(isA<MaplibreException>()));

        session.abandon();
        expect(session.snapshot().state, RenderSessionState.abandoned);
      },
    ),
    skip: _metalTestSkipReason,
  );
}

/// Runs [body] against a Metal-backed map, then tears down whatever it tracked.
///
/// A session passed to `track` is abandoned when it is still attached, then
/// closed, so a body only has to assert the teardown step it is testing.
Future<void> _withMetalMap({
  required int size,
  required Future<void> Function(
    RuntimeHandle runtime,
    MapHandle map,
    _MetalTestContext context,
    void Function(RenderSessionHandle session) track,
  )
  body,
}) async {
  final context = _MetalTestContext.create();
  if (context == null) {
    fail('MTLCreateSystemDefaultDevice returned nil');
  }
  final runtime = RuntimeHandle.create();
  final map = await MapHandle.create(
    runtime,
    options: MapOptions(width: size, height: size),
  );
  RenderSessionHandle? tracked;
  try {
    await body(runtime, map, context, (session) => tracked = session);
  } finally {
    final session = tracked;
    if (session != null) {
      try {
        session.abandon();
      } on MaplibreException catch (_) {}
      session.close();
    }
    await map.close();
    await runtime.close();
    context.close();
  }
}
