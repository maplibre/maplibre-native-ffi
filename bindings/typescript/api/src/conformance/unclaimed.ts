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

  // Cases an external review found overclaimed. Each had a tag on a case that
  // proved part of the requirement; the tag is gone and the reason says what is
  // actually missing. A partial proof recorded as a whole one is worse than no
  // proof, because nobody goes back to it.
  {
    id: "BND-048",
    reason: "unwritten",
    note: "a failed explicit release is proven; the requirement is about the leak channel a best-effort cleanup reports through, which is the finalizer path BND-044 also needs",
  },
  {
    id: "BND-060",
    reason: "unwritten",
    note: "several input-struct families are exercised; the requirement is one test per family, which needs an inventory of the families rather than examples",
  },
  {
    id: "BND-070",
    reason: "unwritten",
    note: "camera options compare and copy by value; the requirement covers every option type and each field mutated in turn",
  },
  {
    id: "BND-085",
    reason: "unwritten",
    note: "an empty database lists through start/event/take; the requirement is region observation delivering copied status and error events",
  },
  {
    id: "BND-088",
    reason: "inapplicable",
    note:
      "the requirement is a park released by a wake signalled from another thread, and " +
      NO_SECOND_THREAD,
  },
  {
    id: "BND-141",
    reason: "unwritten",
    note: "a rewrite rule is observed to apply; the requirement is that request data reaches user code as language-owned copies, and a native rule table never reaches user code to copy",
  },
  {
    id: "BND-158",
    reason: "unwritten",
    note: "headers are installed and a request arrives; the requirement adds that headers cross as copied values and that duplicate field names are rejected case-insensitively",
  },
  {
    id: "BND-159",
    reason: "unwritten",
    note: "rules install, replace, and clear while a map is live; the requirement adds that transformed headers reach matching requests and stop after a clear, which needs a way to observe the headers on a request",
  },
  {
    id: "BND-162",
    reason: "unwritten",
    note: "session-owned and caller-owned texture attach paths are proven; the surface path is not, because no host in the suite supplies a surface",
  },

  // Handle lifetime.
  {
    id: "BND-044",
    reason: "unwritten",
    note: "this was classified inapplicable and that was wrong: `internal/handle.ts` installs a FinalizationRegistry that reports a leaked handle rather than releasing it, which is the behaviour this case describes, and nothing tests it",
  },
  {
    id: "BND-046",
    reason: "inapplicable",
    note: NO_SECOND_THREAD,
  },
  {
    id: "BND-047",
    reason: "inapplicable",
    note: "a handle's native id lives in a private field of its own class, so no code outside that class can reach one to pass it to another kind's operation; forging one means defeating the language rather than the binding",
  },
  { id: "BND-049", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Values and events.

  // Maps, queries, and style.
  { id: "BND-123", reason: "inapplicable", note: NO_SECOND_THREAD },

  // Resource requests.
  {
    id: "BND-145",
    reason: "inapplicable",
    note: NO_SECOND_THREAD,
  },
  { id: "BND-153", reason: "inapplicable", note: NO_SECOND_THREAD },

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
  {
    id: "BND-192",
    reason: "inapplicable",
    note: "this case applies to a binding that ships an owner-thread execution adapter, and this one does not; the reason is the missing adapter rather than the missing second thread",
  },
  ...([190, 191, 193, 194, 195, 197] as const).map((number) => ({
    id: `BND-${number}`,
    reason: "inapplicable" as const,
    note: NO_SECOND_THREAD,
  })),
];
