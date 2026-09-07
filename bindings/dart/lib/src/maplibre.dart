import 'dart:ffi';

import 'package:ffi/ffi.dart';

import 'geo/geo.dart';
import 'internal/callback/callback_state.dart';
import 'internal/c/maplibre_native_c.dart';
import 'internal/c/maplibre_native_c.g.dart' as raw;
import 'internal/memory/memory.dart';
import 'internal/status/status.dart';
import 'internal/struct/struct.dart' as native_struct;
import 'log/log.dart';
import 'render/targets.dart';

final class _LogCallbackState extends RetainedCallbackState {
  _LogCallbackState(LogCallback callback, {required bool consume})
    : _callback = callback {
    listener = NativeCallable<raw.mln_wake_callbackFunction>.listener((
      Pointer<Void> _,
    ) {
      runUpcall(_drain);
    });
    final outQueue = calloc<Uint64>();
    final wake = calloc<raw.mln_wake>();
    try {
      wake.ref.size = sizeOf<raw.mln_wake>();
      wake.ref.callback = listener.nativeFunction;
      wake.ref.user_data = nullptr;
      wake.ref.release_user_data = nullptr;
      Maplibre._checkStatus(raw.mln_adapter_log_queue_create(wake, outQueue));
      queue = outQueue.value;
    } catch (_) {
      listener.close();
      rethrow;
    } finally {
      calloc.free(wake);
      calloc.free(outQueue);
    }
    pointer = calloc<raw.mln_adapter_log_callback_state>();
    pointer.ref.queue = queue;
    pointer.ref.consume = consume ? 1 : 0;
    releaseListener =
        NativeCallable<raw.mln_log_callback_releaseFunction>.listener((
          Pointer<Void> context,
        ) {
          Maplibre._releaseLogCallbackState(context);
        });
    pointer.ref.release_user_data = releaseListener.nativeFunction;
    pointer.ref.release_context = pointer.cast<Void>();
  }

  final LogCallback _callback;
  late final Pointer<raw.mln_adapter_log_callback_state> pointer;
  late final NativeCallable<raw.mln_wake_callbackFunction> listener;
  late final NativeCallable<raw.mln_log_callback_releaseFunction>
  releaseListener;
  late final int queue;

  void _drain() {
    withNativeArena((arena) {
      final outRecord = arena<Pointer<raw.mln_adapter_log_record>>();
      while (true) {
        outRecord.value = nullptr;
        Maplibre._checkStatus(
          raw.mln_adapter_log_queue_acquire(queue, outRecord),
        );
        final record = outRecord.value;
        if (record == nullptr) {
          return;
        }
        try {
          _callback(_copyLogRecord(record.ref));
        } catch (_) {
          // An exception must not escape into notification delivery.
        } finally {
          raw.mln_adapter_log_record_destroy(record.cast<Void>());
        }
      }
    });
  }

  @override
  void closeResources() {
    raw.mln_adapter_log_queue_close(queue);
    calloc.free(pointer);
    listener.close();
    releaseListener.close();
  }
}

LogRecord _copyLogRecord(raw.mln_adapter_log_record record) {
  return LogRecord(
    severity: LogSeverity.fromRawValue(record.severity),
    event: LogEvent.fromRawValue(record.event),
    code: record.code,
    message: record.message == nullptr
        ? ''
        : record.message.cast<Utf8>().toDartString(),
  );
}

/// Returns the log callback state native code currently dispatches through, or
/// `nullptr` when no callback is registered. Lifecycle tests use this to drive
/// native log dispatch directly.
Pointer<raw.mln_adapter_log_callback_state> logCallbackStateForTesting() =>
    Maplibre._logCallbackState?.pointer ?? nullptr;

/// Process-global entry points for the Dart binding.
final class Maplibre {
  Maplibre._();

  static final _logCallbackRoots = <int, _LogCallbackState>{};
  static _LogCallbackState? _logCallbackState;

  static void _releaseLogCallbackState(Pointer<Void> context) {
    final state = _logCallbackRoots.remove(context.address);
    if (state == null) {
      return;
    }
    if (identical(_logCallbackState, state)) {
      _logCallbackState = null;
    }
    state.close();
  }

  /// Returns the native C ABI contract version.
  ///
  /// The one entry point that does not gate on [ensureAbiVersion], because it
  /// reports the version that gate reads.
  static int cVersion() => raw.mln_c_version();

  /// Returns the render backends compiled into the linked native library.
  static RenderBackendMask supportedRenderBackends() {
    ensureAbiVersion();
    return RenderBackendMask(raw.mln_supported_render_backend_mask());
  }

