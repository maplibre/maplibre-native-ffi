// The isolate's delivery queue: deliveries that no registration of this
// isolate claims.
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show nativeDiagnostic;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/delivery_queue.dart';
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('a delivery for a registration this isolate no longer holds is released '
      'unread, which fails its request', () async {
    const styleUrl = 'custom://unclaimed-style.json';
    final fixture = await openRuntime();
    // A provider whose token names no registration of this isolate, as one
    // whose registration the isolate already dropped would.
    await startNativeCompletion<void>(
      start: (completion) => withNativeArena((arena) {
        final provider = arena<raw.mln_resource_provider>();
        provider.ref.size = sizeOf<raw.mln_resource_provider>();
        provider.ref.queue = isolateDeliveryQueue.handle;
        provider.ref.user_data = Pointer.fromAddress(0x7fffffff);
        return raw.mln_runtime_set_resource_provider(
          fixture.runtime.id.toSigned(64).toInt(),
          provider,
          completion,
          nativeDiagnostic,
        );
      }),
      decode: (_) {},
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final failure = await fixture.awaitEventType(
      RuntimeEventType.mapLoadingFailed,
    );
    expect(failure.message, contains('released without a response'));
  });
}
