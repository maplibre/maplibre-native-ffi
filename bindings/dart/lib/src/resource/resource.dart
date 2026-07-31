/// Resource requests, responses, transforms, providers, and request handles.
library;

import 'dart:typed_data';

/// Resource kind requested by MapLibre Native.
final class ResourceKind {
  const ResourceKind._(this.rawValue, this.name);

  /// Unknown resource kind.
  static const unknown = ResourceKind._(0, 'unknown');

  /// Style document.
  static const style = ResourceKind._(1, 'style');

  /// Source metadata.
  static const source = ResourceKind._(2, 'source');

  /// Tile payload.
  static const tile = ResourceKind._(3, 'tile');

  /// Glyph payload.
  static const glyphs = ResourceKind._(4, 'glyphs');

  /// Sprite image.
  static const spriteImage = ResourceKind._(5, 'spriteImage');

  /// Sprite JSON.
  static const spriteJson = ResourceKind._(6, 'spriteJson');

  /// Image payload.
  static const image = ResourceKind._(7, 'image');

  /// Creates a resource kind from a raw native value.
  factory ResourceKind.fromRawValue(int rawValue) => switch (rawValue) {
    0 => unknown,
    1 => style,
    2 => source,
    3 => tile,
    4 => glyphs,
    5 => spriteImage,
    6 => spriteJson,
    7 => image,
    _ => ResourceKind._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource loading method.
final class ResourceLoadingMethod {
  const ResourceLoadingMethod._(this.rawValue, this.name);

  /// All loading methods.
  static const all = ResourceLoadingMethod._(0, 'all');

  /// Cache-only loading.
  static const cacheOnly = ResourceLoadingMethod._(1, 'cacheOnly');

  /// Network-only loading.
  static const networkOnly = ResourceLoadingMethod._(2, 'networkOnly');

  /// Creates a resource loading method from a raw native value.
  factory ResourceLoadingMethod.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => all,
        1 => cacheOnly,
        2 => networkOnly,
        _ => ResourceLoadingMethod._(rawValue, 'unknown($rawValue)'),
      };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource priority.
final class ResourcePriority {
  const ResourcePriority._(this.rawValue, this.name);

  /// Regular priority.
  static const regular = ResourcePriority._(0, 'regular');

  /// Low priority.
  static const low = ResourcePriority._(1, 'low');

  /// Creates a resource priority from a raw native value.
  factory ResourcePriority.fromRawValue(int rawValue) => switch (rawValue) {
    0 => regular,
    1 => low,
    _ => ResourcePriority._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource usage.
final class ResourceUsage {
  const ResourceUsage._(this.rawValue, this.name);

  /// Online usage.
  static const online = ResourceUsage._(0, 'online');

  /// Offline usage.
  static const offline = ResourceUsage._(1, 'offline');

  /// Creates a resource usage from a raw native value.
  factory ResourceUsage.fromRawValue(int rawValue) => switch (rawValue) {
    0 => online,
    1 => offline,
    _ => ResourceUsage._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource storage policy.
final class ResourceStoragePolicy {
  const ResourceStoragePolicy._(this.rawValue, this.name);

  /// Permanent storage.
  static const permanent = ResourceStoragePolicy._(0, 'permanent');

  /// Volatile storage.
  static const volatile = ResourceStoragePolicy._(1, 'volatile');

  /// Creates a resource storage policy from a raw native value.
  factory ResourceStoragePolicy.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => permanent,
        1 => volatile,
        _ => ResourceStoragePolicy._(rawValue, 'unknown($rawValue)'),
      };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource response status.
final class ResourceResponseStatus {
  const ResourceResponseStatus._(this.rawValue, this.name);

  /// Successful response.
  static const ok = ResourceResponseStatus._(0, 'ok');

  /// Error response.
  static const error = ResourceResponseStatus._(1, 'error');

  /// No-content response.
  static const noContent = ResourceResponseStatus._(2, 'noContent');

  /// Not-modified response.
  static const notModified = ResourceResponseStatus._(3, 'notModified');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Resource error reason.
final class ResourceErrorReason {
  const ResourceErrorReason._(this.rawValue, this.name);

  /// No error.
  static const none = ResourceErrorReason._(0, 'none');

  /// Resource was not found.
  static const notFound = ResourceErrorReason._(1, 'notFound');

  /// Server returned an error.
  static const server = ResourceErrorReason._(2, 'server');

  /// Connection failed.
  static const connection = ResourceErrorReason._(3, 'connection');

  /// Request was rate-limited.
  static const rateLimit = ResourceErrorReason._(4, 'rateLimit');

  /// Other resource error.
  static const other = ResourceErrorReason._(5, 'other');

  /// Creates an error reason from a raw native value.
  factory ResourceErrorReason.fromRawValue(int rawValue) => switch (rawValue) {
    0 => none,
    1 => notFound,
    2 => server,
    3 => connection,
    4 => rateLimit,
    5 => other,
    _ => ResourceErrorReason._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Exact URL rewrite rule used by the runtime resource transform.
final class ResourceUrlRewriteRule {
  /// Creates an exact URL rewrite rule.
  const ResourceUrlRewriteRule({
    this.kind,
    required this.url,
    required this.replacementUrl,
  });

  /// Optional resource kind filter. Null matches any kind.
  final ResourceKind? kind;

  /// Original URL to match exactly.
  final String url;

  /// Replacement URL returned to native code.
  final String replacementUrl;
}

/// Copied resource request delivered to a Dart resource provider.
final class ResourceRequest {
  /// Creates a copied resource request.
  ResourceRequest({
    required this.requestedUrl,
    required this.resolvedUrl,
    required this.kind,
    required this.loadingMethod,
    required this.priority,
    required this.usage,
    required this.storagePolicy,
    this.range,
    this.priorModifiedUnixMs,
    this.priorExpiresUnixMs,
    this.priorEtag,
    Uint8List? priorData,
  }) : priorData = priorData == null
           ? null
           : Uint8List.fromList(priorData).asUnmodifiableView();

  /// URL entering the network layer, preserving configured scheme aliases.
  final String requestedUrl;

  /// URL to fetch, after tile server normalization.
  final String resolvedUrl;

  /// Requested resource kind.
  final ResourceKind kind;

  /// Loading method requested by native code.
  final ResourceLoadingMethod loadingMethod;

  /// Request priority.
  final ResourcePriority priority;

  /// Request usage.
  final ResourceUsage usage;

  /// Request storage policy.
  final ResourceStoragePolicy storagePolicy;

  /// Optional byte range as inclusive start/end values.
  final ({BigInt start, BigInt end})? range;

  /// Optional previously modified timestamp in Unix milliseconds.
  final int? priorModifiedUnixMs;

  /// Optional previously expires timestamp in Unix milliseconds.
  final int? priorExpiresUnixMs;

  /// Optional previous entity tag.
  final String? priorEtag;

  /// Optional previous data bytes copied from native code.
  final Uint8List? priorData;
}

/// Exact URL route used by a queued Dart resource provider callback.
final class ResourceProviderRoute {
  /// Creates an exact URL provider route.
  const ResourceProviderRoute({this.kind, required this.requestedUrl});

  /// Optional resource kind filter. Null matches any kind.
  final ResourceKind? kind;

  /// Requested URL to match exactly.
  final String requestedUrl;
}

/// Exact URL provider rule used by the runtime resource provider.
final class ResourceProviderRule {
  /// Creates an exact URL provider rule.
  const ResourceProviderRule({
    this.kind,
    required this.requestedUrl,
    required this.response,
  });

  /// Optional resource kind filter. Null matches any kind.
  final ResourceKind? kind;

  /// Requested URL to match exactly.
  final String requestedUrl;

  /// Response to complete for matching requests.
  final ResourceResponse response;
}

/// Resource response returned by a Dart resource provider.
final class ResourceResponse {
  /// Creates a resource response.
  ResourceResponse({
    required this.status,
    this.errorReason = ResourceErrorReason.none,
    Uint8List? bytes,
    this.errorMessage,
    this.mustRevalidate = false,
    this.modifiedUnixMs,
    this.expiresUnixMs,
    this.etag,
    this.retryAfterUnixMs,
  }) : bytes = bytes == null
           ? null
           : Uint8List.fromList(bytes).asUnmodifiableView();

  /// Response status.
  final ResourceResponseStatus status;

  /// Error reason for error responses.
  final ResourceErrorReason errorReason;

  /// Optional response bytes.
  final Uint8List? bytes;

  /// Optional error message.
  final String? errorMessage;

  /// Whether cached data must be revalidated.
  final bool mustRevalidate;

  /// Optional modified timestamp in Unix milliseconds.
  final int? modifiedUnixMs;

  /// Optional expiration timestamp in Unix milliseconds.
  final int? expiresUnixMs;

  /// Optional entity tag.
  final String? etag;

  /// Optional retry-after timestamp in Unix milliseconds.
  final int? retryAfterUnixMs;
}
