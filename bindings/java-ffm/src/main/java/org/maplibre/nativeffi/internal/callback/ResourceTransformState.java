package org.maplibre.nativeffi.internal.callback;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_resource_transform;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_callback;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.resource.ResourceKind;
import org.maplibre.nativeffi.resource.ResourceTransformCallback;
import org.maplibre.nativeffi.resource.ResourceTransformRequest;

/** Owns runtime-scoped resource transform callback state. */
public final class ResourceTransformState implements AutoCloseable {
  private final Arena arena;
  private final ResourceTransformCallback callback;
  private final MemorySegment stub;
  private final MemorySegment descriptor;
  private final ConcurrentHashMap<Thread, Arena> responseArenas = new ConcurrentHashMap<>();

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

  private int invoke(
      MemorySegment userData, int rawKind, MemorySegment url, MemorySegment outResponse) {
    try {
      closePreviousResponseArena();
      mln_resource_transform_response.size(
          outResponse, (int) mln_resource_transform_response.sizeof());
      mln_resource_transform_response.url(outResponse, MemorySegment.NULL);
      var replacement =
          callback.transform(
              new ResourceTransformRequest(
                  ResourceKind.fromNative(rawKind), rawKind, MemoryUtil.copyCString(url)));
      Objects.requireNonNull(replacement, "replacement");
      if (replacement.isPresent() && !replacement.get().isEmpty()) {
        var responseArena = Arena.ofShared();
        responseArenas.put(Thread.currentThread(), responseArena);
        mln_resource_transform_response.url(
            outResponse, MemoryUtil.allocateCString(responseArena, replacement.get()));
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
    responseArenas.values().forEach(ResourceTransformState::closeQuietly);
    responseArenas.clear();
    arena.close();
  }

  private void closePreviousResponseArena() {
    closeQuietly(responseArenas.remove(Thread.currentThread()));
  }

  private static void closeQuietly(Arena arena) {
    if (arena == null) {
      return;
    }
    try {
      arena.close();
    } catch (IllegalStateException ignored) {
      // Closing callback scratch memory is best-effort during runtime teardown.
    }
  }
}
