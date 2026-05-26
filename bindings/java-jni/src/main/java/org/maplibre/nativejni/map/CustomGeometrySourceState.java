package org.maplibre.nativejni.map;

import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;

/** Owns map/style-scoped custom geometry source callback state. */
final class CustomGeometrySourceState implements AutoCloseable {
  private final CustomGeometrySourceOptions options;
  private final MaplibreNativeC.mln_custom_geometry_source_tile_callback fetchTile;
  private final MaplibreNativeC.mln_custom_geometry_source_tile_callback cancelTile;
  private final MaplibreNativeC.mln_custom_geometry_source_options descriptor;
  private final Object callbackLock = new Object();

  private int activeCallbacks;
  private boolean closeRequested;
  private boolean closed;

  CustomGeometrySourceState(CustomGeometrySourceOptions options) {
    this.options = options;
    this.fetchTile =
        new MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
          @Override
          public void call(Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
            CustomGeometrySourceState.this.fetchTile(tileId);
          }
        };
    this.cancelTile =
        new MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
          @Override
          public void call(Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
            CustomGeometrySourceState.this.cancelTile(tileId);
          }
        };
    this.descriptor = MaplibreNativeC.mln_custom_geometry_source_options_default();
    descriptor.fetch_tile(fetchTile);
    descriptor.cancel_tile(cancelTile);
    descriptor.user_data(JavaCppSupport.pointer(0));
    writeFields();
  }

  MaplibreNativeC.mln_custom_geometry_source_options descriptor() {
    return descriptor;
  }

  private void writeFields() {
    var fields = 0;
    if (options.hasMinZoom()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
      descriptor.min_zoom(options.minZoom());
    }
    if (options.hasMaxZoom()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
      descriptor.max_zoom(options.maxZoom());
    }
    if (options.hasTolerance()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
      descriptor.tolerance(options.tolerance());
    }
    if (options.hasTileSize()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
      descriptor.tile_size(options.tileSize());
    }
    if (options.hasBuffer()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
      descriptor.buffer(options.buffer());
    }
    if (options.hasClip()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
      descriptor.clip(options.clip());
    }
    if (options.hasWrap()) {
      fields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
      descriptor.wrap(options.wrap());
    }
    descriptor.fields(fields);
  }

  private void fetchTile(MaplibreNativeC.mln_canonical_tile_id tileId) {
    if (!enterCallback()) {
      return;
    }
    try {
      options.callback().fetchTile(canonicalTileId(tileId));
    } catch (Throwable ignored) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      exitCallback();
    }
  }

  private void cancelTile(MaplibreNativeC.mln_canonical_tile_id tileId) {
    if (!enterCallback()) {
      return;
    }
    try {
      options.callback().cancelTile(canonicalTileId(tileId));
    } catch (Throwable ignored) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      exitCallback();
    }
  }

  private boolean enterCallback() {
    synchronized (callbackLock) {
      if (closed) {
        return false;
      }
      activeCallbacks++;
      return true;
    }
  }

  private void exitCallback() {
    var shouldClose = false;
    synchronized (callbackLock) {
      activeCallbacks--;
      shouldClose = closeRequested && activeCallbacks == 0 && !closed;
      if (shouldClose) {
        closed = true;
      }
    }
    if (shouldClose) {
      closeNative();
    }
  }

  private static CanonicalTileId canonicalTileId(MaplibreNativeC.mln_canonical_tile_id tileId) {
    return new CanonicalTileId(
        tileId.z(), Integer.toUnsignedLong(tileId.x()), Integer.toUnsignedLong(tileId.y()));
  }

  @Override
  public void close() {
    var shouldClose = false;
    synchronized (callbackLock) {
      if (closed || closeRequested) {
        return;
      }
      closeRequested = true;
      shouldClose = activeCallbacks == 0;
      if (shouldClose) {
        closed = true;
      }
    }
    if (shouldClose) {
      closeNative();
    }
  }

  private void closeNative() {
    descriptor.close();
    fetchTile.close();
    cancelTile.close();
  }
}
