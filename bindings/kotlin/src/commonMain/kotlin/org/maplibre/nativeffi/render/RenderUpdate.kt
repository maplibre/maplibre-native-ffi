package org.maplibre.nativeffi.render

/**
 * Outcome of a successful render-update call.
 *
 * [needsRepaint] is true only when [result] is [RenderResult.RENDERED] and the map asked for
 * another frame while it rendered this one, as during an ongoing camera transition. It carries the
 * same signal as the map-render-frame-finished event's needs-repaint field, without the event round
 * trip.
 */
public data class RenderUpdate(public val result: RenderResult, public val needsRepaint: Boolean)
