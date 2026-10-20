import 'dart:mirrors';

import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/delivery_queue.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart' as runtime;
import 'package:maplibre_native_ffi/src/values.dart';

// Tests inspect library-private state through VM mirrors, keeping probes out of
// the shipped runtime and leaving the production converters under test.
final _runtime = reflectClass(runtime.CommandCompletion).owner as LibraryMirror;
final _queue = reflectClass(DeliveryQueue).owner as LibraryMirror;

Symbol _symbol(String name, LibraryMirror library) =>
    MirrorSystem.getSymbol(name, library);

Object? _field(Object target, String name, [LibraryMirror? library]) =>
    reflect(target).getField(_symbol(name, library ?? _runtime)).reflectee
        as Object?;

/// The one callback registration that owner still roots, or null.
DeliveryRegistration? singleRegistrationProbeForTesting(Object owner) {
  final ports = _field(owner, '_callbackRoots')!;
  final queued = _field(ports, 'queued') as Set<DeliveryRegistration>;
  return queued.isEmpty ? null : queued.single;
}

DeliveryRegistration? logRegistrationProbeForTesting() {
  final registrations =
      _field(isolateDeliveryQueue, '_registrations', _queue)
          as Map<int, WeakReference<DeliveryRegistration>>;
  final pending = registrations.values
      .map((reference) => reference.target)
      .whereType<DeliveryRegistration>()
      .where(
        (registration) =>
            registration.handlers.containsKey(raw.MLN_DELIVERY_TYPE_LOG_RECORD),
      );
  return pending.isEmpty ? null : pending.last;
}

List<RuntimeEvent> decodeRuntimeEventBatchForTesting(
  raw.mln_event_batch_view batch,
) {
  final Object decoded =
      _runtime.invoke(_symbol('_readEventBatchView', _runtime), [
            batch,
          ]).reflectee
          as Object;
  return _field(decoded, 'events') as List<RuntimeEvent>;
}

List<LatLng> decodeLatLngListForTesting(raw.mln_completion_result result) {
  final Object reader =
      _runtime.getField(_symbol('_resultLatLng', _runtime)).reflectee as Object;
  return reflect(
        reader,
      ).invoke(_symbol('readList', _runtime), [result]).reflectee
      as List<LatLng>;
}

T adoptOwnedForTesting<T>(
  int handle,
  T Function() adopt,
  void Function(int) dispose,
) =>
    _runtime.invoke(_symbol('_adoptOwned', _runtime), [
          handle,
          adopt,
          dispose,
        ]).reflectee
        as T;
