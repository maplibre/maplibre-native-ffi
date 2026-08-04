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
    id: "BND-085",
    reason: "unwritten",
    note: "the binding wraps only the region list; `mln_runtime_offline_region_create_start`, `_delete_start`, `_get_status_start` and their take-result pairs exist in C and are not exposed, so there is no region to observe",
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
    reason: "inapplicable",
    note: "the requirement is about data crossing into user code, and this binding installs the adapter's native rewrite rule table rather than a JavaScript callback, so no transform request data reaches user code to be copied; a binding that grew a callback transform would have to claim this",
  },
  {
    id: "BND-158",
    reason: "inapplicable",
    note: "the requirement is about headers crossing a callback boundary as copied values, and this binding installs the adapter's native header rule table rather than a JavaScript callback; the rules it supplies are copied at registration, which BND-159 is where that would be proven",
  },
  {
    id: "BND-159",
    reason: "unwritten",
    note: "rules install, replace, and clear while a map is live; proving headers reach matching requests needs a request to expose the headers it carries, which neither the C API nor this binding does — it needs an HTTP endpoint the suite controls",
  },
  {
    id: "BND-162",
    reason: "unwritten",
    note: "session-owned and caller-owned texture attach paths are proven; the surface path is not, because WebGL has no surface handle and no Node-API host in the suite supplies one — it needs a host with a native window",
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
