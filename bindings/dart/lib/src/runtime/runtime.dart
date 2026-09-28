import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../error/maplibre_exception.dart';
import '../log/log.dart';
import '../internal/callback/callback_state.dart';
import '../internal/callback/completion.dart';
import '../internal/callback/wake.dart';
import '../internal/callback/retained.dart';
import '../internal/c/maplibre_native_c.dart';
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/lifecycle/lifecycle.dart';
import '../internal/lifecycle/native_handles.dart';
import '../internal/memory/memory.dart';
import '../internal/status/status.dart';
import '../internal/value/uint64.dart';
import '../render/native_pointer.dart';

part 'runtime_resource_callbacks.dart';
part 'runtime_logging.dart';
part 'generated_operations.dart';
part 'runtime_offline.dart';

final MaplibreNativeCApi _c = MaplibreNativeCApi.open();

/// Native release roots for the adapter rule contexts and the log callback.
///
/// Native code retires each registration with a release message, which can
/// arrive after the owner that registered it is closed or collected. Nothing
/// here captures an owner, so one set serves the isolate.
final _callbackReleases = NativeCallbackReleases();

/// Native release roots for queued resource providers, one set per runtime.
///
/// A provider callback can capture its runtime, so these roots live only as
/// long as that runtime and never keep it reachable.
final _resourceProviderReleases = Expando<NativeCallbackReleases>();

final class CallbackPortLifecycleProbe {
  CallbackPortLifecycleProbe._(this._port);
  final _NativeCallbackPort _port;
  bool get retirementQueued => _port.retirementQueued;
  bool get closed => _port.closed;
}

/// Returns a map's single pending port, or null after native release.
CallbackPortLifecycleProbe? singleCallbackPortProbeForTesting(MapHandle map) {
  final pending = map._callbackPorts.pending;
  return pending.isEmpty ? null : CallbackPortLifecycleProbe._(pending.single);
}

/// Dart resource provider callback run asynchronously on its receiver isolate.
typedef ResourceProviderCallback =
    void Function(ResourceRequest request, ResourceRequestHandle handle);

/// Receiver-isolate resource provider definition.
final class QueuedResourceProvider {
  /// Creates a resource provider with native-owned routing rules.
  QueuedResourceProvider({
    required List<AdapterQueuedResourceProviderRoute> routes,
    required this.callback,
  }) : routes = List.unmodifiable(routes);

  /// Routes handled by this provider.
  final List<AdapterQueuedResourceProviderRoute> routes;

  /// Callback invoked on the receiver isolate for matching requests.
  final ResourceProviderCallback callback;
}

/// Queued Dart resource provider registration on a runtime.
///
/// The provider copies each matching request into a native queue that its
/// isolate drains, a protocol the generated operations do not express.
extension RuntimeQueuedResourceProvider on RuntimeHandle {
  /// Registers or replaces a queued Dart resource provider callback.
  ///
  /// Requests reach [QueuedResourceProvider.callback] on the isolate that
  /// registered the provider, one event-loop turn after MapLibre queues them.
  Future<CommandCompletion> setQueuedResourceProvider(
    QueuedResourceProvider provider,
  ) {
    final state = _ResourceProviderCallbackState(provider);
    final userData = state.pointer.cast<Void>();
    final releases = _resourceProviderReleases[this] ??=
        NativeCallbackReleases();
    releases.register(userData, state.retire, arena: state.arena);
    return _startCommand(
      (completion) => withNativeArena((arena) {
        final nativeProvider = arena<raw.mln_resource_provider>();
        nativeProvider.ref.size = sizeOf<raw.mln_resource_provider>();
        nativeProvider.ref.callback = _c
            .adapterQueuedResourceProviderCallback();
        nativeProvider.ref.user_data = userData;
        nativeProvider.ref.release_user_data =
            Native.addressOf<
              NativeFunction<raw.mln_runtime_callback_releaseFunction>
            >(raw.mln_adapter_dart_release);
        return raw.mln_runtime_set_resource_provider(
          _handle.raw,
          nativeProvider,
          completion,
        );
      }),
      onRejected: () => releases.reject(userData),
    );
  }
}

