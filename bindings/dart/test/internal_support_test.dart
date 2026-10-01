import 'package:maplibre_native_ffi/src/values.dart';
import 'dart:convert';
import 'dart:ffi';

import 'package:maplibre_native_ffi/src/error/maplibre_exception.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:maplibre_native_ffi/src/internal/status/status.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart';
import 'package:ffi/ffi.dart';
import 'package:test/test.dart';

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

    test('native statuses map to their exceptions and keep unknown codes', () {
      final cases = <(int, Type)>[
        (nativeStatusInvalidArgument, InvalidArgumentException),
        (nativeStatusInvalidState, InvalidStateException),
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
  });

  group('native string helpers', () {
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
        final empty = nativeStringView('', arena).value;
        expect(empty.size, 0);
        expect(empty.data, isNot(nullptr));
      });
    });
  });

  test(
    'a drained batch is indexed by its stride and copied field by field',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      // A stride wider than this binding's own record is what a C API version
      // that added a payload member reports, so the decoder indexes by it.
      final eventSize = sizeOf<raw.mln_runtime_event>() + 8;
      final payloadOffset =
          sizeOf<raw.mln_runtime_event>() -
          sizeOf<raw.mln_runtime_event_payload>();
      final events = calloc<Uint8>(eventSize * 3);
      final messageBytes = utf8.encode('copied message\u0000tile-source\u0000');
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
              runtime.identity.toSigned(64).toInt(),
              outBatch,
              nullptr,
            ),
            nativeStatusOk,
          );
          try {
            expect(
              raw.mln_event_batch_get(outBatch.value, view, nullptr),
              nativeStatusOk,
            );
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

        final renderMap = (events + eventSize)
            .cast<raw.mln_runtime_event>()
            .ref;
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
        transition.type = 22;
        transition.source_type = 0;
        transition.code = 0;
        transition.payload_type = raw
            .mln_runtime_event_payload_type
            .MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED;
        transition.payload.camera_transition_finished.transition_id = -1;
        transition.message_offset = 15;
        transition.message_size = 11;

        batch.ref.size = sizeOf<raw.mln_runtime_event_batch_view>();
        batch.ref.event_size = eventSize;
        batch.ref.events = events.cast<raw.mln_runtime_event>();
        batch.ref.event_count = 3;
        batch.ref.messages = messages.cast<Char>();
        batch.ref.messages_size = messageBytes.length;
        final decoded = decodeRuntimeEventBatchForTesting(batch.ref, runtime);
        // Every field is copied before the drain returns, so overwriting the
        // arena cannot change what the host already holds.
        unknownWindow.fillRange(0, unknownWindow.length, 9);
        messages[0] = 'X'.codeUnitAt(0);

        expect(decoded, hasLength(3));

        final unknownEvent = decoded[0];
        expect(unknownEvent.type.rawValue, 0xfeed);
        expect(unknownEvent.code, 17);
        expect(unknownEvent.sourceType.rawValue, 0xbeef);
        expect(unknownEvent.source, BigInt.from(0xcafe));
        expect(unknownEvent.message, 'copied message');
        final unknownPayload = unknownEvent.payload;
        expect(unknownPayload, isA<RuntimeEventPayloadUnknown>());
        expect((unknownPayload as RuntimeEventPayloadUnknown).tag, 0xf00d);
        expect(
          (unknownPayload).rawRecord.sublist(payloadOffset),
          List.generate(eventSize - payloadOffset, (index) => index + 1),
        );

        // The second and third events decode only when the walk steps by the
        // reported stride rather than by this binding's own record size.
        final renderMapEvent = decoded[1];
        expect(renderMapEvent.type, RuntimeEventType.mapRenderMapFinished);
        expect(renderMapEvent.sourceType, RuntimeEventSourceType.map);
        expect(renderMapEvent.source, BigInt.from(0xfeed));
        expect(renderMapEvent.message, isEmpty);
        expect(
          (renderMapEvent.payload as RuntimeEventPayloadRenderMap).value.mode,
          RenderMode.full,
        );

        final transitionEvent = decoded[2];
        expect(
          transitionEvent.type,
          RuntimeEventType.mapCameraTransitionFinished,
        );
        expect(transitionEvent.sourceType, RuntimeEventSourceType.runtime);
        expect(transitionEvent.message, 'tile-source');
        expect(
          (transitionEvent.payload
                  as RuntimeEventPayloadCameraTransitionFinished)
              .value
              .transitionId,
          (BigInt.one << 64) - BigInt.one,
        );
      } finally {
        calloc.free(batch);
        calloc.free(messages);
        calloc.free(events);
        await runtime.close();
      }
    },
  );
}
