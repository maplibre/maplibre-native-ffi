package org.maplibre.nativejni.map;

import java.util.EnumSet;
import java.util.Set;

/** Copied map debug overlay mask. */
public record DebugOptions(Set<DebugOption> options) {
  public DebugOptions {
    options = options == null ? Set.of() : Set.copyOf(options);
  }

  public Set<DebugOption> asSet() {
    return options;
  }

  public boolean contains(DebugOption option) {
    return options.contains(option);
  }

  public boolean isEmpty() {
    return options.isEmpty();
  }

  public static DebugOptions fromMask(int mask) {
    var options = EnumSet.noneOf(DebugOption.class);
    for (var option : DebugOption.values()) {
      if ((mask & option.nativeMask()) != 0) {
        options.add(option);
      }
    }
    return new DebugOptions(options);
  }
}
