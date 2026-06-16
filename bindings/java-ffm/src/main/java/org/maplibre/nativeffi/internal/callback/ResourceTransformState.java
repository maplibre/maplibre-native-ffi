package org.maplibre.nativeffi.internal.callback;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.nio.charset.StandardCharsets;
import java.util.Objects;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_resource_transform;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_callback;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.resource.ResourceTransformCallback;
import org.maplibre.nativeffi.resource.ResourceTransformRequest;

/** Owns runtime-scoped resource transform callback state. */
public final class ResourceTransformState implements AutoCloseable {
  private final Arena arena;
  private final CallbackLifecycle lifecycle = new CallbackLifecycle();
  private final ResourceTransformCallback callback;
  private final MemorySegment stub;
  private final MemorySegment descriptor;

  public ResourceTransformState(ResourceTransformCallback callback) {
    this.arena = Arena.ofShared();
    this.callback = callback;
    this.stub = mln_resource_transform_callback.allocate(this::invoke, arena);
    this.descriptor = mln_resource_transform.allocate(arena);
    mln_resource_transform.size(descriptor, (int) mln_resource_transform.sizeof());
    mln_resource_transform.callback(descriptor, stub);
    mln_resource_transform.user_data(descriptor, MemorySegment.NULL);
  }

  public MemorySegment descriptor() {
    return descriptor;
  }

  public boolean isCurrentThreadInCallback() {
    return lifecycle.isCurrentThreadInCallback();
  }

  private int invoke(
      MemorySegment userData, int rawKind, MemorySegment url, MemorySegment outResponse) {
    var lease = lifecycle.enter();
    if (lease.isEmpty()) {
      return MapLibreNativeC.MLN_STATUS_NATIVE_ERROR();
    }
    try (var ignored = lease.get()) {
      mln_resource_transform_response.size(
          outResponse, (int) mln_resource_transform_response.sizeof());
      mln_resource_transform_response.url(outResponse, MemorySegment.NULL);
      var replacement =
          callback.transform(
              new ResourceTransformRequest(
                  NativeValues.resourceKind(rawKind), rawKind, MemoryUtil.copyCString(url)));
      Objects.requireNonNull(replacement, "replacement");
      if (replacement.isPresent() && !replacement.get().isEmpty()) {
        return setResponseUrl(outResponse, replacement.get());
      }
      return MapLibreNativeC.MLN_STATUS_OK();
    } catch (IllegalArgumentException error) {
      return MapLibreNativeC.MLN_STATUS_INVALID_ARGUMENT();
    } catch (Throwable ignored) {
      return MapLibreNativeC.MLN_STATUS_NATIVE_ERROR();
    }
  }

  @Override
  public void close() {
    lifecycle.close("Resource transform", arena::close);
  }

  private static int setResponseUrl(MemorySegment outResponse, String value) {
    var bytes = value.getBytes(StandardCharsets.UTF_8);
    try (var responseArena = Arena.ofConfined()) {
      return MapLibreNativeC.mln_resource_transform_response_set_url(
          outResponse, responseArena.allocateFrom(ValueLayout.JAVA_BYTE, bytes), bytes.length);
    }
  }
}
