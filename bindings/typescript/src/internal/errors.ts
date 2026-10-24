import { Status } from "../generated_values.ts";

const names = new Map<number, string>(
  Object.entries(Status).map(([name, value]) => [value, name]),
);

/** The name of a status, or `unknown` for a code this binding does not know. */
export function statusName(status: number): string {
  return names.get(status) ?? "unknown";
}

/**
 * A failed MapLibre Native call: its status, the status name, the call's own
 * diagnostic message, and for a map command, the generation it published.
 */
export class MaplibreError extends Error {
  /** The `mln_status` code, kept as is when this binding does not know it. */
  readonly status: Status;
  /** The status as a name of {@link Status}, or `unknown`. */
  readonly statusName: string;
  /** The diagnostic message of the failed call, which may be empty. */
  readonly diagnostic: string;
  /**
   * The map snapshot generation that a failed or cancelled map command
   * published, and zero for any other call.
   */
  readonly generation: bigint;

  constructor(status: number, diagnostic: string, generation = 0n) {
    const name = statusName(status);
    super(diagnostic ? `${name}: ${diagnostic}` : name);
    this.name = "MaplibreError";
    this.status = status;
    this.statusName = name;
    this.diagnostic = diagnostic;
    this.generation = generation;
  }
}

/**
 * The loaded addon does not match this package. Install the facade and the
 * addon package at the same version. The status is {@link Status.unsupported}.
 */
export class AbiVersionMismatchError extends MaplibreError {
  constructor(diagnostic: string) {
    super(Status.unsupported, diagnostic);
    this.name = "AbiVersionMismatchError";
  }
}
