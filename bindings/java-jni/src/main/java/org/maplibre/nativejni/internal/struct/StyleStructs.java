package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import org.maplibre.nativejni.style.StyleImageOptions;
import org.maplibre.nativejni.style.TileSourceOptions;

/** Internal materializers for style source, image, layer, and custom geometry values. */
public final class StyleStructs {
  private StyleStructs() {}

  public record TileSourceOptionsValue(
      boolean hasMinZoom,
      double minZoom,
      boolean hasMaxZoom,
      double maxZoom,
      String attribution,
      Integer scheme,
      CoreStructs.LatLngBoundsValue bounds,
      Integer tileSize,
      Integer vectorEncoding,
      Integer rasterDemEncoding) {}

  public record StyleImageOptionsValue(
      boolean hasPixelRatio, float pixelRatio, boolean hasSdf, boolean sdf) {}

  public static TileSourceOptionsValue tileSourceOptions(TileSourceOptions options) {
    Objects.requireNonNull(options, "options");
    return new TileSourceOptionsValue(
        options.hasMinZoom(),
        options.hasMinZoom() ? options.minZoom() : 0,
        options.hasMaxZoom(),
        options.hasMaxZoom() ? options.maxZoom() : 0,
        options.hasAttribution() ? options.attribution() : null,
        options.hasScheme() ? options.scheme().nativeValue() : null,
        options.hasBounds() ? CoreStructs.latLngBounds(options.bounds()) : null,
        options.hasTileSize() ? options.tileSize() : null,
        options.hasVectorEncoding() ? options.vectorEncoding().nativeValue() : null,
        options.hasRasterDemEncoding() ? options.rasterDemEncoding().nativeValue() : null);
  }

  public static StyleImageOptionsValue styleImageOptions(StyleImageOptions options) {
    Objects.requireNonNull(options, "options");
    return new StyleImageOptionsValue(
        options.hasPixelRatio(),
        options.hasPixelRatio() ? options.pixelRatio() : 0,
        options.hasSdf(),
        options.hasSdf() && options.sdf());
  }
}
