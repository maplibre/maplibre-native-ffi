part of 'runtime.dart';

int _urlMatchFlags(bool matchGlob) => matchGlob
    ? raw.mln_adapter_url_match_flags.MLN_ADAPTER_URL_MATCH_GLOB.value
    : raw.mln_adapter_url_match_flags.MLN_ADAPTER_URL_MATCH_FLAGS_NONE.value;

final class _ResourceTransformState {
  _ResourceTransformState(List<ResourceUrlRewriteRule> rules) {
    for (final rule in rules) {
      _checkNativeCString(rule.url);
      _checkNativeCString(rule.replacementUrl);
    }
    pointer = calloc<raw.mln_adapter_resource_rewrite_rules>();
    pointer.ref.count = rules.length;
    pointer.ref.rules = rules.isEmpty
        ? nullptr.cast<raw.mln_adapter_resource_rewrite_rule>()
        : calloc<raw.mln_adapter_resource_rewrite_rule>(rules.length);
    for (var index = 0; index < rules.length; index += 1) {
      final rule = rules[index];
      pointer.ref.rules[index].kind =
          rule.kind?.rawValue ?? _resourceKindWildcard;
      pointer.ref.rules[index].flags = _urlMatchFlags(rule.matchGlob);
      pointer.ref.rules[index].url = _nativeOwnedCString(rule.url);
      pointer.ref.rules[index].replacement_url = _nativeOwnedCString(
        rule.replacementUrl,
      );
    }
  }

  late final Pointer<raw.mln_adapter_resource_rewrite_rules> pointer;

  void close() {
    final rules = pointer.ref.rules;
    for (var index = 0; index < pointer.ref.count; index += 1) {
      calloc.free(rules[index].url);
      calloc.free(rules[index].replacement_url);
    }
    if (rules != nullptr) {
      calloc.free(rules);
    }
    calloc.free(pointer);
  }
}

final class _HttpHeaderTransformState {
  _HttpHeaderTransformState(List<HttpHeaderTransformRule> rules) {
    for (final rule in rules) {
      _checkNativeCString(rule.url);
      final names = <String>{};
      for (final header in rule.headers) {
        _validateHttpHeader(header);
        final folded = header.name.toLowerCase();
        if (!names.add(folded)) {
          throwInvalidArgument(
            'HTTP header names must be unique ignoring case: ${header.name}',
          );
        }
      }
    }
    pointer = calloc<raw.mln_adapter_http_header_transform_rules>();
    pointer.ref.count = rules.length;
    pointer.ref.rules = rules.isEmpty
        ? nullptr.cast<raw.mln_adapter_http_header_transform_rule>()
        : calloc<raw.mln_adapter_http_header_transform_rule>(rules.length);
    for (var ruleIndex = 0; ruleIndex < rules.length; ruleIndex += 1) {
      final rule = rules[ruleIndex];
      final nativeRule = pointer.ref.rules[ruleIndex];
      nativeRule.kind = rule.kind?.rawValue ?? _resourceKindWildcard;
      nativeRule.flags = _urlMatchFlags(rule.matchGlob);
      nativeRule.url = _nativeOwnedCString(rule.url);
      nativeRule.header_count = rule.headers.length;
      nativeRule.headers = rule.headers.isEmpty
          ? nullptr.cast<raw.mln_adapter_http_header>()
          : calloc<raw.mln_adapter_http_header>(rule.headers.length);
      for (
        var headerIndex = 0;
        headerIndex < rule.headers.length;
        headerIndex += 1
      ) {
        final header = rule.headers[headerIndex];
        nativeRule.headers[headerIndex].name = _nativeOwnedCString(header.name);
        nativeRule.headers[headerIndex].value = _nativeOwnedCString(
          header.value,
        );
      }
    }
  }

  late final Pointer<raw.mln_adapter_http_header_transform_rules> pointer;

  void close() {
    final rules = pointer.ref.rules;
    for (var ruleIndex = 0; ruleIndex < pointer.ref.count; ruleIndex += 1) {
      final rule = rules[ruleIndex];
      for (
        var headerIndex = 0;
        headerIndex < rule.header_count;
        headerIndex += 1
      ) {
        calloc.free(rule.headers[headerIndex].name);
        calloc.free(rule.headers[headerIndex].value);
      }
      if (rule.headers != nullptr) {
        calloc.free(rule.headers);
      }
      calloc.free(rule.url);
    }
    if (rules != nullptr) {
      calloc.free(rules);
    }
    calloc.free(pointer);
  }
}

void _validateHttpHeader(HttpHeader header) {
  _checkNativeCString(header.name);
  _checkNativeCString(header.value);
  withNativeArena((arena) {
    final name = nativeUtf8CString(header.name, arena);
    final value = nativeUtf8CString(header.value, arena);
    _check(
      raw.mln_adapter_http_header_validate(
        name.pointer.cast<Char>(),
        value.pointer.cast<Char>(),
      ),
    );
  });
}

