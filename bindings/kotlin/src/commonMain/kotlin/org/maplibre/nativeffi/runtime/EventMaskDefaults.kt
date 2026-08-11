package org.maplibre.nativeffi.runtime

/**
 * Returns the event mask the C runtime options default selects.
 *
 * The mask keeps the bits this version does not name, so a newer native library's default still
 * selects the event types it adds. Those events reach a host as unknown event and payload domains
 * rather than being suppressed.
 */
internal expect fun defaultRuntimeEventMask(): RuntimeEventMask

/**
 * Returns the event mask the C map options default selects, keeping unnamed bits the way
 * [defaultRuntimeEventMask] does.
 */
internal expect fun defaultMapEventMask(): RuntimeEventMask
