import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/lifecycle/native_handles.dart';
import 'package:maplibre_native_ffi/src/internal/status/status.dart';
import 'package:test/test.dart';

final _c = MaplibreNativeCApi.open();

/// Calls the C size accessor with a raw map id, so a test can replay a released
/// id or use one from another thread. The safe API expresses neither.
void _mapSizeById(NativeMap map) {
  final arena = Arena();
  try {
    final width = arena<Uint32>();
    final height = arena<Uint32>();
    final scaleFactor = arena<Double>();
    checkNativeStatus(
      _c.raw.mln_map_get_size(map.raw, width, height, scaleFactor).value,
      _c.threadLastErrorMessage,
    );
  } finally {
    arena.releaseAll();
  }
}

void main() {
  test('a released map id replayed after a new map is reported stale', () {
    final runtime = RuntimeHandle.create();
    final first = runtime.createMap();
    final released = NativeMap(first.attachRef().mapIdForTesting);
    first.close();

    // The released slot is the one the next map takes, so this is the case a
    // pointer handle could not tell apart from a live map.
    final second = runtime.createMap();
    addTearDown(() {
      second.close();
      runtime.close();
    });

    expect(
      () => _mapSizeById(released),
      throwsA(
        isA<InvalidArgumentException>().having(
          (error) => error.diagnostic,
          'diagnostic',
          contains('stale'),
        ),
      ),
    );

    // The live map is unaffected by the replay.
    _mapSizeById(NativeMap(second.attachRef().mapIdForTesting));
  });

  test('a map id passed to a runtime operation is rejected on its kind', () {
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap();
    addTearDown(() {
      map.close();
      runtime.close();
    });

    // The generated bindings spell both as `int`, so this call needs the raw
    // id; the C API rejects it on its kind tag.
    expect(
      () => checkNativeStatus(
        _c.raw.mln_runtime_pump(map.attachRef().mapIdForTesting, 0).value,
        _c.threadLastErrorMessage,
      ),
      throwsA(
        isA<InvalidArgumentException>()
            .having((e) => e.diagnostic, 'diagnostic', contains('map'))
            .having((e) => e.diagnostic, 'diagnostic', contains('runtime')),
      ),
    );
  });

  test(
    'a live map id used from another isolate reports wrong thread',
    () async {
      final runtime = RuntimeHandle.create();
      final map = runtime.createMap();
      final live = map.attachRef().mapIdForTesting;

      // The id crosses isolates because it is a plain integer; the owner-thread
      // rule is what rejects the call, not identity.
      final diagnostic = await Isolate.run(() {
        try {
          _mapSizeById(NativeMap(live));
          return 'succeeded';
        } on MaplibreException catch (error) {
          return '${error.runtimeType}: ${error.diagnostic}';
        }
      });

      expect(diagnostic, startsWith('WrongThreadException'));
      expect(diagnostic, isNot(contains('stale')));

      map.close();
      runtime.close();
    },
  );
}
