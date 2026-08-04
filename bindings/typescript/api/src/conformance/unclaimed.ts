/**
 * Specification cases this suite does not claim, and why.
 *
 * The suite tags a case with the specification cases it proves, so coverage is
 * countable. Counting only what is claimed would let a gap sit unnoticed, so
 * every remaining identifier is named here with the reason it is not claimed.
 * `//bindings/typescript:check-spec-coverage` fails when an identifier is
 * neither tagged nor listed, so a new specification case has to be classified
 * rather than silently missed, and an entry here that a case later covers has
 * to be removed.
 *
 * A reason is a statement about this binding, not an excuse. "Not written yet"
 * is a legitimate one and is spelled that way.
 */

export type UnclaimedReason =
  /** The behaviour cannot occur in this binding, and the entry says why. */
  | "inapplicable"
  /** Reachable through the public API, but no case exercises it yet. */
  | "unwritten";

export interface UnclaimedCase {
  readonly id: string;
  readonly reason: UnclaimedReason;
  readonly note: string;
}

/**
 * A binding reaches the C API from one JavaScript execution context. Node,
 * Bun, Deno, and ArkTS each give a worker its own module instance rather than a
 * second thread onto the same one, and a WebAssembly module's handles belong to
 * the agent that made them. There is therefore no way to call a handle from
 * another thread that owns it, which is what the thread-affinity cases require.
 */
const NO_SECOND_THREAD =
  "no runtime this binding supports exposes one library instance to two threads";

export const UNCLAIMED: readonly UnclaimedCase[] = [
  // Error mapping.

  // Handle lifetime.
  {
    id: "BND-044",
    reason: "inapplicable",
    note: "JavaScript finalization is not a cleanup hook this binding runs native release from",
  },
  {
    id: "BND-046",
    reason: "inapplicable",
    note: NO_SECOND_THREAD,
  },
  {
    id: "BND-047",
    reason: "unwritten",
    note: "handles are separate classes here, so a mismatch needs a seam that forges an id of the wrong kind, which faults.ts does not do",
  },
  { id: "BND-049", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Values and events.
  {
    id: "BND-066",
    reason: "unwritten",
    note: "the seam can force the copy to fail, and whether the snapshot was given back is not observable through the public API",
  },

  // Maps, queries, and style.
  { id: "BND-123", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Resource requests.
  {
    id: "BND-145",
    reason: "inapplicable",
    note: NO_SECOND_THREAD,
  },
  { id: "BND-153", reason: "inapplicable", note: NO_SECOND_THREAD },
  {
    id: "BND-158",
    reason: "unwritten",
    note: "needs the adapter's HTTP header callback family, which this binding does not wrap yet",
  },
  {
    id: "BND-159",
    reason: "unwritten",
    note: "needs the adapter's HTTP header callback family, which this binding does not wrap yet",
  },

  // Render sessions. The browser suite attaches one through a real WebGL
  // context, so what remains is not the context but the API: the binding
  // exposes no CPU readback, no owned-texture frames, and no `set_target`.
  {
    id: "BND-172",
    reason: "inapplicable",
    note: "the frame wrapper's construction cannot fail: it is built from a record already copied out",
  },
  { id: "BND-174", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Threading.
  ...([190, 191, 192, 193, 194, 195, 197] as const).map((number) => ({
    id: `BND-${number}`,
    reason: "inapplicable" as const,
    note: NO_SECOND_THREAD,
  })),
];
