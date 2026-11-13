// Render sessions on a core worker: rendering and readback offscreen,
// disposal and release of an attached session, and the scope of a frame view of
// a borrowed texture.
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/backend.dart';
import 'support/fixture.dart';
import 'support/graphics.dart';
import 'support/render.dart';

const _redStyle = {
  'version': 8,
  'sources': <String, Object?>{},
  'layers': [
    {
      'id': 'background',
      'type': 'background',
      'paint': {'background-color': '#ff0000'},
    },
  ],
};

void _expectRed(TextureReadbackResult image) {
  expect(image.info.width, renderSize);
  expect(image.info.height, renderSize);
  final center = (renderSize ~/ 2) * image.info.stride + (renderSize ~/ 2) * 4;
  expect(image.data.sublist(center, center + 4), [255, 0, 0, 255]);
}

/// An OpenGL session exposes frames only from a context that it shares with
/// the host, which only a caller-driven session can drive. A core-worker EGL
/// session keeps its frames in its own context and offers readback instead.
final Object _openGlFrameSkip = coreWorkerExposesFrames
    ? false
    : 'An OpenGL core-worker session exposes no frames to view';

/// Only Metal and Vulkan open a context from `tests/graphics`.
final Object _testGraphicsSkip = testGraphicsOpens
    ? false
    : 'OpenGL opens no test graphics context';

void main() {
  test('a device renders with the backend under test, $backendUnderTest', () {
    expect(openGpuDevice().getCapabilities().backend, backendUnderTest);
  });

  test(
    'an offscreen session renders a frame that reads back as pixels',
    () async {
      final map = await openRenderMap(_redStyle);
      final worker = await WorkerSession.attach(map);

      final result = await worker.renderFrame();
      expect(result.frameGeneration, greaterThan(0));
      _expectRed(
        await within(worker.session.readTexture(), 'texture readback'),
      );
    },
  );

  test(
    'a disposed attached session lets its map release before its device goes',
    () async {
      final map = await openRenderMap(emptyStyle);
      final device = openGpuDevice();
      final session = attachOffscreenSession(
        map,
        const RenderSessionAttachOptions(driver: RenderDriverKind.coreWorker),
        device: device,
      );
      addTearDown(() => releaseSession(session));
      await within(session.ready, 'attachment');

      session.dispose();
      expect(session.isClosed, isTrue);
      // A second dispose is a no-op.
      session.dispose();
      // A dispose reports no retirement, so the map's release waits for it.
      await within(map.release(), 'map release');
      device.release();
    },
  );

  test(
    'a session disposed while it attaches fails its ready only where awaited',
    () async {
      final map = await openRenderMap(emptyStyle);
      // Nothing services a caller-driven session, so it is still attaching.
      final session = TestGraphics.open().attachBorrowedTexture(
        map,
        const LogicalSize(width: renderSize, height: renderSize),
        const RenderSessionAttachOptions(
          driver: RenderDriverKind.callerGraphicsThread,
        ),
      );
      addTearDown(() => releaseSession(session));
      session.dispose();
      // The disposal fails the attach before the map's release finishes, and
      // the zone would report a failure that nothing handled.
      await within(map.release(), 'map release');
      await expectLater(
        session.ready,
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.targetLost,
          ),
        ),
      );
    },
    skip: _testGraphicsSkip,
  );

  test(
    "a custom source answers through the map it borrows, which it can't release",
    () async {
      final map = await openRenderMap(emptyStyle);
      final calls = <(int, String, Object?)>[];
      await within(
        map.addCustomGeometrySource(
          'custom',
          CustomGeometrySourceOptions(
            fetchTile: (borrowed, sourceId, tile) {
              borrowed.discarding.setCustomGeometrySourceTileData(
                sourceId,
                tile,
                {'type': 'FeatureCollection', 'features': <Object?>[]},
              );
              Object? refusal;
              try {
                borrowed.dispose();
              } on MaplibreException catch (error) {
                refusal = error;
              }
              calls.add((borrowed.id, sourceId, refusal));
            },
          ),
        ),
        'source add',
      );
      await within(
        map.addStyleLayerJson({
          'id': 'dots',
          'type': 'circle',
          'source': 'custom',
        }),
        'layer add',
      );
      final worker = await WorkerSession.attach(map);
      // The map asks for its tiles as it renders, and the source fetches them
      // on a worker thread in its own time.
      final deadline = DateTime.now().add(waitDeadline);
      while (calls.isEmpty) {
        if (DateTime.now().isAfter(deadline)) {
          fail('timed out waiting for the source to fetch a tile');
        }
        await worker.renderFrame();
      }
      final (id, sourceId, refusal) = calls.first;
      expect(id, map.id);
      expect(sourceId, 'custom');
      expect(
        refusal,
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidState,
        ),
      );
      expect(map.isClosed, isFalse);
    },
  );

  test('a drain before any demand returns no results', () async {
    final map = await openRenderMap(_redStyle);
    final worker = await WorkerSession.attach(map);
    // Native writes a null batch, which reads as no results.
    expect(worker.session.drainFrameResults(), isEmpty);
  });

  test(
    'a frame view expires with its scope and holds off release inside it',
    () async {
      final map = await openRenderMap(emptyStyle);
      final worker = await WorkerSession.attachBorrowedTexture(map);
      await worker.renderFrame();
      final session = worker.session;
      final frame = session.acquireFrame()!;
      const done = GpuSync(kind: GpuSyncKind.cpuComplete);

      final escaped = _openFrameView(frame, (view) {
        expect(view.width(), renderSize);
        view.texture();
        // The view borrows the frame, so neither the frame nor its session
        // can go away while it is open.
        expect(
          () => frame.release(done),
          throwsA(
            isA<MaplibreException>().having(
              (error) => error.status,
              'status',
              Status.busy,
            ),
          ),
        );
        expect(
          session.abandon,
          throwsA(
            isA<MaplibreException>().having(
              (error) => error.status,
              'status',
              Status.busy,
            ),
          ),
        );
      });
      // A view that escapes its scope refuses the texture it lends, and still
      // reads its plain values.
      expect(escaped.width(), renderSize);
      expect(escaped.texture, throwsA(isA<MaplibreException>()));

      // The refused release left the frame open; a second release after the
      // accepted one is a no-op.
      expect(frame.isClosed, isFalse);
      frame.release(done);
      expect(frame.isClosed, isTrue);
      frame.release(done);
      expect(
        frame.toString(),
        'AcquiredFrameHandle(id=0x${frame.id.toRadixString(16)}, closed=true)',
      );
      expect(frame.getResult, throwsA(isA<MaplibreException>()));
    },
    skip: _openGlFrameSkip,
  );
}

/// Reads a backend's texture view: its width, a plain value, and the texture
/// that it lends, which the view's scope guards.
typedef _FrameView = ({int Function() width, int Function() texture});

/// Runs [inside] within the scope of the backend's texture view of [frame],
/// and returns a reader of that view for use after the scope ends.
_FrameView _openFrameView(
  AcquiredFrameHandle frame,
  void Function(_FrameView view) inside,
) {
  if (backendUnderTest == RenderBackend.metal) {
    late _FrameView escaped;
    frame.withMetalTexture((view) {
      escaped = (width: () => view.width, texture: () => view.texture.address);
      inside(escaped);
    });
    return escaped;
  }
  if (backendUnderTest == RenderBackend.vulkan) {
    late _FrameView escaped;
    frame.withVulkanTexture((view) {
      escaped = (width: () => view.width, texture: () => view.image);
      inside(escaped);
    });
    return escaped;
  }
  fail('no core-worker frame view on $backendUnderTest');
}
