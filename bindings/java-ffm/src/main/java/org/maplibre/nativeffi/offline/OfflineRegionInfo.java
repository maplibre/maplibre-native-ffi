package org.maplibre.nativeffi.offline;

import java.util.Arrays;
import java.util.Objects;

/** Offline region snapshot copied from native snapshot or list handles. */
public record OfflineRegionInfo(long id, OfflineRegionDefinition definition, byte[] metadata) {
  public OfflineRegionInfo {
    Objects.requireNonNull(definition, "definition");
    Objects.requireNonNull(metadata, "metadata");
    metadata = metadata.clone();
  }

  public byte[] metadata() {
    return metadata.clone();
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof OfflineRegionInfo that
        && id == that.id
        && definition.equals(that.definition)
        && Arrays.equals(metadata, that.metadata);
  }

  @Override
  public int hashCode() {
    var result = Long.hashCode(id);
    result = 31 * result + definition.hashCode();
    result = 31 * result + Arrays.hashCode(metadata);
    return result;
  }

  @Override
  public String toString() {
    return "OfflineRegionInfo[id="
        + id
        + ", definition="
        + definition
        + ", metadata="
        + metadata.length
        + " bytes]";
  }
}
