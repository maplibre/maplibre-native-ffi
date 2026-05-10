package org.maplibre.nativeffi;

import java.util.List;
import java.util.Objects;

/** Mutable options for rendered feature queries. */
public final class RenderedFeatureQueryOptions {
  private List<String> layerIds;
  private JsonValue filter;

  public boolean hasLayerIds() {
    return layerIds != null;
  }

  public List<String> layerIds() {
    return layerIds;
  }

  public RenderedFeatureQueryOptions setLayerIds(List<String> layerIds) {
    this.layerIds = List.copyOf(Objects.requireNonNull(layerIds, "layerIds"));
    return this;
  }

  public RenderedFeatureQueryOptions clearLayerIds() {
    layerIds = null;
    return this;
  }

  public boolean hasFilter() {
    return filter != null;
  }

  public JsonValue filter() {
    return filter;
  }

  public RenderedFeatureQueryOptions setFilter(JsonValue filter) {
    this.filter = Objects.requireNonNull(filter, "filter");
    return this;
  }

  public RenderedFeatureQueryOptions clearFilter() {
    filter = null;
    return this;
  }
}
