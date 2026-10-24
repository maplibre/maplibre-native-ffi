// Owned-texture sessions on a core worker: rendering and readback, teardown
// before the graphics go, and the scope of a borrowed frame view.
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';
import 'support/graphics.dart';
import 'support/render.dart';

const _redStyleJson =
    '{"version":8,"sources":{},"layers":[{"id":"background",'
    '"type":"background","paint":{"background-color":"#ff0000"}}]}';

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
    final map = await openRenderMap(_redStyleJson);
    final worker = await WorkerSession.attach(map);

    final result = await worker.renderFrame();
    expect(result.frameGeneration, greaterThan(0));
    _expectRed(await within(worker.session.readTexture(), 'texture readback'));
  }, skip: _wglSkip);

  test('an attach with default options completes on its own', () async {
    final map = await openRenderMap(emptyStyleJson);
    // Nothing services driver work, so only a core-worker default completes.
    final attachment = TestGraphics.open().attachOwnedTexture(
      map,
      _extent,
      RenderSessionAttachOptions(),
    );
    addTearDown(() => releaseSession(attachment.session));
    await within(attachment.completed, 'attachment');
  }, skip: _wglSkip);

  test(
    'a disposed attached session lets its map release before its graphics go',
    () async {
      final map = await openRenderMap(emptyStyleJson);
      final graphics = TestGraphics.open();
      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        RenderSessionAttachOptions(),
      );
      addTearDown(() => releaseSession(attachment.session));
      await within(attachment.completed, 'attachment');

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
      final map = await openRenderMap(emptyStyleJson);
      final graphics = TestGraphics.open();
      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        RenderSessionAttachOptions(),
      );
      addTearDown(() => releaseSession(attachment.session));
      await within(attachment.completed, 'attachment');

      await within(attachment.session.close(), 'session close');
      graphics.close();
    },
    skip: _wglSkip,
  );

  test('a drain before any demand returns no batch', () async {
    final map = await openRenderMap(_redStyleJson);
    final worker = await WorkerSession.attach(map);
    // Native reports the drain as not ready, which reads as null.
    expect(worker.session.drainFrameResults(), isNull);
  }, skip: _wglSkip);

  test(
    'a frame view expires with its scope and holds off release inside it',
    () async {
      final map = await openRenderMap(emptyStyleJson);
      final worker = await WorkerSession.attach(map);
      await worker.renderFrame();
      final session = worker.session;
      final frame = session.acquireFrame()!;

      final readEscapedWidth = _openFrameView(frame, (width) {
        expect(width, renderSize);
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
      // A view that escapes its scope refuses every read.
      expect(readEscapedWidth, throwsA(isA<MaplibreException>()));

      frame.release();
      expect(frame.getResult, throwsA(isA<MaplibreException>()));
    },
    skip: _openglFrameSkip,
  );
}

/// Runs [inside] within the scope of the backend's texture view of [frame],
/// and returns a reader of that view's width for use after the scope ends.
int Function() _openFrameView(
  AcquiredFrameHandle frame,
  void Function(int width) inside,
) {
  final backends = supportedRenderBackendMask();
  if (backends.contains(RenderBackendFlag.metal)) {
    late ScopedMetalTextureFrame escaped;
    frame.getMetalTexture().withView((view) {
      escaped = view;
      inside(view.width);
    });
    return () => escaped.width;
  }
  if (backends.contains(RenderBackendFlag.vulkan)) {
    late ScopedVulkanTextureFrame escaped;
    frame.getVulkanTexture().withView((view) {
      escaped = view;
      inside(view.width);
    });
    return () => escaped.width;
  }
  fail('no core-worker frame view on $backends');
}
