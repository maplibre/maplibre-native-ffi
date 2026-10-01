package org.maplibre.nativeffi.internal.lifecycle

/**
 * Disposes and reports [report]'s handle once [owner] becomes unreachable unreleased.
 *
 * The owner keeps the returned value in a field, which ties the cleanup to the owner's lifetime on
 * a platform whose cleaner is itself a collected object.
 */
internal expect fun trackLeak(owner: Any, report: HandleStateCore.LeakReport): Any

/** Closes [core] once [owner] becomes unreachable. The owner keeps the returned value too. */
internal expect fun closeWhenUnreachable(owner: Any, core: DecisionOwnerCore): Any
