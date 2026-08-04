/**
 * Making a C call fail on purpose.
 *
 * Some rules only show themselves when native code refuses something it
 * normally accepts: a destroy that fails must leave the handle live. Nothing a
 * caller can do provokes those, so the conformance suite arranges them here.
 *
 * This is not public API and no shipped code path reads it. A call checks one
 * boolean, which stays false unless a case has armed a fault and puts it back
 * afterwards, so the cost to a normal call is a branch that is never taken.
 */

/** Kept beside the map so the common case does not touch the map at all. */
let armed = false;
const forced = new Map<number, number[]>();

/** Makes the next `count` calls to `entrypoint` report `status` without running. */
export function forceStatus(
  entrypoint: number,
  status: number,
  count = 1,
): void {
  const queued = forced.get(entrypoint) ?? [];
  for (let index = 0; index < count; index += 1) {
    queued.push(status);
  }
  forced.set(entrypoint, queued);
  armed = true;
}

/** Takes the status a call should report, when one was arranged for it. */
export function takeForcedStatus(entrypoint: number): number | undefined {
  if (!armed) {
    return undefined;
  }
  const queued = forced.get(entrypoint);
  const status = queued?.shift();
  if (queued !== undefined && queued.length === 0) {
    forced.delete(entrypoint);
  }
  if (forced.size === 0) {
    armed = false;
  }
  return status;
}

/** Disarms everything, so a case that failed part-way leaves nothing behind. */
export function clearForcedStatuses(): void {
  forced.clear();
  armed = false;
}
