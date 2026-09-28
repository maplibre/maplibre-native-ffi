/// Resource requests, responses, transforms, providers, and request handles.
library;

import '../generated_values.dart';
export '../generated_values.dart';

/// URL route used by a queued Dart resource provider callback.
final class ResourceProviderRoute {
  /// Creates a provider route.
  const ResourceProviderRoute({
    this.kind,
    required this.url,
    this.matchGlob = false,
    this.useRequestedUrl = false,
  });

  /// Optional resource kind filter. Null matches any kind.
  final ResourceKind? kind;

  /// URL comparison value, compared case-sensitively with no URL parsing or
  /// normalization.
  final String url;

  /// Reads [url] as a glob pattern: `*` excludes slashes, `**` includes them,
  /// and `?` matches one character other than a slash.
  final bool matchGlob;

  /// Compares [ResourceRequest.requestedUrl] instead of
  /// [ResourceRequest.resolvedUrl].
  final bool useRequestedUrl;
}
