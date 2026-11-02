import 'dart:ffi';
import 'dart:mirrors';

import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/runtime/runtime.dart' as runtime;
import 'package:maplibre_native_ffi/src/values.dart';

// Tests inspect library-private state through VM mirrors, keeping probes out of
// the shipped runtime and leaving the production converters under test.
final _runtime = reflectClass(runtime.CommandCompletion).owner as LibraryMirror;

Symbol _symbol(String name, LibraryMirror library) =>
    MirrorSystem.getSymbol(name, library);

Object? _field(Object target, String name) =>
    reflect(target).getField(_symbol(name, _runtime)).reflectee as Object?;

List<RuntimeEvent> decodeRuntimeEventBatchForTesting(
  raw.mln_event_batch_view batch,
) {
  return _runtime.invoke(_symbol('_readEventBatchView', _runtime), [
        batch,
      ]).reflectee
      as List<RuntimeEvent>;
}

List<LatLng> decodeLatLngListForTesting(raw.mln_completion_result result) {
  final Object reader =
      _runtime.getField(_symbol('_resultLatLng', _runtime)).reflectee as Object;
  return reflect(
        reader,
      ).invoke(_symbol('readList', _runtime), [result]).reflectee
      as List<LatLng>;
}

/// Prepares wakes as an owner's call would, without a render backend.
final class WakeRootsProbe {
  WakeRootsProbe()
    : _roots =
          (_runtime.declarations[_symbol('_CallbackRoots', _runtime)]
                      as ClassMirror)
                  .newInstance(const Symbol(''), [])
                  .reflectee
              as Object;

  final Object _roots;

  WakeProbe register(void Function() callback) => WakeProbe._(
    _runtime.invoke(_symbol('_prepareWake', _runtime), [
          Wake(callback: callback),
          _roots,
        ]).reflectee
        as Object,
  );
}

final class WakeProbe {
  WakeProbe._(this._registration);

  final Object _registration;

  Pointer<raw.mln_wake> get _pointer =>
      _field(_registration, 'pointer')! as Pointer<raw.mln_wake>;

  void notify() => _pointer.ref.callback
      .asFunction<void Function(Pointer<Void>)>()(_pointer.ref.user_data);

  void release() {
    (_field(_registration, 'reject')! as void Function())();
    (_field(_registration, 'releaseMemory')! as void Function())();
  }
}
