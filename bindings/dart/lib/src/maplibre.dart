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
  _LogCallbackState(LogCallback callback, {required bool consume}) {
    listener =
        NativeCallable<raw.mln_adapter_log_record_listenerFunction>.listener((
          // The adapter passes each registration's own listener_data. Dart
          // mints one native callback per registration, so identity is already
          // in the closure and this argument is unused here.
          Pointer<Void> listenerData,
          Pointer<Void> record,
        ) {
          if (record == nullptr) {
            close();
            return;
          }
          final ran = runUpcall(() {
            try {
              final copied = record.cast<raw.mln_adapter_log_record>().ref;
              if (copied.retire_callback) {
                close();
                return;
              }
              try {
                callback(_copyLogRecord(copied));
              } catch (_) {
                // Log callbacks are notification boundaries; user exceptions are
                // contained so they never surface from native callback delivery.
              }
            } finally {
              Maplibre._c.adapterLogRecordDestroy(record);
            }
          });
          if (!ran) {
            Maplibre._c.adapterLogRecordDestroy(record);
          }
        });
    pointer = calloc<raw.mln_adapter_log_callback_state>();
    pointer.ref.listener = listener.nativeFunction;
    pointer.ref.consume = consume ? 1 : 0;
  }

  late final Pointer<raw.mln_adapter_log_callback_state> pointer;
  late final NativeCallable<raw.mln_adapter_log_record_listenerFunction>
  listener;

  @override
  void closeResources() {
    calloc.free(pointer);
    listener.close();
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
/// `nullptr` when no callback is registered.
///
/// Lifecycle tests use this to drive native log dispatch directly, the way
/// MapLibre's logging threads do.
Pointer<raw.mln_adapter_log_callback_state> logCallbackStateForTesting() =>
    Maplibre._logCallbackState?.pointer ?? nullptr;

/// Process-global entry points for the Dart binding.
final class Maplibre {
  Maplibre._();

  static final MaplibreNativeCApi _c = MaplibreNativeCApi.open();
  // Retains the Dart listener while native code owns its callback pointer.
  static _LogCallbackState? _logCallbackState;

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
    // Before the registration, not after: a native callback that is already
    // installed cannot be taken back by throwing, and the failure path frees
    // the state native still points at.
    ensureAbiVersion();
    final state = _LogCallbackState(callback, consume: consume);
    try {
      _checkStatus(raw.mln_adapter_log_set_callback(state.pointer));
      _logCallbackState = state;
    } catch (_) {
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
    checkNativeStatus(status, _c.threadLastErrorMessage);
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
