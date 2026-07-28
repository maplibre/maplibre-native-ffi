part of 'runtime.dart';

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

final class _ResourceProviderRulesState {
  _ResourceProviderRulesState(List<ResourceProviderRule> rules) {
    for (final rule in rules) {
      _checkNativeCString(rule.url);
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
      pointer.ref.rules[index].url = _nativeOwnedCString(rule.url);
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
      calloc.free(rules[index].url);
      _freeNativeResourceResponse(rules[index].response, calloc);
    }
    if (rules != nullptr) {
      calloc.free(rules);
    }
    calloc.free(pointer);
  }
}

final class _ResourceProviderCallbackState extends RetainedCallbackState {
  _ResourceProviderCallbackState(ResourceProvider provider) {
    for (final route in provider.routes) {
      _checkNativeCString(route.url);
    }
    callback =
        NativeCallable<
          raw.mln_adapter_queued_resource_request_listenerFunction
        >.listener((Pointer<Void> request) {
          if (request == nullptr) {
            close();
            return;
          }
          final ran = runUpcall(
            () => _invokeQueuedResourceProvider(provider.callback, request),
          );
          if (!ran) {
            _dropQueuedResourceProviderRequest(request);
          }
        });
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
      pointer.ref.routes[index].url = _nativeOwnedCString(route.url);
    }
    pointer.ref.listener = callback.nativeFunction;
  }

  late final Pointer<raw.mln_adapter_queued_resource_provider> pointer;
  late final NativeCallable<
    raw.mln_adapter_queued_resource_request_listenerFunction
  >
  callback;
  var _retirementQueued = false;

  void retire() {
    if (_retirementQueued) {
      return;
    }
    _retirementQueued = true;
    _c.raw.mln_adapter_queued_resource_provider_retire(pointer);
  }

  @override
  void closeResources() {
    final routes = pointer.ref.routes;
    for (var index = 0; index < pointer.ref.route_count; index += 1) {
      calloc.free(routes[index].url);
    }
    if (routes != nullptr) {
      calloc.free(routes);
    }
    calloc.free(pointer);
    callback.close();
  }
}

void _dropQueuedResourceProviderRequest(Pointer<Void> rawRequest) {
  try {
    final request = rawRequest
        .cast<raw.mln_adapter_queued_resource_request>()
        .ref;
    final handle = ResourceRequestHandle._(request.handle);
    if (!handle.isReleased) {
      try {
        handle.complete(
          ResourceResponse(
            status: ResourceResponseStatus.error,
            errorReason: ResourceErrorReason.other,
            errorMessage: 'Dart resource provider callback was retired',
          ),
        );
      } catch (_) {
        handle.close();
      }
    }
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
    final handle = ResourceRequestHandle._(request.handle);
    try {
      callback(_copyResourceRequest(request), handle);
    } catch (_) {
      if (!handle.isReleased) {
        try {
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.error,
              errorReason: ResourceErrorReason.other,
              errorMessage: 'Dart resource provider callback threw',
            ),
          );
        } catch (_) {
          handle.close();
        }
      }
    }
  } finally {
    _c.adapterResourceProviderRequestDestroy(rawRequest);
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
    url: request.url.cast<Utf8>().toDartString(),
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
