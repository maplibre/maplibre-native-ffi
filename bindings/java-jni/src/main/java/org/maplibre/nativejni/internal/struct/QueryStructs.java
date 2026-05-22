package org.maplibre.nativejni.internal.struct;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;

/** Internal materializers for query descriptors and copied query results. */
public final class QueryStructs {
  private QueryStructs() {}

  public record RenderedFeatureQueryOptionsValue(
      boolean hasLayerIds, List<String> layerIds, boolean hasFilter, ValueStructs.JsonNode filter) {
    public RenderedFeatureQueryOptionsValue {
      layerIds = layerIds == null ? List.of() : List.copyOf(layerIds);
    }
  }

  public record SourceFeatureQueryOptionsValue(
      boolean hasSourceLayerIds,
      List<String> sourceLayerIds,
      boolean hasFilter,
      ValueStructs.JsonNode filter) {
    public SourceFeatureQueryOptionsValue {
      sourceLayerIds = sourceLayerIds == null ? List.of() : List.copyOf(sourceLayerIds);
    }
  }

  public static RenderedFeatureQueryOptionsValue renderedFeatureQueryOptions(
      RenderedFeatureQueryOptions options) {
    Objects.requireNonNull(options, "options");
    return new RenderedFeatureQueryOptionsValue(
        options.hasLayerIds(),
        options.hasLayerIds() ? options.layerIds() : List.of(),
        options.hasFilter(),
        options.hasFilter() ? ValueStructs.jsonValue(options.filter()) : null);
  }

  public static SourceFeatureQueryOptionsValue sourceFeatureQueryOptions(
      SourceFeatureQueryOptions options) {
    Objects.requireNonNull(options, "options");
    return new SourceFeatureQueryOptionsValue(
        options.hasSourceLayerIds(),
        options.hasSourceLayerIds() ? options.sourceLayerIds() : List.of(),
        options.hasFilter(),
        options.hasFilter() ? ValueStructs.jsonValue(options.filter()) : null);
  }
}
