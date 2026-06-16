package org.maplibre.nativeffi.map;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.geo.CanonicalTileId;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id;
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options;
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_tile_callback;
import org.maplibre.nativeffi.internal.callback.CallbackLifecycle;
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions;

/** Owns map/style-scoped custom geometry source callback state. */
final class CustomGeometrySourceState implements AutoCloseable {
  private final Arena arena;
  private final CustomGeometrySourceOptions options;
  private final MemorySegment fetchStub;
  private final MemorySegment cancelStub;
  private final MemorySegment descriptor;
  private final CallbackLifecycle lifecycle = new CallbackLifecycle();

  CustomGeometrySourceState(CustomGeometrySourceOptions options) {
    this.arena = Arena.ofShared();
    this.options = options;
    this.fetchStub = mln_custom_geometry_source_tile_callback.allocate(this::fetchTile, arena);
    this.cancelStub = mln_custom_geometry_source_tile_callback.allocate(this::cancelTile, arena);
    this.descriptor = MapLibreNativeC.mln_custom_geometry_source_options_default(arena);
    mln_custom_geometry_source_options.fetch_tile(descriptor, fetchStub);
    mln_custom_geometry_source_options.cancel_tile(descriptor, cancelStub);
    mln_custom_geometry_source_options.user_data(descriptor, MemorySegment.NULL);
    writeFields();
  }

  MemorySegment descriptor() {
    return descriptor;
  }

  private void writeFields() {
    if ((options.hasTileSize() && options.tileSize() < 0)
        || (options.hasBuffer() && options.buffer() < 0)) {
      throw new InvalidArgumentException(
          MapLibreNativeC.MLN_STATUS_INVALID_ARGUMENT(),
          "custom geometry source unsigned options must be non-negative");
    }
    var fields = 0;
    if (options.hasMinZoom()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM();
      mln_custom_geometry_source_options.min_zoom(descriptor, options.minZoom());
    }
    if (options.hasMaxZoom()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM();
      mln_custom_geometry_source_options.max_zoom(descriptor, options.maxZoom());
    }
    if (options.hasTolerance()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE();
      mln_custom_geometry_source_options.tolerance(descriptor, options.tolerance());
    }
    if (options.hasTileSize()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE();
      mln_custom_geometry_source_options.tile_size(descriptor, options.tileSize());
    }
    if (options.hasBuffer()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER();
      mln_custom_geometry_source_options.buffer(descriptor, options.buffer());
    }
    if (options.hasClip()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP();
      mln_custom_geometry_source_options.clip(descriptor, options.clip());
    }
    if (options.hasWrap()) {
      fields |= MapLibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP();
      mln_custom_geometry_source_options.wrap(descriptor, options.wrap());
    }
    mln_custom_geometry_source_options.fields(descriptor, fields);
  }

  private void fetchTile(MemorySegment userData, MemorySegment tileId) {
    var lease = lifecycle.enter();
    if (lease.isEmpty()) {
      return;
    }
    try (var ignored = lease.get()) {
      options.callback().fetchTile(canonicalTileId(tileId));
    } catch (Throwable ignored) {
      // Native callbacks must not unwind through the C ABI.
    }
  }

  private void cancelTile(MemorySegment userData, MemorySegment tileId) {
    var lease = lifecycle.enter();
    if (lease.isEmpty()) {
      return;
    }
    try (var ignored = lease.get()) {
      options.callback().cancelTile(canonicalTileId(tileId));
    } catch (Throwable ignored) {
      // Native callbacks must not unwind through the C ABI.
    }
  }

  private static CanonicalTileId canonicalTileId(MemorySegment segment) {
    return new CanonicalTileId(
        mln_canonical_tile_id.z(segment),
        Integer.toUnsignedLong(mln_canonical_tile_id.x(segment)),
        Integer.toUnsignedLong(mln_canonical_tile_id.y(segment)));
  }

  @Override
  public void close() {
    lifecycle.close("Custom geometry source", arena::close);
  }
}
