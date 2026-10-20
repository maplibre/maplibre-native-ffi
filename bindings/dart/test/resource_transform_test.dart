// Resource transforms: queued requests that the binding hands over as
// transferable request handles.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('a transform answers its request after its callback returns', () async {
    const styleUrl = 'custom://transformed-style.json';
    final handed = Completer<ResourceTransformRequestHandle>();
    // A provider whose one route matches nothing leaves the style to native
    // loading, which waits for the transform's answer.
    final fixture = await openRuntime(
      provider: routedProvider([
        styleRoute('custom://unused.json'),
      ], (_, handle) => handle.close()),
    );
    await fixture.runtime.setResourceTransform(
      ResourceTransform(
        callback: (kind, url, request) {
          if (url == styleUrl) {
            handed.complete(request);
          } else {
            request.close();
          }
        },
      ),
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final request = await within(handed.future, 'the transform');
    request.complete('unsupported://rewritten-style.json');
    request.close();
    // Native loading cannot fetch the rewritten scheme.
    await fixture.awaitEventType(RuntimeEventType.mapLoadingFailed);
    await fixture.runtime.clearResourceTransform();
  });
}
