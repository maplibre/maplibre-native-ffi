package org.maplibre.nativeffi;

import java.util.Objects;

/** Offline region definition copied into native storage at creation time. */
public sealed interface OfflineRegionDefinition
    permits OfflineRegionDefinition.TilePyramid, OfflineRegionDefinition.GeometryRegion {
  record TilePyramid(
      String styleUrl,
      LatLngBounds bounds,
      double minZoom,
      double maxZoom,
      float pixelRatio,
      boolean includeIdeographs)
      implements OfflineRegionDefinition {
    public TilePyramid {
      Objects.requireNonNull(styleUrl, "styleUrl");
      Objects.requireNonNull(bounds, "bounds");
      validateZoomRange(minZoom, maxZoom);
      validatePixelRatio(pixelRatio);
    }
  }

  record GeometryRegion(
      String styleUrl,
      Geometry geometry,
      double minZoom,
      double maxZoom,
      float pixelRatio,
      boolean includeIdeographs)
      implements OfflineRegionDefinition {
    public GeometryRegion {
      Objects.requireNonNull(styleUrl, "styleUrl");
      Objects.requireNonNull(geometry, "geometry");
      validateZoomRange(minZoom, maxZoom);
      validatePixelRatio(pixelRatio);
    }
  }

  private static void validateZoomRange(double minZoom, double maxZoom) {
    if (!Double.isFinite(minZoom) || minZoom < 0.0 || Double.isNaN(maxZoom) || maxZoom < minZoom) {
      throw new IllegalArgumentException("offline region zoom range is invalid");
    }
  }

  private static void validatePixelRatio(float pixelRatio) {
    if (!Float.isFinite(pixelRatio) || pixelRatio < 0.0f) {
      throw new IllegalArgumentException("pixelRatio must be finite and non-negative");
    }
  }
}
