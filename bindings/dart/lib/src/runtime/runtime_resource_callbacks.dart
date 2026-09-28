part of 'runtime.dart';

final class _ResourceProviderCallbackState extends RetainedCallbackState {
  _ResourceProviderCallbackState(QueuedResourceProvider provider)
    : _callback = provider.callback {
    var queueAccepted = false;
    var wakeCreated = false;
    try {
      final routes = provider.routes;
      pointer = arena<raw.mln_adapter_queued_resource_provider>();
      pointer.ref.route_count = routes.length;
      pointer.ref.routes = routes.isEmpty
          ? nullptr.cast<raw.mln_adapter_queued_resource_provider_route>()
          : arena<raw.mln_adapter_queued_resource_provider_route>(
              routes.length,
            );
      for (var index = 0; index < routes.length; index += 1) {
        pointer.ref.routes[index] = _writeAdapterQueuedResourceProviderRoute(
          routes[index],
          arena,
        ).ref;
      }
      wake = NativeWakeState(() => runUpcall(drain));
      wakeCreated = true;
      withNativeArena((temporary) {
        final outQueue = temporary<Uint64>();
        final descriptor = temporary<raw.mln_wake>();
        wake.writeTo(descriptor.ref);
        _check(
          raw.mln_adapter_resource_request_queue_create(descriptor, outQueue),
        );
        queueAccepted = true;
        queue = outQueue.value;
        arena.adoptHandle(queue);
      });
      pointer.ref.queue = queue;
    } catch (_) {
      if (wakeCreated && !queueAccepted) wake.reject();
      arena.releaseAll();
      rethrow;
    }
  }

  final ResourceProviderCallback _callback;
  final arena = NativeOwnedArena();
  late final NativeWakeState wake;
  late final int queue;
  late final Pointer<raw.mln_adapter_queued_resource_provider> pointer;

  void drain() {
    withNativeArena((arena) {
      final outRequest =
          arena<Pointer<raw.mln_adapter_queued_resource_request>>();
      while (true) {
        outRequest.value = nullptr;
        final status = raw.mln_adapter_resource_request_queue_acquire(
          queue,
          outRequest,
        );
        // Native release owns the queue and can precede delivery of its wake.
        if (status == raw.mln_status.MLN_STATUS_INVALID_ARGUMENT) return;
        _check(status);
        final request = outRequest.value.cast<Void>();
        if (request == nullptr) return;
        final ran = runUpcall(
          () => _invokeQueuedResourceProvider(_callback, request),
        );
        if (!ran) _dropQueuedResourceProviderRequest(request);
      }
    });
  }

  void retire() => closeSynchronously();

  @override
  void closeResources() => arena.releaseAll();
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
    final handle = _adoptOwned(
      request.handle,
      () => ResourceRequestHandle._(NativeResourceRequest(request.handle)),
      raw.mln_resource_request_release,
    );
    try {
      callback(_readAdapterQueuedResourceRequest(request), handle);
    } catch (_) {
      if (!handle.isClosed) {
        try {
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.error,
              errorReason: ResourceErrorReason.other,
              errorMessage: 'Dart resource provider callback threw',
            ),
          );
        } catch (_) {
        } finally {
          handle.close();
        }
      }
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
  } finally {
    handle.close();
  }
}

/// Dart callback run when MapLibre cancels a provider resource request.
typedef ResourceRequestCancelCallback = void Function();

/// Cancel registrations made on this isolate, keyed by request handle id.
///
bool _registerResourceCancellation(
  ResourceRequestHandle owner,
  void Function() callback,
) {
  final handle = owner._handle.raw;
  if (owner._cancelRegistration != null) {
    throwInvalidState('request already has a cancel registration');
  }
  final state = _ResourceRequestCancelState(owner, callback);
  owner._cancelRegistration = state;
  try {
    final cancelled = withNativeArena((arena) {
      final output = arena<Bool>();
      _check(
        raw.mln_adapter_dart_resource_cancel_register(
          handle,
          NativeApi.postCObject.cast(),
          state.port.sendPort.nativePort,
          output,
        ),
      );
      return output.value;
    });
    if (cancelled) {
      owner._cancelRegistration = null;
      state.close();
    }
    return cancelled;
  } catch (_) {
    owner._cancelRegistration = null;
    state.close();
    rethrow;
  }
}

final class _ResourceRequestCancelState {
  _ResourceRequestCancelState(
    ResourceRequestHandle owner,
    void Function() callback,
  ) : _owner = WeakReference(owner),
      _callback = callback {
    port = _cancelPort(WeakReference(this), Zone.current);
  }
  final WeakReference<ResourceRequestHandle> _owner;
  void Function()? _callback;
  late final RawReceivePort port;
  void deliver() {
    final owner = _owner.target;
    final callback = _callback;
    _callback = null;
    if (owner != null && !owner.isClosed && callback != null) {
      try {
        callback();
      } catch (_) {}
    }
  }

  void close() {
    _callback = null;
    port.close();
  }
}

RawReceivePort _cancelPort(
  WeakReference<_ResourceRequestCancelState> weak,
  Zone zone,
) {
  late final RawReceivePort port;
  port = RawReceivePort((dynamic message) {
    if (message == 1) {
      weak.target?._callback = null;
      port.close();
    } else if (message == 0) {
      zone.runGuarded(() => weak.target?.deliver());
    }
  });
  return port;
}
