import 'generated_workflows.dart';
import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show checkNativeCall, nativeDiagnostic;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:test/test.dart';

void _discardCompletion(
  Pointer<Void> _,
  Pointer<raw.mln_completion_result> _,
) {}

void _discardCompletionState(Pointer<Void> _) {}

/// Calls the C snapshot accessor with a raw map ID, so a test can replay a
/// released ID or use one from another isolate. The safe API expresses
/// neither.
void _mapSnapshotById(int map) {
  final arena = Arena();
  try {
    final snapshot = arena<raw.mln_map_snapshot>();
    snapshot.ref.size = sizeOf<raw.mln_map_snapshot>();
    checkNativeCall(raw.mln_map_snapshot_get(map, snapshot, nativeDiagnostic));
  } finally {
    arena.releaseAll();
  }
}

void _runtimeBarrierById(int handle) {
  final arena = Arena();
  try {
    final completion = raw.mln_completion.$allocate(
      arena,
      size: sizeOf<raw.mln_completion>(),
      callback: Pointer.fromFunction<raw.mln_completion_callbackFunction>(
        _discardCompletion,
      ),
      user_data: nullptr,
      release_user_data:
          Pointer.fromFunction<raw.mln_completion_releaseFunction>(
            _discardCompletionState,
          ),
    );
    checkNativeCall(
      raw.mln_runtime_barrier(handle, completion, nativeDiagnostic),
    );
  } finally {
    arena.releaseAll();
  }
}

void main() {
  test(
    'a released map id replayed after a new map is reported stale',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final first = await runtime.createMap();
      final released = first.identity.toSigned(64).toInt();
      await first.close();

      // The released slot is the one the next map takes, so the replayed id
      // names a retired generation of a slot that is live again.
      final second = await runtime.createMap();
      addTearDown(() async {
        await second.close();
        await runtime.close();
      });

      expect(
        () => _mapSnapshotById(released),
        throwsA(
          isA<InvalidArgumentException>().having(
            (error) => error.diagnostic,
            'diagnostic',
            contains('stale'),
          ),
        ),
      );

      // The live map is unaffected by the replay.
      _mapSnapshotById(second.identity.toSigned(64).toInt());
    },
  );

  test(
    'a map id passed to a runtime operation is rejected on its kind',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap();
      addTearDown(() async {
        await map.close();
        await runtime.close();
      });

      // The generated bindings spell both as `int`, so this call needs the raw
      // id; the C API rejects it on its kind tag.
      expect(
        () => _runtimeBarrierById(map.identity.toSigned(64).toInt()),
        throwsA(
          isA<InvalidArgumentException>()
              .having((e) => e.diagnostic, 'diagnostic', contains('map'))
              .having((e) => e.diagnostic, 'diagnostic', contains('runtime')),
        ),
      );
    },
  );
}