final class _ResourceProviderRulesState {
  _ResourceProviderRulesState(List<ResourceProviderRule> rules) {
    for (final rule in rules) {
      _checkNativeCString(rule.requestedUrl);
      _checkResourceResponseNativeStrings(rule.response);
    }
    pointer = calloc<raw.mln_adapter_resource_provider_rules>();
    pointer.ref.count = rules.length;
    pointer.ref.rules = rules.isEmpty
        ? nullptr.cast<raw.mln_adapter_resource_provider_rule>()
        : calloc<raw.mln_adapter_resource_provider_rule>(rules.length);
    for (var index = 0; index < rules.length; index += 1) {
      final rule = rules[index];
      pointer.ref.rules[index].kind =
          rule.kind?.rawValue ?? _resourceKindWildcard;
      pointer.ref.rules[index].flags = _urlMatchFlags(rule.matchGlob);
      pointer.ref.rules[index].requested_url = _nativeOwnedCString(
        rule.requestedUrl,
      );
      pointer.ref.rules[index].response = _resourceResponseToNative(
        rule.response,
        calloc,
      );
    }
  }

  late final Pointer<raw.mln_adapter_resource_provider_rules> pointer;

  void close() {
    final rules = pointer.ref.rules;
    for (var index = 0; index < pointer.ref.count; index += 1) {
      calloc.free(rules[index].requested_url);
      _freeNativeResourceResponse(rules[index].response, calloc);
    }
    if (rules != nullptr) {
      calloc.free(rules);
    }
    calloc.free(pointer);
  }
}

int _resourceRouteFlags(ResourceProviderRoute route) {
  var flags = raw
      .mln_adapter_resource_route_flags
      .MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE
      .value;
  if (route.matchGlob) {
    flags |= raw
        .mln_adapter_resource_route_flags
        .MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB
        .value;
  }
  if (route.useRequestedUrl) {
    flags |= raw
        .mln_adapter_resource_route_flags
        .MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL
        .value;
  }
  return flags;
}

final class _ResourceProviderCallbackState extends RetainedCallbackState {
  _ResourceProviderCallbackState(ResourceProvider provider)
    : _callback = provider.callback {
    for (final route in provider.routes) {
      _checkNativeCString(route.url);
    }
    listener = NativeCallable<raw.mln_wake_callbackFunction>.listener((
      Pointer<Void> _,
    ) {
      runUpcall(drain);
    });
    final outQueue = calloc<Uint64>();
    final wake = calloc<raw.mln_wake>();
    try {
      wake.ref.size = sizeOf<raw.mln_wake>();
      wake.ref.callback = listener.nativeFunction;
      wake.ref.user_data = nullptr;
      wake.ref.release_user_data = nullptr;
      _check(raw.mln_adapter_resource_request_queue_create(wake, outQueue));
      queue = outQueue.value;
    } catch (_) {
      listener.close();
      rethrow;
    } finally {
      calloc.free(wake);
      calloc.free(outQueue);
    }
    pointer = calloc<raw.mln_adapter_queued_resource_provider>();
    pointer.ref.route_count = provider.routes.length;
    pointer.ref.routes = provider.routes.isEmpty
        ? nullptr.cast<raw.mln_adapter_queued_resource_provider_route>()
        : calloc<raw.mln_adapter_queued_resource_provider_route>(
            provider.routes.length,
          );
    for (var index = 0; index < provider.routes.length; index += 1) {
      final route = provider.routes[index];
      pointer.ref.routes[index].kind =
          route.kind?.rawValue ?? _resourceKindWildcard;
      pointer.ref.routes[index].flags = _resourceRouteFlags(route);
      pointer.ref.routes[index].url = _nativeOwnedCString(route.url);
    }
    pointer.ref.queue = queue;
  }

  final ResourceProviderCallback _callback;
  late final NativeCallable<raw.mln_wake_callbackFunction> listener;
  late final int queue;
  late final Pointer<raw.mln_adapter_queued_resource_provider> pointer;

  void drain() {
    withNativeArena((arena) {
      final outRequest =
          arena<Pointer<raw.mln_adapter_queued_resource_request>>();
      while (true) {
        // The acquire requires the null handle on entry, and the pointer is
        // reused across iterations.
        outRequest.value = nullptr;
        _check(
          raw.mln_adapter_resource_request_queue_acquire(queue, outRequest),
        );
        final request = outRequest.value.cast<Void>();
        if (request == nullptr) {
          return;
        }
        final ran = runUpcall(
          () => _invokeQueuedResourceProvider(_callback, request),
        );
        if (!ran) {
          _dropQueuedResourceProviderRequest(request);
        }
      }
    });
  }

  void retire() => closeSynchronously();

  @override
  void closeResources() {
    drain();
    raw.mln_adapter_resource_request_queue_close(queue);
    final routes = pointer.ref.routes;
    for (var index = 0; index < pointer.ref.route_count; index += 1) {
      calloc.free(routes[index].url);
    }
    if (routes != nullptr) {
      calloc.free(routes);
    }
    calloc.free(pointer);
    listener.close();
  }
}