  /// Returns OpenGL context providers compiled into the linked native library.
  static OpenGLContextProviderMask supportedOpenGLContextProviders() {
    ensureAbiVersion();
    return OpenGLContextProviderMask(
      raw.mln_opengl_supported_context_provider_mask(),
    );
  }

  /// Reads MapLibre Native's process-global network status.
  static NetworkStatus networkStatus() {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final outStatus = arena<Uint32>();
      _checkStatus(raw.mln_network_status_get(outStatus));
      return NetworkStatus.fromRawValue(outStatus.value);
    });
  }

  /// Sets MapLibre Native's process-global network status.
  static void setNetworkStatus(NetworkStatus status) {
    ensureAbiVersion();
    _checkStatus(raw.mln_network_status_set(status.rawValue));
  }

  /// Sets the process-global native log callback.
  static void setLogCallback(LogCallback callback, {bool consume = false}) {
    // Before the registration: the failure path frees state that an installed
    // native callback would still point at.
    ensureAbiVersion();
    final state = _LogCallbackState(callback, consume: consume);
    _logCallbackRoots[state.pointer.address] = state;
    try {
      _checkStatus(raw.mln_adapter_log_set_callback(state.pointer));
      _logCallbackState = state;
    } catch (_) {
      _logCallbackRoots.remove(state.pointer.address);
      state.close();
      rethrow;
    }
  }

  /// Clears the process-global native log callback.
  static void clearLogCallback() {
    ensureAbiVersion();
    _checkStatus(raw.mln_adapter_log_set_callback(nullptr));
    _logCallbackState = null;
  }

  /// Sets which log severities MapLibre Native may dispatch asynchronously.
  static void setAsyncLogSeverityMask(LogSeverityMask mask) {
    ensureAbiVersion();
    _checkStatus(raw.mln_log_set_async_severity_mask(mask.bits));
  }

  /// Converts a geographic coordinate to spherical Mercator projected meters.
  static ProjectedMeters projectedMetersForLatLng(LatLng coordinate) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final outMeters = arena<raw.mln_projected_meters>();
      _checkStatus(
        raw.mln_projected_meters_for_lat_lng(
          native_struct.latLngToNative(coordinate),
          outMeters,
        ),
      );
      return native_struct.projectedMetersFromNative(outMeters.ref);
    });
  }

  /// Converts spherical Mercator projected meters to a geographic coordinate.
  static LatLng latLngForProjectedMeters(ProjectedMeters meters) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _checkStatus(
        raw.mln_lat_lng_for_projected_meters(
          native_struct.projectedMetersToNative(meters),
          outCoordinate,
        ),
      );
      return native_struct.latLngFromNative(outCoordinate.ref);
    });
  }

  /// Restores MapLibre Native's default async log severity mask.
  static void restoreDefaultAsyncLogSeverityMask() {
    setAsyncLogSeverityMask(LogSeverityMask.defaultMask);
  }

  static void _checkStatus(int status) {
    ensureAbiVersion();
    checkNativeStatus(status, threadLastErrorMessage);
  }
}

/// Render backend support flags reported by this native library build.
final class RenderBackendMask {
  /// Creates a backend mask from raw C flag bits.
  const RenderBackendMask(this.bits);

  /// Metal backend support bit.
  static const metal = RenderBackendMask(1 << 0);

  /// Vulkan backend support bit.
  static const vulkan = RenderBackendMask(1 << 1);

  /// OpenGL backend support bit.
  static const opengl = RenderBackendMask(1 << 2);

  /// WebGPU backend support bit.
  static const webgpu = RenderBackendMask(1 << 3);

  /// Raw backend mask bits.
  final int bits;

  /// Returns true when all [backend] bits are present in this mask.
  bool contains(RenderBackendMask backend) =>
      (bits & backend.bits) == backend.bits;

  @override
  String toString() => 'RenderBackendMask[bits=0x${bits.toRadixString(16)}]';
}

/// Process-global network status.
final class NetworkStatus {
  const NetworkStatus._(this.rawValue, this.name);

  /// Network requests are allowed.
  static const online = NetworkStatus._(1, 'online');

  /// Online source network requests are paused.
  static const offline = NetworkStatus._(2, 'offline');

  /// Creates the public network-status value for a raw native value.
  factory NetworkStatus.fromRawValue(int rawValue) => switch (rawValue) {
    1 => online,
    2 => offline,
    _ => NetworkStatus._(rawValue, 'unknown'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable status name.
  final String name;

  @override
  String toString() => name == 'unknown' ? 'unknown($rawValue)' : name;
}
