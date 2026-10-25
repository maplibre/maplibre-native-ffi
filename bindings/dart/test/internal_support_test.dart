import 'package:maplibre_native_ffi/src/values.dart';
import 'dart:convert';
import 'dart:ffi';

import 'package:maplibre_native_ffi/src/error/maplibre_exception.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:maplibre_native_ffi/src/internal/status/status.dart';

import 'support/runtime_probes.dart';
import 'package:ffi/ffi.dart';
import 'package:test/test.dart';

void main() {
  group('status conversion', () {
    test('ABI mismatch has a stable public error category', () {
      expect(
        () => validateCAbiVersion(expectedCAbiVersion + 1),
        throwsA(
          isA<AbiVersionMismatchException>()
              .having((error) => error.status, 'status', Status.unsupported)
              .having(
                (error) => error.expected,
                'expected version',
                expectedCAbiVersion,
              ),
        ),
      );
    });

    test('native statuses map to their exceptions and keep unknown codes', () {
      final cases = <(int, Status)>[
        (nativeStatusInvalidArgument, Status.invalidArgument),
        (nativeStatusInvalidState, Status.invalidState),
        (nativeStatusUnsupported, Status.unsupported),
        (nativeStatusNativeError, Status.nativeError),
      ];

      for (final (status, type) in cases) {
        try {
          checkNativeStatus(status, () => 'diagnostic $status');
          fail('status $status unexpectedly succeeded');
        } on MaplibreException catch (error) {
          expect(error.status, type);
          expect(error.status.rawValue, status);
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

      expect(error.status, const Status.fromRawValue(-999));
      expect(error.toString(), contains('Status(-999)'));
      expect(error.diagnostic, 'first diagnostic');
    });
  });

  test('an empty string view still points at storage', () {
    withNativeArena((arena) {
      final empty = nativeStringView('', arena).value;
      expect(empty.size, 0);
      expect(empty.data, isNot(nullptr));
    });
  });

  group('call arena', () {
    test('reuses zeroed memory across calls, including after a throw', () {
      final first = withNativeArena((arena) {
        final bytes = arena<Uint8>(64);
        bytes.asTypedList(64).fillRange(0, 64, 0xff);
        return bytes.address;
      });
      expect(
        () => withNativeArena((arena) {
          arena<Uint8>(64).asTypedList(64).fillRange(0, 64, 0xff);
          throw StateError('body failed');
        }),
        throwsStateError,
      );
      withNativeArena((arena) {
        final bytes = arena<Uint8>(64);
        expect(bytes.address, first);
        expect(bytes.asTypedList(64), everyElement(0));
      });
    });

    test('gives a nested call memory apart from the outer call', () {
      withNativeArena((outer) {
        final value = outer<Uint64>()..value = 1;
        withNativeArena((inner) {
          final nested = inner<Uint64>();
          expect(nested.address, isNot(value.address));
          expect(nested.value, 0);
          nested.value = 2;
        });
        expect(value.value, 1);
      });
    });

    test('keeps every allocation of a call distinct, aligned, and zeroed', () {
      withNativeArena((arena) {
        final allocations = [
          for (var index = 0; index < 100; index++)
            (arena<Uint8>(index % 3 == 0 ? 4096 : 24), index),
        ];
        for (final (pointer, index) in allocations) {
          expect(pointer.address % 16, 0);
          expect(pointer.cast<Uint64>().value, 0);
          pointer.cast<Uint64>().value = index;
        }
        for (final (pointer, index) in allocations) {
          expect(pointer.cast<Uint64>().value, index);
        }
      });
    });
  });

  // A native build whose element grew reports a value_size wider than this
  // binding's element, so the reader steps by it. A stride narrower than the
  // element cannot hold one.
  test('an array completion is read at its value_size', () {
    // Two coordinates, each followed by a member this binding does not know.
    final stride = sizeOf<raw.mln_lat_lng>() + 8;
    final points = calloc<Uint8>(2 * stride);
    final result = calloc<raw.mln_completion_result>();
    try {
      for (final (index, (latitude, longitude)) in [
        (1.0, 2.0),
        (3.0, 4.0),
      ].indexed) {
        final point = (points + index * stride).cast<raw.mln_lat_lng>().ref;
        point.latitude = latitude;
        point.longitude = longitude;
      }
      result.ref.size = sizeOf<raw.mln_completion_result>();
      result.ref.status = nativeStatusOk;
      result.ref.value = points.cast();
      result.ref.value_count = 2;
      result.ref.value_size = stride;

      expect(decodeLatLngListForTesting(result.ref), const [
        LatLng(latitude: 1, longitude: 2),
        LatLng(latitude: 3, longitude: 4),
      ]);
      result.ref.value_size = sizeOf<raw.mln_lat_lng>() - 1;
      expect(
        () => decodeLatLngListForTesting(result.ref),
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.invalidState,
          ),
        ),
      );
    } finally {
      calloc.free(result);
      calloc.free(points);
    }
  });

  test('a drained batch is indexed by its stride and copied field by field', () {
    final eventSize = sizeOf<raw.mln_runtime_event>() + 8;
    final payloadOffset =
        sizeOf<raw.mln_runtime_event>() -
        sizeOf<raw.mln_runtime_event_payload>();
    final events = calloc<Uint8>(eventSize * 2);
    final messageBytes = utf8.encode('copied message\u0000');
    final messages = calloc<Uint8>(messageBytes.length);
    final batch = calloc<raw.mln_event_batch_view>();
    try {
      messages.asTypedList(messageBytes.length).setAll(0, messageBytes);

      final unknown = (events + 0).cast<raw.mln_runtime_event>().ref;
      unknown.type = 0xfeed;
      unknown.source_type = 0xbeef;
      unknown.source = 0xcafe;
      unknown.generation = 0x100000005;
      unknown.code = 17;
      unknown.payload_type = 0xf00d;
      unknown.message.data = messages.cast();
      unknown.message.size = 14;
      final unknownWindow = (events + payloadOffset).asTypedList(
        eventSize - payloadOffset,
      );
      for (var index = 0; index < unknownWindow.length; index += 1) {
        unknownWindow[index] = index + 1;
      }

      final renderMap = (events + eventSize).cast<raw.mln_runtime_event>().ref;
      renderMap.type = raw.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED;
      renderMap.source_type = raw.MLN_RUNTIME_EVENT_SOURCE_MAP;
      // A map id this build cannot resolve to a wrapper still names one object
      // for the life of the process, so the raw id has to survive the decode:
      // it is the only identity a host can route or correlate the event on.
      renderMap.source = 0xfeed;
      renderMap.code = 0;
      renderMap.payload_type = raw.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
      renderMap.payload.render_map.mode = raw.MLN_RENDER_MODE_FULL;

      batch.ref.size = sizeOf<raw.mln_event_batch_view>();
      batch.ref.event_size = eventSize;
      batch.ref.events = events.cast<raw.mln_runtime_event>();
      batch.ref.event_count = 2;
      final decoded = decodeRuntimeEventBatchForTesting(batch.ref);
      // Every field is copied before the drain returns, so overwriting the
      // arena cannot change what the host already holds.
      unknownWindow.fillRange(0, unknownWindow.length, 9);
      messages[0] = 'X'.codeUnitAt(0);

      expect(decoded, hasLength(2));

      final unknownEvent = decoded[0];
      expect(unknownEvent.type.rawValue, 0xfeed);
      expect(unknownEvent.code, 17);
      expect(unknownEvent.sourceType.rawValue, 0xbeef);
      expect(unknownEvent.source, 0xcafe);
      expect(unknownEvent.generation, 0x100000005);
      expect(unknownEvent.message, 'copied message');
      final unknownPayload = unknownEvent.payload;
      expect(unknownPayload, isA<RuntimeEventPayloadUnknown>());
      expect((unknownPayload as RuntimeEventPayloadUnknown).tag, 0xf00d);
      expect(
        (unknownPayload).rawRecord.sublist(payloadOffset),
        List.generate(eventSize - payloadOffset, (index) => index + 1),
      );

      // The second event decodes only when the walk steps by the
      // reported stride rather than by this binding's own record size.
      final renderMapEvent = decoded[1];
      expect(renderMapEvent.type, RuntimeEventType.mapRenderMapFinished);
      expect(renderMapEvent.sourceType, RuntimeEventSourceType.map);
      expect(renderMapEvent.source, 0xfeed);
      expect(renderMapEvent.message, isEmpty);
      expect(
        (renderMapEvent.payload as RuntimeEventPayloadRenderMap).value.mode,
        RenderMode.full,
      );
    } finally {
      calloc.free(batch);
      calloc.free(messages);
      calloc.free(events);
    }
  });
}