void _dropQueuedResourceProviderRequest(Pointer<Void> rawRequest) {
  try {
    _failQueuedRequest(
      rawRequest,
      'Dart resource provider callback was retired',
    );
  } finally {
    _c.adapterResourceProviderRequestDestroy(rawRequest);
  }
}

void _invokeQueuedResourceProvider(
  ResourceProviderCallback callback,
  Pointer<Void> rawRequest,
) {
  try {
    final request = rawRequest
        .cast<raw.mln_adapter_queued_resource_request>()
        .ref;
    final handle = ResourceRequestHandle._(
      NativeResourceRequest(request.handle),
    );
    try {
      callback(_copyResourceRequest(request), handle);
    } catch (_) {
      _failQueuedRequest(rawRequest, 'Dart resource provider callback threw');
    }
  } finally {
    _c.adapterResourceProviderRequestDestroy(rawRequest);
  }
}

/// Completes the request in [rawRequest] with an error, and releases it
/// instead when that completion is itself rejected.
void _failQueuedRequest(Pointer<Void> rawRequest, String errorMessage) {
  final handle = ResourceRequestHandle._(
    NativeResourceRequest(
      rawRequest.cast<raw.mln_adapter_queued_resource_request>().ref.handle,
    ),
  );
  try {
    handle.complete(
      ResourceResponse(
        status: ResourceResponseStatus.error,
        errorReason: ResourceErrorReason.other,
        errorMessage: errorMessage,
      ),
    );
  } catch (_) {
    handle.close();
  }
}

ResourceRequest _copyResourceRequest(
  raw.mln_adapter_queued_resource_request request,
) {
  final priorData =
      request.prior_data == nullptr || request.prior_data_size == 0
      ? null
      : Uint8List.fromList(
          request.prior_data.asTypedList(request.prior_data_size),
        );
  return ResourceRequest(
    requestedUrl: request.requested_url.cast<Utf8>().toDartString(),
    resolvedUrl: request.resolved_url.cast<Utf8>().toDartString(),
    kind: ResourceKind.fromRawValue(request.kind),
    loadingMethod: ResourceLoadingMethod.fromRawValue(request.loading_method),
    priority: ResourcePriority.fromRawValue(request.priority),
    usage: ResourceUsage.fromRawValue(request.usage),
    storagePolicy: ResourceStoragePolicy.fromRawValue(request.storage_policy),
    range: request.has_range
        ? (
            start: uint64FromNative(request.range_start),
            end: uint64FromNative(request.range_end),
          )
        : null,
    priorModifiedUnixMs: request.has_prior_modified
        ? request.prior_modified_unix_ms
        : null,
    priorExpiresUnixMs: request.has_prior_expires
        ? request.prior_expires_unix_ms
        : null,
    priorEtag: request.prior_etag == nullptr
        ? null
        : request.prior_etag.cast<Utf8>().toDartString(),
    priorData: priorData,
  );
}

/// Dart callback run when MapLibre cancels a provider resource request.
typedef ResourceRequestCancelCallback = void Function();

/// Cancel registrations made on this isolate, keyed by request handle id.
///
/// An entry lives from registration until the request is retired on this
/// isolate or its callback has run. Completion, release, and native cancellation
/// all look the state up here, so a cancel message that arrives after the
/// request was retired finds no entry and is dropped.
final Map<int, _ResourceRequestCancelState> _resourceRequestCancelStates = {};

final class _ResourceRequestCancelState {
  _ResourceRequestCancelState(this.requestId, this._callback) {
    listener =
        NativeCallable<
          raw.mln_resource_request_cancel_callbackFunction
        >.listener((Pointer<Void> _) => _deliver());
  }

  final int requestId;
  ResourceRequestCancelCallback? _callback;
  late final NativeCallable<raw.mln_resource_request_cancel_callbackFunction>
  listener;
  var _closed = false;

  /// Handles the queued native cancel message on the registering isolate.
  void _deliver() {
    if (!identical(_resourceRequestCancelStates[requestId], this)) {
      return;
    }
    _resourceRequestCancelStates.remove(requestId);
    runCallback();
    // Native invokes the callback at most once, so the callable is unused from
    // here on even while the host still owns the request.
    close();
  }

  /// Runs the host callback once, containing any exception it throws.
  void runCallback() {
    final callback = _callback;
    _callback = null;
    if (callback == null) {
      return;
    }
    try {
      callback();
    } catch (_) {
      // An exception must not escape into native callback delivery.
    }
  }

  void close() {
    if (_closed) {
      return;
    }
    _closed = true;
    _callback = null;
    listener.close();
  }
}

/// Drops the cancel registration for [requestId] once native release returned.
void _retireResourceRequestCancelState(int requestId) {
  _resourceRequestCancelStates.remove(requestId)?.close();
}
