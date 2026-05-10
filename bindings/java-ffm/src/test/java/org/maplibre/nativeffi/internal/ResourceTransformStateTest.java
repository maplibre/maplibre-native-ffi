package org.maplibre.nativeffi.internal;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Optional;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.ResourceKind;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_resource_transform;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_callback;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response;

final class ResourceTransformStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void copiesReplacementUrlIntoNativeResponse() {
    try (var state =
            new ResourceTransformState(request -> Optional.of(request.url() + "?token=1"));
        var arena = Arena.ofConfined()) {
      var response = mln_resource_transform_response.allocate(arena);
      var status =
          mln_resource_transform_callback.invoke(
              mln_resource_transform.callback(state.descriptor()),
              MemorySegment.NULL,
              ResourceKind.STYLE.nativeValue(),
              MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
              response);

      assertEquals(MapLibreNativeC.MLN_STATUS_OK(), status);
      assertEquals(
          "https://example.test/style.json?token=1",
          MemoryUtil.copyCString(mln_resource_transform_response.url(response)));
    }
  }

  @Test
  void callbackExceptionsBecomeNativeErrorStatus() {
    try (var state =
            new ResourceTransformState(
                request -> {
                  throw new AssertionError("boom");
                });
        var arena = Arena.ofConfined()) {
      var response = mln_resource_transform_response.allocate(arena);
      var status =
          mln_resource_transform_callback.invoke(
              mln_resource_transform.callback(state.descriptor()),
              MemorySegment.NULL,
              ResourceKind.STYLE.nativeValue(),
              MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
              response);

      assertEquals(MapLibreNativeC.MLN_STATUS_NATIVE_ERROR(), status);
    }
  }
}
