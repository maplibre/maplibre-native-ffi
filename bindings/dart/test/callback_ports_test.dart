// Port-delivered callback registrations: each one roots its callback for as
// long as native holds it, and retires when native releases it.
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show singleCallbackPortProbeForTesting;
import 'package:test/test.dart';

import 'support/fixture.dart';

/// Every event type but style-loaded, the type a host most plausibly clears
/// while still expecting a source's callback root to be released.
final _maskWithoutStyleLoaded = RuntimeEventMask.fromRawValue(
  RuntimeEventMask.all.rawValue & ~RuntimeEventMask.mapStyleLoaded.rawValue,
);

CustomGeometrySourceOptions _geometrySource({int? tileSize}) =>
    CustomGeometrySourceOptions(fetchTile: (_) {}, tileSize: tileSize);

void main() {
  test(
    'a callback root is released by a removal, a map close, or a refusal',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final removed = singleCallbackPortProbeForTesting(map)!;
      expect(removed.closed, isFalse);
      await expectCommitted(map.removeStyleSource('source'));
      await within(removed.released, 'the release after a removal');
      expect(singleCallbackPortProbeForTesting(map), isNull);

      // A registration that never reaches native is released at once.
      expect(
        () => map.addCustomGeometrySource(
          'refused',
          _geometrySource(tileSize: -1),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(singleCallbackPortProbeForTesting(map), isNull);

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final closed = singleCallbackPortProbeForTesting(map)!;
      await within(map.close(), 'map close');
      await within(closed.released, 'the release after a map close');
    },
  );

  // The release callback the C API invokes is what tells this binding a style
  // replacement detached a source, so a host that reads no style-loaded events
  // still gets its callback root retired.
  test(
    'a style replacement releases a source root with style loads unselected',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap(
        MapOptions(
          initialExtent: const LogicalExtent(
            width: 64,
            height: 64,
            scaleFactor: 1,
          ),
          eventMask: _maskWithoutStyleLoaded,
        ),
      );
      await expectCommitted(map.setStyleJson(jsonBytes(emptyStyleJson)));
      await expectCommitted(
        map.addCustomMvtVectorSource(
          'source',
          CustomMvtVectorSourceOptions(fetchTile: (_) {}),
        ),
      );
      final probe = singleCallbackPortProbeForTesting(map)!;
      // The mask reads back as the host set it, because the binding selects
      // no events of its own.
      expect(map.getSnapshot().eventMask, _maskWithoutStyleLoaded);

      await expectCommitted(map.setStyleJson(jsonBytes(emptyStyleJson)));
      await within(probe.released, 'the release after a style replacement');
      expect(singleCallbackPortProbeForTesting(map), isNull);
    },
  );
}
