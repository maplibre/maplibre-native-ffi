import 'generated_workflows.dart';
import 'dart:async';
import 'dart:convert';
import 'support/owned_texture.dart';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  // EGL displays are process resources, so these workflows run in one isolate.
  _registerProjectionTest();
  test(
    'public core-worker render workflow renders, reads, and queries',
    () => _withMap(
      size: 32,
      body: (runtime, map, context, track) async {
        final style = Uint8List.fromList(
          utf8.encode('''
{"version":8,"sources":{"points":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","properties":{"name":"origin"},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"points","type":"circle","source":"points","paint":{"circle-radius":8,"circle-color":"#ff0000"}}]}
'''),
        );
        map.setStyleJson(style);
        await runtime.barrier();

        final attachment = context.attach(
          map,
          const RenderTargetExtent(width: 32, height: 32, scaleFactor: 1),
          const RenderSessionAttachOptions(
            driver: RenderDriverKind.coreWorker,
            requestedTextureRingDepth: 2,
          ),
        );
        final session = attachment.session;
        track(session);
        await attachment.completed;
        expect(
          session.getCapabilities().textureRingDepth,
          session.getCapabilities().flags.contains(
                RenderSessionCapabilityFlag.frameAcquisition,
              )
              ? 2
              : 1,
        );
        expect(session.getSnapshot().state, RenderSessionState.attached);

        session.requestFrame(
          FrameDemand(
            token: BigInt.from(17),
            coalescingBoundary: BigInt.zero,
            timeoutNs: BigInt.zero,
          ),
        );
        await session.barrier();
        final results = session.drainCopiedFrameResults();
        expect(results, hasLength(1));
        expect(results.single.token, BigInt.from(17));
        expect(results.single.disposition, RenderResult.rendered);
        expect(results.single.frameGeneration, greaterThan(BigInt.zero));

        if (session.getCapabilities().flags.contains(
          RenderSessionCapabilityFlag.frameAcquisition,
        )) {
          final frame = session.tryAcquireFrame()!;
          expect(
            frame.getResult().frameGeneration,
            results.single.frameGeneration,
          );
          expect(
            frame.getProducerSync().withView((sync) => sync.kind),
            GpuSyncKind.cpuComplete,
          );
          if (supportedRenderBackendMask().contains(RenderBackendFlag.metal)) {
            final textureView = frame.getMetalTexture();
            expect(
              () => textureView.texture,
              throwsA(isA<MaplibreException>()),
            );
            final pointer = textureView.withView((texture) {
              expect(texture.texture.isNull, isFalse);
              expect(
                () => frame.release(gpuSyncDefault()),
                throwsA(isA<MaplibreException>()),
              );
              expect(
                () => session.abandon(),
                throwsA(isA<MaplibreException>()),
              );
              return texture.texture;
            });
            expect(() => pointer.address, throwsA(isA<MaplibreException>()));
            frame.release(gpuSyncDefault());
            expect(
              () => textureView.withView((view) => view.width),
              throwsA(isA<MaplibreException>()),
            );
          } else if (supportedRenderBackendMask().contains(
            RenderBackendFlag.vulkan,
          )) {
            final textureView = frame.getVulkanTexture();
            textureView.withView((texture) {
              expect(texture.width, 32);
              expect(texture.height, 32);
              expect(
                () => frame.release(gpuSyncDefault()),
                throwsA(isA<MaplibreException>()),
              );
              expect(
                () => session.abandon(),
                throwsA(isA<MaplibreException>()),
              );
            });
            frame.release(gpuSyncDefault());
            expect(
              () => textureView.withView((texture) => texture.width),
              throwsA(isA<MaplibreException>()),
            );
          } else {
            final textureView = frame.getOpenglTexture();
            textureView.withView((texture) {
              expect(texture.width, 32);
              expect(texture.height, 32);
              expect(texture.texture, greaterThan(0));
              expect(
                () => frame.release(gpuSyncDefault()),
                throwsA(isA<MaplibreException>()),
              );
              expect(
                () => session.abandon(),
                throwsA(isA<MaplibreException>()),
              );
            });
            frame.release(gpuSyncDefault());
            expect(
              () => textureView.withView((texture) => texture.width),
              throwsA(isA<MaplibreException>()),
            );
          }
          // The ring holds one rendered slot, so a second lease finds none.
          expect(session.tryAcquireFrame(), isNull);
          expect(session.drainCopiedFrameResults(), isEmpty);
        }
        final image = await session.textureReadPremultipliedRgba8();
        expect(image.info.width, 32);
        expect(image.info.height, 32);
        expect(image.data, hasLength(image.info.byteLength));

        const queryPoint = RenderedQueryGeometry.point(ScreenPoint(16, 16));
        final deadline = DateTime.now().add(const Duration(seconds: 10));
        var query = await session.queryRenderedFeatures(queryPoint);
        while (query.isEmpty) {
          if (DateTime.now().isAfter(deadline)) {
            fail('the rendered circle never became queryable');
          }
          session.requestFrame(
            FrameDemand(
              flags: FrameDemandFlag.ifNeeded,
              token: BigInt.zero,
              coalescingBoundary: BigInt.zero,
              timeoutNs: BigInt.zero,
            ),
          );
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

        // the session keeps the scale factor it attached with, and
        // the rejection is synchronous. A resize that keeps the scale factor
        // publishes a new extent generation.
        expect(
          () => session.resize(
            const RenderTargetExtent(width: 32, height: 32, scaleFactor: 2),
          ),
          throwsA(isA<InvalidArgumentException>()),
        );
        final extentBeforeResize = session.getSnapshot().extentGeneration;
        await session.resize(
          const RenderTargetExtent(width: 48, height: 24, scaleFactor: 1),
        );
        expect(
          session.getSnapshot().extentGeneration,
          greaterThan(extentBeforeResize),
        );

        await session.detach();
      },
    ),
  );

  test(
    'caller driver services work and abandons cleanly',
    () => _withMap(
      size: 16,
      body: (runtime, map, context, track) async {
        final driverWake = Completer<void>();
        final frameWake = Completer<void>();
        final attachment = context.attach(
          map,
          const RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          RenderSessionAttachOptions(
            driver: RenderDriverKind.callerGraphicsThread,
            requestedTextureRingDepth: 2,
            driverWorkWake: Wake(
              callback: () {
                if (!driverWake.isCompleted) driverWake.complete();
              },
            ),
            frameWake: Wake(
              callback: () {
                if (!frameWake.isCompleted) frameWake.complete();
              },
            ),
          ),
        );
        final session = attachment.session;
        track(session);
        final attachWorkReady = driverWake.future;
        await attachWorkReady.timeout(const Duration(seconds: 5));
        expect(session.serviceDriverWork(0), greaterThan(0));
        await attachment.completed;
        expect(
          session.getCapabilities().driver,
          RenderDriverKind.callerGraphicsThread,
        );

        map.setStyleJson(
          Uint8List.fromList(
            utf8.encode('{"version":8,"sources":{},"layers":[]}'),
          ),
        );
        await runtime.barrier();

        final frameResultsReady = frameWake.future;
        session.requestFrame(
          FrameDemand(
            token: BigInt.from(23),
            coalescingBoundary: BigInt.zero,
            timeoutNs: BigInt.zero,
          ),
        );
        expect(session.serviceDriverWork(0), greaterThan(0));
        await frameResultsReady.timeout(const Duration(seconds: 5));
        final result = session.drainCopiedFrameResults().single;
        expect(result.token, BigInt.from(23));
        expect(result.disposition, RenderResult.rendered);

        if (session.getCapabilities().flags.contains(
          RenderSessionCapabilityFlag.frameAcquisition,
        )) {
          final frame = session.tryAcquireFrame()!;
          frame.release(gpuSyncDefault());
          expect(session.serviceDriverWork(0), greaterThan(0));
          expect(() => frame.getResult(), throwsA(isA<MaplibreException>()));
        }
        session.abandon();
        expect(session.getSnapshot().state, RenderSessionState.abandoned);
      },
    ),
  );
}

