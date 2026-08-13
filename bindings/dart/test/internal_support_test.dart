import 'dart:convert';
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
import 'package:maplibre_native_ffi/src/internal/lifecycle/native_handles.dart';

/// A handle of a distinct kind for tests that exercise binding-owned
/// bookkeeping without a live native object.
extension type const _FakeNativeHandle(int raw) implements NativeHandle {}

const _fakeHandle = _FakeNativeHandle(0x0200000000001234);

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
        _fakeHandle,
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
        _fakeHandle,
        'fake_handle',
        ownerIsolateHash: Isolate.current.hashCode + 1,
        leakReporting: false,
      );

      expect(() => state.handle, throwsA(isA<WrongThreadException>()));
      expect(
        () => state.close((_) => nativeStatusOk, () => 'unused'),
        throwsA(isA<WrongThreadException>()),
      );
    });

    test('owner native thread mismatch reports the isolate moved', () {
      // The isolate check still passes when the VM moves an isolate off its
      // original native thread, so the thread check has to catch it.
      final state = NativeHandleState<_FakeNativeHandle>(
        _fakeHandle,
        'fake_handle',
        ownerThreadToken: -1,
        leakReporting: false,
      );

      expect(
        () => state.handle,
        throwsA(
          isA<WrongThreadException>().having(
            (error) => error.diagnostic,
            'diagnostic',
            allOf(
              contains('native thread its isolate has since left'),
              contains('awaited I/O'),
            ),
          ),
        ),
      );
      expect(
        () => state.close((_) => nativeStatusOk, () => 'unused'),
        throwsA(isA<WrongThreadException>()),
      );
    });

    test('failed close leaves handle live for retry', () {
      final state = NativeHandleState<_FakeNativeHandle>(
        _fakeHandle,
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

  test('a drained batch is indexed by its stride and copied field by field', () {
    final runtime = RuntimeHandle.create();
    // A stride wider than this binding's own record is what a C API version
    // that added a payload member reports, so the decoder indexes by it.
    final eventSize = sizeOf<raw.mln_runtime_event>() + 8;
    final payloadOffset =
        sizeOf<raw.mln_runtime_event>() -
        sizeOf<raw.mln_runtime_event_payload>();
    final events = calloc<Uint8>(eventSize * 3);
    final messageBytes = utf8.encode('copied message tile-source ');
    final messages = calloc<Uint8>(messageBytes.length);
    final batch = calloc<raw.mln_runtime_event_batch_view>();
    try {
      // The library this binding runs against reports the record size this
      // binding compiled, so a later mismatch is an ABI change rather than a
      // decode bug.
      withNativeArena((arena) {
        final outBatch = arena<Uint64>();
        final view = arena<raw.mln_runtime_event_batch_view>();
        view.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
        expect(
          raw.mln_runtime_drain_events(
            runtimeHandleIdForTesting(runtime),
            0,
            outBatch,
          ),
          nativeStatusOk,
        );
        try {
          expect(raw.mln_event_batch_get(outBatch.value, view), nativeStatusOk);
          expect(view.ref.event_size, sizeOf<raw.mln_runtime_event>());
        } finally {
          raw.mln_event_batch_release(outBatch.value);
        }
      });

      messages.asTypedList(messageBytes.length).setAll(0, messageBytes);

      final unknown = (events + 0).cast<raw.mln_runtime_event>().ref;
      unknown.type = 0xfeed;
      unknown.source_type = 0xbeef;
      unknown.source = 0xcafe;
      unknown.code = 17;
      unknown.payload_type = 0xf00d;
      unknown.message_offset = 0;
      unknown.message_size = 14;
      final unknownWindow = (events + payloadOffset).asTypedList(
        eventSize - payloadOffset,
      );
      for (var index = 0; index < unknownWindow.length; index += 1) {
        unknownWindow[index] = index + 1;
      }

      final renderMap = (events + eventSize).cast<raw.mln_runtime_event>().ref;
      renderMap.type = 16;
      renderMap.source_type = 1;
      // A map id this build cannot resolve to a wrapper still names one object
      // for the life of the process, so the raw id has to survive the decode:
      // it is the only identity a host can route or correlate the event on.
      renderMap.source = 0xfeed;
      renderMap.code = 0;
      renderMap.payload_type = 2;
      renderMap.payload.render_map.mode = 1;

      final transition = (events + 2 * eventSize)
          .cast<raw.mln_runtime_event>()
          .ref;
      transition.type = 23;
      transition.source_type = 0;
      transition.code = 0;
      transition.payload_type = 9;
      transition.payload.camera_transition_finished.transition_id = -1;
      transition.message_offset = 15;
      transition.message_size = 11;

      batch.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
      batch.ref.event_size = eventSize;
      batch.ref.events = events.cast<raw.mln_runtime_event>();
      batch.ref.event_count = 3;
      batch.ref.messages = messages.cast<Char>();
      batch.ref.messages_size = messageBytes.length;
      batch.ref.remaining_count = 7;

      final decoded = decodeRuntimeEventBatchForTesting(batch.ref, runtime);
      // Every field is copied before the drain returns, so overwriting the
      // arena cannot change what the host already holds.
      unknownWindow.fillRange(0, unknownWindow.length, 9);
      messages[0] = 'X'.codeUnitAt(0);

      expect(decoded.remainingCount, 7);
      expect(decoded.events, hasLength(3));

      final unknownEvent = decoded.events[0];
      expect(unknownEvent.eventType.rawValue, 0xfeed);
      expect(unknownEvent.code, 17);
      final unknownSource = unknownEvent.source as UnknownRuntimeEventSource;
      expect(unknownSource.sourceType.rawValue, 0xbeef);
      expect(unknownSource.sourceId, 0xcafe);
      expect(unknownEvent.message, 'copied message');
      final unknownPayload = unknownEvent.payload;
      expect(unknownPayload, isA<RuntimeEventPayloadUnknown>());
      expect(unknownPayload.rawPayloadType, 0xf00d);
      expect(
        (unknownPayload as RuntimeEventPayloadUnknown).bytes,
        List.generate(eventSize - payloadOffset, (index) => index + 1),
      );

      // The second and third events decode only when the walk steps by the
      // reported stride rather than by this binding's own record size.
      final renderMapEvent = decoded.events[1];
      expect(renderMapEvent.eventType, RuntimeEventType.mapRenderMapFinished);
      final renderMapSource = renderMapEvent.source as MapRuntimeEventSource;
      expect(renderMapSource.map, isNull);
      expect(renderMapSource.sourceId, 0xfeed);
      // A zero message size is the absent message, not the empty one.
      expect(renderMapEvent.message, isNull);
      expect(
        (renderMapEvent.payload as RuntimeEventRenderMap).mode,
        RenderMode.full,
      );

      final transitionEvent = decoded.events[2];
      expect(
        transitionEvent.eventType,
        RuntimeEventType.mapCameraTransitionFinished,
      );
      expect(transitionEvent.source, isA<RuntimeRuntimeEventSource>());
      expect(transitionEvent.message, 'tile-source');
      expect(
        (transitionEvent.payload as RuntimeEventCameraTransitionFinished)
            .transitionId,
        (BigInt.one << 64) - BigInt.one,
      );
    } finally {
      calloc.free(batch);
      calloc.free(messages);
      calloc.free(events);
      runtime.close();
    }
  });
}