final class _NativeCallbackPorts {
  final pending = <_NativeCallbackPort>{};
  _NativeCallbackPort register(
    Map<int, void Function(List<dynamic>)> callbacks,
  ) {
    final port = _NativeCallbackPort(callbacks, (port) => pending.remove(port));
    pending.add(port);
    return port;
  }
}

final class _NativeCallbackPort {
  _NativeCallbackPort(
    Map<int, void Function(List<dynamic>)> callbacks,
    this._onReleased,
  ) {
    _callbacks = callbacks;
    _port = RawReceivePort();
    _port.handler = _callbackPortHandler(WeakReference(this), _port);
    context = raw.mln_adapter_dart_port_create(
      NativeApi.postCObject.cast(),
      _port.sendPort.nativePort,
    );
    if (context == nullptr) {
      _port.close();
      throw StateError('native callback port allocation failed');
    }
  }
  final void Function(_NativeCallbackPort) _onReleased;
  final _zone = Zone.current;
  late final Map<int, void Function(List<dynamic>)> _callbacks;
  void _deliver(dynamic message) {
    _zone.runGuarded(() {
      if (message == 0) {
        _finish();
        return;
      }
      final values = message as List<dynamic>;
      _callbacks[values[0]]?.call(values);
    });
  }

  var _closed = false;
  late final RawReceivePort _port;
  late final Pointer<Void> context;
  bool get closed => _closed;
  bool get retirementQueued => closed;
  void _finish() {
    if (closed) return;
    _closed = true;
    _port.close();
    _callbacks.clear();
    _onReleased(this);
  }

  void reject() {
    raw.mln_adapter_dart_port_release(context);
    _finish();
  }
}

void Function(dynamic) _callbackPortHandler(
  WeakReference<_NativeCallbackPort> reference,
  RawReceivePort port,
) => (dynamic message) {
  final state = reference.target;
  if (state == null) {
    if (message == 0) port.close();
    return;
  }
  state._deliver(message);
};

/// Decodes a synthetic native batch through the production generated converter.
List<RuntimeEvent> decodeRuntimeEventBatchForTesting(
  raw.mln_runtime_event_batch_view batch,
  RuntimeHandle runtime,
) => _readRuntimeEventBatchView(batch).events;

/// Starts a command and decodes its receipt, including failed dispositions.
Future<CommandCompletion> _startCommand(
  NativeCompletionStart start, {
  void Function()? onRejected,
}) => startNativeCompletion(
  copyKind:
      raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT,
  elementSize: 0,
  start: start,
  onRejected: onRejected,
  acceptErrorStatus: true,
  decode: (result) => CommandCompletion(
    disposition: CommandDisposition.fromRawValue(result.disposition),
    generation: uint64FromNative(result.generation),
    status: MaplibreStatus.fromNativeStatusCode(result.status),
    diagnostic: copyCompletionDiagnostic(result.diagnostic),
  ),
);

final class CommandCompletion {
  const CommandCompletion({
    required this.disposition,
    required this.generation,
    required this.status,
    required this.diagnostic,
  });

  final CommandDisposition disposition;
  final BigInt generation;
  final MaplibreStatus status;
  final String diagnostic;
}

T _adoptOwned<T>(int handle, T Function() adopt, void Function(int) dispose) {
  try {
    return adopt();
  } catch (error, stack) {
    try {
      dispose(handle);
    } catch (cleanup) {
      throw NativeAdoptionFailure(error, cleanup, () => dispose(handle));
    }
    Error.throwWithStackTrace(error, stack);
  }
}

final class NativeAdoptionFailure implements Exception {
  NativeAdoptionFailure(
    this.adoptionError,
    this.cleanupError,
    this.retryCleanup,
  );
  final Object adoptionError;
  final Object cleanupError;
  final void Function() retryCleanup;
}

void _check(int status) => checkNativeStatus(status, threadLastErrorMessage);
