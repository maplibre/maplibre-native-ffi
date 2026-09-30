// Owned-texture sessions: rendering and readback, the caller driver, and the
// scope of a borrowed frame view.
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';
import 'support/graphics.dart';
import 'support/render.dart';

const _redStyleJson =
    '{"version":8,"sources":{},"layers":[{"id":"background",'
    '"type":"background","paint":{"background-color":"#ff0000"}}]}';

void _expectRed(TextureReadbackResult image) {
  expect(image.info.width, renderSize);
  expect(image.info.height, renderSize);
  final center = (renderSize ~/ 2) * image.info.stride + (renderSize ~/ 2) * 4;
  expect(image.data.sublist(center, center + 4), [255, 0, 0, 255]);
}

void main() {
  test(
    'an owned texture renders a frame that reads back as pixels',
    () async {
      final map = await openRenderMap(_redStyleJson);
      final worker = await WorkerSession.attach(map);

      final result = await worker.renderFrame();
      expect(result.frameGeneration, greaterThan(BigInt.zero));
      _expectRed(
        await within(
          worker.session.textureReadPremultipliedRgba8(),
          'texture readback',
        ),
      );
    },
    skip: buildHasCoreWorkerTexture
        ? false
        : 'A WGL texture shares the host context, which only the caller '
              'driver drives; the caller-driven test renders and reads back '
              'pixels on WGL',
  );

  test(
    'a caller driver is serviced within one synchronous stretch of the isolate',
    () async {
      final map = await openRenderMap(_redStyleJson);
      final readback = await withCallerDrivenSession(map, (driven) {
        expect(
          driven.session.getCapabilities().driver,
          RenderDriverKind.callerGraphicsThread,
        );
        expect(driven.renderFrame().disposition, RenderResult.rendered);
        // The stretch services this readback before it detaches.
        return driven.session.textureReadPremultipliedRgba8();
      });
      _expectRed(await within(readback, 'texture readback'));
    },
  );

  test(
    'a frame view expires with its scope and holds off release inside it',
    () async {
      final map = await openRenderMap(emptyStyleJson);
      await withCallerDrivenSession(map, (driven) {
        driven.renderFrame();
        final session = driven.session;
        final frame = session.acquireFrame();

        final readEscapedWidth = _openFrameView(frame, (width) {
          expect(width, renderSize);
          // The view borrows the frame, so neither the frame nor its session
          // can go away while it is open.
          expect(
            () => frame.release(gpuSyncDefault()),
            throwsA(isA<BusyException>()),
          );
          expect(session.abandon, throwsA(isA<BusyException>()));
        });
        // A view that escapes its scope refuses every read.
        expect(readEscapedWidth, throwsA(isA<MaplibreException>()));

        frame.release(gpuSyncDefault());
        expect(frame.getResult, throwsA(isA<MaplibreException>()));
      });
    },
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
    late ScopedMetalOwnedTextureFrame escaped;
    frame.getMetalTexture().withView((view) {
      escaped = view;
      inside(view.width);
    });
    return () => escaped.width;
  }
  if (backends.contains(RenderBackendFlag.vulkan)) {
    late ScopedVulkanOwnedTextureFrame escaped;
    frame.getVulkanTexture().withView((view) {
      escaped = view;
      inside(view.width);
    });
    return () => escaped.width;
  }
  late ScopedOpenglOwnedTextureFrame escaped;
  frame.getOpenglTexture().withView((view) {
    escaped = view;
    inside(view.width);
  });
  return () => escaped.width;
}
