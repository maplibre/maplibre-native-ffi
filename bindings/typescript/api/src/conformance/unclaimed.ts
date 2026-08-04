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
  {
    id: "BND-020",
    reason: "unwritten",
    note: "one category is proven; a case driving every status category is not written",
  },
  {
    id: "BND-021",
    reason: "unwritten",
    note: "needs an internal conversion seam, as the network-status case has",
  },
  {
    id: "BND-025",
    reason: "unwritten",
    note: "binding-owned validation is proven; that it never reports a stale native diagnostic is not",
  },

  // Handle lifetime.
  {
    id: "BND-041",
    reason: "unwritten",
    note: "needs fault injection into native destroy, which no seam offers yet",
  },
  {
    id: "BND-043",
    reason: "unwritten",
    note: "a projection outliving its map is reachable and untested",
  },
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
    note: "handles are separate classes here, so this needs an internal seam to forge a mismatch",
  },
  {
    id: "BND-048",
    reason: "unwritten",
    note: "the leak channel exists and no case drives a best-effort cleanup failure",
  },
  { id: "BND-049", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Values and events.
  {
    id: "BND-066",
    reason: "unwritten",
    note: "needs fault injection into a copy that follows a native snapshot",
  },
  {
    id: "BND-069",
    reason: "unwritten",
    note: "copies are proven by content; that a later caller mutation cannot reach one is not",
  },
  {
    id: "BND-087",
    reason: "unwritten",
    note: "payload size is validated and no case proves the check precedes the read",
  },

  // Maps, queries, and style.
  {
    id: "BND-106",
    reason: "unwritten",
    note: "a session can be attached now, and the query it answers decodes features the library owns through a decoder that reads binding-owned slabs only, so this case cannot pass until that is fixed",
  },
  {
    id: "BND-107",
    reason: "unwritten",
    note: "the binding exposes no feature-extension query, and BND-106 blocks it besides",
  },
  { id: "BND-123", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Resource requests.
  {
    id: "BND-145",
    reason: "inapplicable",
    note: NO_SECOND_THREAD,
  },
  {
    id: "BND-146",
    reason: "unwritten",
    note: "double completion is refused by the binding and no case drives it",
  },
  {
    id: "BND-147",
    reason: "unwritten",
    note: "a released request reporting closed is untested",
  },
  {
    id: "BND-148",
    reason: "unwritten",
    note: "cancellation observed before a late completion is untested",
  },
  {
    id: "BND-149",
    reason: "unwritten",
    note: "an error response becoming a loading-failure event is untested",
  },
  {
    id: "BND-150",
    reason: "unwritten",
    note: "inline completion followed by a contradicting return value is untested",
  },
  {
    id: "BND-151",
    reason: "unwritten",
    note: "a stale request handle reaching a later native request is untested",
  },
  {
    id: "BND-152",
    reason: "unwritten",
    note: "a non-OK native completion staying terminal is untested",
  },
  { id: "BND-153", reason: "inapplicable", note: NO_SECOND_THREAD },
  {
    id: "BND-155",
    reason: "unwritten",
    note: "scheme aliases are configurable and no case reads one back as the requested URL",
  },
  {
    id: "BND-156",
    reason: "unwritten",
    note: "prefix routes are supported and only exact routes are exercised",
  },
  {
    id: "BND-157",
    reason: "unwritten",
    note: "route comparison against an aliased URL is untested",
  },
  {
    id: "BND-158",
    reason: "unwritten",
    note: "header transforms are unimplemented in this binding",
  },
  {
    id: "BND-159",
    reason: "unwritten",
    note: "header transforms are unimplemented in this binding",
  },

  // Render sessions. The browser suite attaches one through a real WebGL
  // context, so what remains is not the context but the API: the binding
  // exposes no CPU readback, no owned-texture frames, and no `set_target`.
  {
    id: "BND-166",
    reason: "unwritten",
    note: "the binding exposes no CPU readback",
  },
  ...([167, 168, 169, 170, 172, 173] as const).map((number) => ({
    id: `BND-${number}`,
    reason: "unwritten" as const,
    note: "the binding exposes no owned-texture frame handles",
  })),
  {
    id: "BND-171",
    reason: "unwritten",
    note: "a caller-owned texture is attachable and no case proves close leaves the caller's handles alone",
  },
  { id: "BND-174", reason: "inapplicable", note: NO_SECOND_THREAD },
  ...([175, 176] as const).map((number) => ({
    id: `BND-${number}`,
    reason: "unwritten" as const,
    note: "the binding exposes no `set_target`",
  })),

  // Threading.
  ...([190, 191, 192, 193, 194, 195, 197] as const).map((number) => ({
    id: `BND-${number}`,
    reason: "inapplicable" as const,
    note: NO_SECOND_THREAD,
  })),
];
