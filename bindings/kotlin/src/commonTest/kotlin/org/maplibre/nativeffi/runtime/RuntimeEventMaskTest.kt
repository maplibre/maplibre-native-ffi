package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class RuntimeEventMaskTest {
  @Test
  fun combiningAndRemovingSelectsExactlyTheNamedTypes() {
    val mask = RuntimeEventMask.MAP_IDLE + RuntimeEventMask.MAP_STYLE_LOADED
    assertTrue(RuntimeEventType.MAP_IDLE in mask)
    assertTrue(RuntimeEventType.MAP_STYLE_LOADED in mask)
    assertFalse(RuntimeEventType.MAP_TILE_ACTION in mask)
    assertFalse(mask.isEmpty())

    val narrowed = mask - RuntimeEventMask.MAP_IDLE
    assertEquals(RuntimeEventMask.MAP_STYLE_LOADED, narrowed)
    // Removing a type a mask never selected leaves it unchanged.
    assertEquals(narrowed, narrowed - RuntimeEventMask.MAP_TILE_ACTION)
    assertTrue((narrowed - RuntimeEventMask.MAP_STYLE_LOADED).isEmpty())

    // A mask holds one bit per type, so a type outside 0 through 63 has no bit
    // rather than the bit that shifting past the width would wrap onto.
    assertEquals(RuntimeEventMask.NONE, RuntimeEventMask.of(RuntimeEventType(64)))

    // The two groups name different types, so neither carries the other's bits.
    assertEquals(
      RuntimeEventMask.ALL_MAP_EVENTS,
      RuntimeEventMask.ALL_MAP_EVENTS - RuntimeEventMask.ALL_RUNTIME_EVENTS,
    )
  }
}
