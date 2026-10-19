// Resource and HTTP header transforms: native rule tables that rewrite and
// decorate the requests native loading sends, without a Dart callback.
import 'dart:io';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('transform rules rewrite a request with no callback', () async {
    final server = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
    addTearDown(() => server.close(force: true));
    final served = <(String, String?)>[];
    server.listen((request) {
      served.add((request.uri.path, request.headers.value('X-Token')));
      final response = request.response;
      if (request.uri.path == '/rewritten.json') {
        response.write(emptyStyleJson);
      } else {
        response.statusCode = HttpStatus.notFound;
      }
      response.close();
    });
    final base = 'http://127.0.0.1:${server.port}';
    final original = '$base/original.json';
    final rewritten = '$base/rewritten.json';

    final fixture = await openRuntime();
    // Native loading takes the request once no provider claims it.
    await within(fixture.runtime.clearResourceProvider(), 'provider clear');
    // In each table a rule for another kind matches the URL first, so the
    // request reaches the rule that names its kind.
    await within(
      fixture.runtime.setResourceTransform(
        ResourceTransform(
          rules: [
            ResourceRewriteRule(
              kinds: ResourceKindMask.tile,
              url: original,
              replacementUrl: '$base/tile.json',
            ),
            ResourceRewriteRule(
              kinds: ResourceKindMask.style,
              url: original,
              replacementUrl: rewritten,
            ),
          ],
        ),
      ),
      'resource transform registration',
    );
    // Header rules compare the URL the resource transform produced.
    await within(
      fixture.runtime.setHttpHeaderTransform(
        HttpHeaderTransform(
          rules: [
            HttpHeaderRule(
              kinds: ResourceKindMask.tile,
              url: rewritten,
              headers: [const HttpHeader(name: 'X-Token', value: 'tile')],
            ),
            HttpHeaderRule(
              url: rewritten,
              headers: [const HttpHeader(name: 'X-Token', value: 'style')],
            ),
          ],
        ),
      ),
      'header transform registration',
    );
    final map = await fixture.openMap();

    // The registrations returned before the request, so native rewrites it
    // from its own copy of each table.
    await expectCommitted(map.setStyleUrl(original));
    final loaded = await fixture.awaitEventType(
      RuntimeEventType.mapStyleLoaded,
    );
    expect(loaded.source, map.id);
    expect(served, [('/rewritten.json', 'style')]);
  });
}
