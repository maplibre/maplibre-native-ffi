package org.maplibre.nativeffi.internal.lifecycle

/**
 * Holds the render session [handle] so that the process abandons it at exit, unless [report] shows
 * that its owner released it first.
 *
 * C requires every render session's graphics calls to end before the process exits. On the JVM and
 * Android, a shutdown hook abandons the sessions held here, except one whose graphics thread is
 * inside driver service as the hook runs: C reports busy for that session, and its host ends it
 * before exit. A Kotlin/Native host ends its sessions itself, so there this holds nothing.
 */
internal expect fun endGraphicsAtExit(handle: Long, report: HandleStateCore.LeakReport)
