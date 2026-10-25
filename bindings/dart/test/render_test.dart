// Owned-texture sessions on a core worker: rendering and readback, teardown
// before the graphics go, and the scope of a borrowed frame view.
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

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

final _extent = LogicalExtent(
  width: renderSize,
  height: renderSize,
  scaleFactor: 1,
);

void _expectRed(TextureReadbackResult image) {
  expect(image.info.width, renderSize);
  expect(image.info.height, renderSize);
  final center = (renderSize ~/ 2) * image.info.stride + (renderSize ~/ 2) * 4;
  expect(image.data.sublist(center, center + 4), [255, 0, 0, 255]);
}

/// A WGL texture shares the host's context, which only a caller-driven session
/// can drive, and a Dart isolate services core-worker sessions only.
final Object _wglSkip = buildHasCoreWorkerTexture
    ? false
    : 'A WGL texture has no core-worker session for a Dart isolate to render';

/// An OpenGL session exposes frames only from a context that it shares with
/// the host, which only a caller-driven session can drive. A core-worker EGL
/// session keeps its frames in its own context and offers readback instead.
final Object _openglFrameSkip = buildExposesCoreWorkerFrames
    ? false
    : 'An OpenGL core-worker session exposes no frames to view';

void main() {
  test('an owned texture renders a frame that reads back as pixels', () async {
    final map = await openRenderMap(_redStyle);
    final worker = await WorkerSession.attach(map);

    final result = await worker.renderFrame();
    expect(result.frameGeneration, greaterThan(0));
    _expectRed(await within(worker.session.readTexture(), 'texture readback'));
  }, skip: _wglSkip);

  test('an attach with default options completes on its own', () async {
    final map = await openRenderMap(emptyStyle);
    // Nothing services driver work, so only a core-worker default completes.
    final attachment = TestGraphics.open().attachOwnedTexture(
      map,
      _extent,
      RenderSessionAttachOptions(),
    );
    addTearDown(() => releaseSession(attachment.session));
    await within(attachment.ready, 'attachment');
  }, skip: _wglSkip);

  test(
    'a disposed attached session lets its map release before its graphics go',
    () async {
      final map = await openRenderMap(emptyStyle);
      final graphics = TestGraphics.open();
      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        RenderSessionAttachOptions(),
      );
      addTearDown(() => releaseSession(attachment.session));
      await within(attachment.ready, 'attachment');

      attachment.session.dispose();
      expect(attachment.session.isClosed, isTrue);
      // A second dispose is a no-op.
      attachment.session.dispose();
      // A dispose reports no retirement, so the map's release waits for it.
      await within(map.close(), 'map close');
      graphics.close();
    },
    skip: _wglSkip,
  );

  test(
    'closing an attached session completes once its graphics may go',
    () async {
      final map = await openRenderMap(emptyStyle);
      final graphics = TestGraphics.open();
      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        RenderSessionAttachOptions(),
      );
      addTearDown(() => releaseSession(attachment.session));
      await within(attachment.ready, 'attachment');

      await within(attachment.session.close(), 'session close');
      graphics.close();
    },
    skip: _wglSkip,
  );

  test('a drain before any demand returns no results', () async {
    final map = await openRenderMap(_redStyle);
    final worker = await WorkerSession.attach(map);
    // Native reports the drain as not ready, which reads as no results.
    expect(worker.session.drainFrameResults(), isEmpty);
  }, skip: _wglSkip);

  test(
    'a frame view expires with its scope and holds off release inside it',
    () async {
      final map = await openRenderMap(emptyStyle);
      final worker = await WorkerSession.attach(map);
      await worker.renderFrame();
      final session = worker.session;
      final frame = session.acquireFrame()!;

      final escaped = _openFrameView(frame, (view) {
        expect(view.width(), renderSize);
        view.texture();
        // The view borrows the frame, so neither the frame nor its session
        // can go away while it is open.
        expect(
          () => frame.release(consumerCompletion: gpuSyncDefault()),
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
      frame.release();
      expect(frame.isClosed, isTrue);
      frame.release();
      expect(
        frame.toString(),
        'AcquiredFrameHandle(id=0x${frame.id.toRadixString(16)}, closed=true)',
      );
      expect(frame.getResult, throwsA(isA<MaplibreException>()));
    },
    skip: _openglFrameSkip,
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
  final backends = supportedRenderBackendMask();
  if (backends.contains(RenderBackendFlag.metal)) {
    late _FrameView escaped;
    frame.withMetalTexture((view) {
      escaped = (width: () => view.width, texture: () => view.texture.address);
      inside(escaped);
    });
    return escaped;
  }
  if (backends.contains(RenderBackendFlag.vulkan)) {
    late _FrameView escaped;
    frame.withVulkanTexture((view) {
      escaped = (width: () => view.width, texture: () => view.image);
      inside(escaped);
    });
    return escaped;
  }
  fail('no core-worker frame view on $backends');
}
