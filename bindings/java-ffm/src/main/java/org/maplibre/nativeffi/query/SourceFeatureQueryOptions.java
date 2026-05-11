package org.maplibre.nativeffi.query;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativeffi.json.JsonValue;

/** Mutable options for source feature queries. */
public final class SourceFeatureQueryOptions {
  private List<String> sourceLayerIds;
  private JsonValue filter;

  public boolean hasSourceLayerIds() {
    return sourceLayerIds != null;
  }

  public List<String> sourceLayerIds() {
    return sourceLayerIds;
  }

  public SourceFeatureQueryOptions sourceLayerIds(List<String> sourceLayerIds) {
    this.sourceLayerIds = List.copyOf(Objects.requireNonNull(sourceLayerIds, "sourceLayerIds"));
    return this;
  }

  public SourceFeatureQueryOptions clearSourceLayerIds() {
    sourceLayerIds = null;
    return this;
  }

  public boolean hasFilter() {
    return filter != null;
  }

  public JsonValue filter() {
    return filter;
  }

  public SourceFeatureQueryOptions filter(JsonValue filter) {
    this.filter = Objects.requireNonNull(filter, "filter");
    return this;
  }

  public SourceFeatureQueryOptions clearFilter() {
    filter = null;
    return this;
  }
}
