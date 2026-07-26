import 'dart:ffi';
import 'dart:isolate';

import 'package:maplibre_native_ffi/src/error/maplibre_exception.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/callback_state.dart';
import 'package:maplibre_native_ffi/src/internal/lifecycle/lifecycle.dart';
import 'package:maplibre_native_ffi/src/internal/lifecycle/frame_construction.dart';
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:maplibre_native_ffi/src/internal/status/status.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart';
import 'package:ffi/ffi.dart';
import 'package:test/test.dart';

final class _FakeNativeHandle extends Opaque {}

final class _FakeCallbackState extends RetainedCallbackState {
  var closes = 0;

  @override
  void closeResources() {
    closes += 1;
  }
}

void main() {
  group('status conversion', () {
    test('ABI mismatch has a stable public error category', () {
      expect(
        () => validateCAbiVersion(expectedCAbiVersion + 1),
        throwsA(
          isA<AbiVersionMismatchException>()
              .having(
                (error) => error.status,
                'status',
                MaplibreStatus.abiVersionMismatch,
              )
              .having(
                (error) => error.diagnostic,
                'diagnostic',
                contains('expected $expectedCAbiVersion'),
              ),
        ),
      );
    });

    test('ok status returns without reading diagnostics', () {
      var diagnosticReads = 0;

      checkNativeStatus(nativeStatusOk, () {
        diagnosticReads += 1;
        return 'unused';
      });

      expect(diagnosticReads, 0);
    });

    test('native failures map every known status category', () {
      final cases = <(int, Type)>[
        (nativeStatusInvalidArgument, InvalidArgumentException),
        (nativeStatusInvalidState, InvalidStateException),
        (nativeStatusWrongThread, WrongThreadException),
        (nativeStatusUnsupported, UnsupportedFeatureException),
        (nativeStatusNativeError, NativeErrorException),
      ];

      for (final (status, type) in cases) {
        try {
          checkNativeStatus(status, () => 'diagnostic $status');
          fail('status $status unexpectedly succeeded');
        } on MaplibreException catch (error) {
          expect(error.runtimeType, type);
          expect(error.nativeStatusCode, status);
          expect(error.diagnostic, 'diagnostic $status');
        }
      }
    });

    test('unknown native status and copied diagnostic remain available', () {
      var nativeDiagnostic = 'first diagnostic';
      late MaplibreException error;

      try {
        checkNativeStatus(-999, () => nativeDiagnostic);
        fail('unknown status unexpectedly succeeded');
      } on MaplibreException catch (caught) {
        error = caught;
      }
      nativeDiagnostic = 'later diagnostic';

      expect(error, isA<UnknownMaplibreException>());
      expect(error.status.name, 'unknown');
      expect(error.nativeStatusCode, -999);
      expect(error.diagnostic, 'first diagnostic');
    });

    test('binding validation produces a fresh binding diagnostic', () {
      expect(
        () => throwInvalidArgument('fresh binding diagnostic'),
        throwsA(
          isA<InvalidArgumentException>()
              .having(
                (error) => error.nativeStatusCode,
                'nativeStatusCode',
                isNull,
              )
              .having(
                (error) => error.diagnostic,
                'diagnostic',
                'fresh binding diagnostic',
              ),
        ),
      );
    });
  });

  group('native string helpers', () {
    test('null-terminated strings reject embedded NUL', () {
      expect(
        () => withNativeArena((arena) => nativeUtf8CString('a\u0000b', arena)),
        throwsA(isA<InvalidArgumentException>()),
      );
    });

    test('null-terminated strings expose UTF-8 bytes and trailing NUL', () {
      withNativeArena((arena) {
        final value = nativeUtf8CString('café', arena);
        final bytes = value.pointer.cast<Uint8>();

        expect(value.byteLength, 5);
        expect(bytes[0], 'c'.codeUnitAt(0));
        expect(bytes[3], 0xc3);
        expect(bytes[4], 0xa9);
        expect(bytes[5], 0);
      });
    });

    test('string views preserve explicit byte length and embedded NUL', () {
      withNativeArena((arena) {
        final value = nativeStringView('a\u0000b', arena);

        expect(value.byteLength, 3);
        expect(value.value.size, 3);
        expect(value.value.data.cast<Uint8>()[1], 0);
      });
    });
  });

  group('callback state', () {
    test('retired callback state waits until queued turn to close', () async {
      final state = _FakeCallbackState();

      state.close();
      expect(state.closes, 0);
      expect(state.runUpcall(() {}), isFalse);

      await Future<void>.delayed(Duration.zero);
      expect(state.closes, 1);
    });

    test('retired callback state waits for active upcalls', () async {
      final state = _FakeCallbackState();
      expect(
        state.runUpcall(() {
          state.close();
          expect(state.closes, 0);
        }),
        isTrue,
      );

      expect(state.closes, 0);
      await Future<void>.delayed(Duration.zero);
      expect(state.closes, 1);
      expect(state.runUpcall(() {}), isFalse);
    });
  });

  group('native handle state', () {
    test('close succeeds once and later closes are no-ops', () {
      final state = NativeHandleState<_FakeNativeHandle>(
        Pointer.fromAddress(0x1234),
        'fake_handle',
      );
      var closes = 0;

      state.close((_) {
        closes += 1;
        return nativeStatusOk;
      }, () => 'unused');
      state.close((_) {
        closes += 1;
        return nativeStatusOk;
      }, () => 'unused');

      expect(closes, 1);
      expect(state.isClosed, isTrue);
    });

    test('owner isolate mismatch rejects use before native calls', () {
      final state = NativeHandleState<_FakeNativeHandle>(
        Pointer.fromAddress(0x1234),
        'fake_handle',
        ownerIsolateHash: Isolate.current.hashCode + 1,
        leakReporting: false,
      );

      expect(() => state.pointer, throwsA(isA<WrongThreadException>()));
      expect(
        () => state.close((_) => nativeStatusOk, () => 'unused'),
        throwsA(isA<WrongThreadException>()),
      );
    });

    test('failed close leaves handle live for retry', () {
      final state = NativeHandleState<_FakeNativeHandle>(
        Pointer.fromAddress(0x1234),
        'fake_handle',
      );

      expect(
        () => state.close((_) => nativeStatusInvalidState, () => 'busy'),
        throwsA(isA<InvalidStateException>()),
      );

      expect(state.isClosed, isFalse);
      state.close((_) => nativeStatusOk, () => 'unused');
      expect(state.isClosed, isTrue);
    });
  });

  group('owned frame construction cleanup', () {
    test('successful cleanup releases descriptor ownership', () {
      var released = 0;
      var retained = 0;

      cleanupFailedFrameConstruction(
        release: () => nativeStatusOk,
        releaseSucceeded: () => released += 1,
        releaseFailed: () => retained += 1,
      );

      expect(released, 1);
      expect(retained, 0);
    });

    test('failed cleanup preserves descriptor for owner-thread retry', () {
      var released = 0;
      var retained = 0;

      cleanupFailedFrameConstruction(
        release: () => nativeStatusInvalidState,
        releaseSucceeded: () => released += 1,
        releaseFailed: () => retained += 1,
      );

      expect(released, 0);
      expect(retained, 1);
    });
  });

  test('runtime event decoding copies and guards native payloads', () {
    final runtime = RuntimeHandle.create();
    final event = calloc<raw.mln_runtime_event>();
    final unknownPayload = calloc<Uint8>(3);
    final message = 'copied message'.toNativeUtf8();
    try {
      event.ref.size = sizeOf<raw.mln_runtime_event>();
      event.ref.type = 0xfeed;
      event.ref.source_type = 0xbeef;
      event.ref.source = nullptr;
      event.ref.code = 17;
      event.ref.payload_type = 0xf00d;
      event.ref.payload = unknownPayload.cast<Void>();
      event.ref.payload_size = 3;
      event.ref.message = message.cast<Char>();
      event.ref.message_size = 14;
      unknownPayload.asTypedList(3).setAll(0, [1, 2, 3]);

      final copied = copyRuntimeEventForTesting(event.ref, runtime);
      unknownPayload.asTypedList(3).fillRange(0, 3, 9);
      message.cast<Uint8>()[0] = 'X'.codeUnitAt(0);

      expect(copied.eventType.rawValue, 0xfeed);
      expect(copied.source, isA<UnknownRuntimeEventSource>());
      expect(copied.payload, isA<RuntimeEventPayloadUnknown>());
      expect((copied.payload as RuntimeEventPayloadUnknown).bytes, [1, 2, 3]);
      expect(copied.message, 'copied message');

      event.ref.payload_type = 2;
      event.ref.payload_size = 1;
      expect(
        copyRuntimeEventForTesting(event.ref, runtime).payload,
        isA<RuntimeEventPayloadUnknown>(),
      );

      event.ref.payload = nullptr;
      event.ref.payload_size = 7;
      final nullPayload = copyRuntimeEventForTesting(
        event.ref,
        runtime,
      ).payload;
      expect(nullPayload, isA<RuntimeEventPayloadUnknown>());
      expect((nullPayload as RuntimeEventPayloadUnknown).bytes, isEmpty);

      final renderMap = calloc<raw.mln_runtime_event_render_map>();
      try {
        renderMap.ref.size = sizeOf<raw.mln_runtime_event_render_map>();
        renderMap.ref.mode = 1;
        event.ref.payload = renderMap.cast<Void>();
        event.ref.payload_size = sizeOf<raw.mln_runtime_event_render_map>();
        final typed = copyRuntimeEventForTesting(event.ref, runtime).payload;
        expect(typed, isA<RuntimeEventRenderMap>());
        expect((typed as RuntimeEventRenderMap).mode, RenderMode.full);
      } finally {
        calloc.free(renderMap);
      }
    } finally {
      malloc.free(message);
      calloc.free(unknownPayload);
      calloc.free(event);
      runtime.close();
    }
  });
}