/// Runs [body] against the selected backend and tears down tracked sessions.
///
/// A session passed to `track` is abandoned when it is still attached, then
/// closed, so a body only has to assert the teardown step it is testing.
Future<void> _withMap({
  required int size,
  required Future<void> Function(
    RuntimeHandle runtime,
    MapHandle map,
    OwnedTextureContext context,
    void Function(RenderSessionHandle session) track,
  )
  body,
}) async {
  final context = OwnedTextureContext.create();
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final map = await runtime.mapCreate(
    MapOptions(
      initialExtent: LogicalExtent(width: size, height: size, scaleFactor: 1),
      eventMask: RuntimeEventMask.all,
    ),
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

void _registerProjectionTest() {
  test('projection captures rendered camera and outlives session', () async {
    final context = OwnedTextureContext.create();
    addTearDown(context.close);
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(runtime.close);
    final map = await runtime.createMap(
      options: const MapOptions(
        initialExtent: LogicalExtent(width: 128, height: 64, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
      ),
    );
    addTearDown(map.close);
    final attachment = context.attach(
      map,
      const RenderTargetExtent(width: 128, height: 64, scaleFactor: 1),
      const RenderSessionAttachOptions(driver: RenderDriverKind.coreWorker),
    );
    final session = attachment.session;
    addTearDown(() {
      if (!session.isClosed) {
        session.abandon();
        session.close();
      }
    });
    await attachment.completed;
    expect(session.projectionCreate, throwsA(isA<InvalidStateException>()));
    await map.setStyleJson(
      Uint8List.fromList(utf8.encode('{"version":8,"sources":{},"layers":[]}')),
    );
    await runtime.barrier();
    const coordinate = LatLng(37.78, -122.41);
    await map.updateCamera(
      CameraUpdate(
        camera: const CameraOptions(
          center: LatLng(37.7749, -122.4194),
          zoom: 12,
          bearing: 23,
          pitch: 40,
        ),
      ),
    );
    await runtime.barrier();
    final expected = await map.pixelForLatLng(coordinate);
    session.requestFrame(
      FrameDemand(
        token: BigInt.from(1),
        coalescingBoundary: BigInt.zero,
        timeoutNs: BigInt.zero,
      ),
    );
    await session.barrier();
    expect(
      session.drainCopiedFrameResults().single.disposition,
      RenderResult.rendered,
    );
    await map.updateCamera(
      CameraUpdate(camera: const CameraOptions(center: LatLng(37.80, -122.45))),
    );
    await runtime.barrier();
    final projection = session.projectionCreate();
    addTearDown(projection.close);
    _expectPoint(expected, projection.pixelForLatLng(coordinate));
    expect(
      ((await map.pixelForLatLng(coordinate)).x - expected.x).abs(),
      greaterThan(1),
    );
    if (session.getCapabilities().flags.contains(
      RenderSessionCapabilityFlag.frameAcquisition,
    )) {
      final frame = session.tryAcquireFrame()!;
      try {
        final captured = session.projectionCreate();
        try {
          _expectPoint(expected, captured.pixelForLatLng(coordinate));
        } finally {
          captured.close();
        }
      } finally {
        frame.release(gpuSyncDefault());
      }
    }
    await session.resize(
      const RenderTargetExtent(width: 96, height: 48, scaleFactor: 1),
    );
    expect(session.projectionCreate, throwsA(isA<InvalidStateException>()));
    await runtime.barrier();
    session.requestFrame(
      FrameDemand(
        token: BigInt.from(1),
        coalescingBoundary: BigInt.zero,
        timeoutNs: BigInt.zero,
      ),
    );
    await session.barrier();
    expect(
      session.drainCopiedFrameResults().single.disposition,
      RenderResult.rendered,
    );
    final resized = session.projectionCreate();
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
  });
}

void _expectPoint(ScreenPoint expected, ScreenPoint actual) {
  expect(actual.x, closeTo(expected.x, 1e-6));
  expect(actual.y, closeTo(expected.y, 1e-6));
}
