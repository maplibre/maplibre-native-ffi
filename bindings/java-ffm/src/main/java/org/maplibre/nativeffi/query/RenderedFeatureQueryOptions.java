package org.maplibre.nativeffi.query;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativeffi.json.JsonValue;

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

  public RenderedFeatureQueryOptions layerIds(List<String> layerIds) {
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

  public RenderedFeatureQueryOptions filter(JsonValue filter) {
    this.filter = Objects.requireNonNull(filter, "filter");
    return this;
  }

  public RenderedFeatureQueryOptions clearFilter() {
    filter = null;
    return this;
  }
}
