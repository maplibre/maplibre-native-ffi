/**
 * The check that decides whether a runtime payload describes this package's ABI.
 *
 * The call ABI indexes entrypoints rather than naming them, so a payload
 * generated from other headers would dispatch an id to another function with
 * another signature. That is memory corruption rather than a missing symbol,
 * which is why this runs before a transport is built rather than at the first
 * call that goes wrong.
 *
 * The rule belongs to the binding, not to one transport, so it lives here and
 * every transport calls it. A conformance case can then prove it on whichever
 * transport it is running over.
 */

import { ABI_FINGERPRINT, ABI_HEADER_DIGEST } from "../raw/fingerprint.ts";

/** Thrown when an installed runtime payload does not match this package. */
export class AbiMismatchError extends Error {
  override name = "AbiMismatchError";
}

/** What a payload reports about the ABI it was built from. */
export interface PayloadIdentity {
  /** The schema fingerprint the payload's dispatch was generated from. */
  readonly fingerprint: string;
  /** The digest of the public headers the payload was built against. */
  readonly headerDigest: string;
}

/**
 * Accepts a payload that agrees with this package, and refuses one that does
 * not.
 */
export function verifyPayload(payload: PayloadIdentity): void {
  if (payload.fingerprint !== ABI_FINGERPRINT) {
    throw new AbiMismatchError(
      `the installed runtime reports ABI fingerprint ${payload.fingerprint}, ` +
        `and this package was generated from ${ABI_FINGERPRINT}`,
    );
  }
  if (payload.headerDigest !== ABI_HEADER_DIGEST) {
    throw new AbiMismatchError(
      `the installed runtime was built against public headers digesting to ` +
        `${payload.headerDigest}, and this package was generated from ` +
        `${ABI_HEADER_DIGEST}`,
    );
  }
}

/** What a payload that agrees with this package reports. */
export function expectedPayloadIdentity(): PayloadIdentity {
  return { fingerprint: ABI_FINGERPRINT, headerDigest: ABI_HEADER_DIGEST };
}
