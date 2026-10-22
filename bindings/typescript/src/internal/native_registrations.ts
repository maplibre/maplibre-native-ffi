// Registrations whose function pointers, context, and release native code in
// the host supplies.
//
// A realm runs host code only on its own thread, so a registration that native
// code calls synchronously on another thread, such as a queue lock, has no
// JavaScript form. A native registration fills the C descriptor with native
// function pointers instead, which native code calls directly; a native wake
// also skips the post to the realm. The addon takes the fields of a native
// registration through takeNativeRegistration() as it converts the call that
// carries it, so the first such call consumes it. The addon releases the
// context of a call that native code does not accept.

const MAX_U64 = (1n << 64n) - 1n;

/**
 * A callback registration whose function pointers, context, and release
 * native code in the host supplies. The first call that carries it consumes
 * it.
 */
export class NativeRegistration<Kind extends string = string> {
  /** The registration type that this registration fills. */
  readonly kind: Kind;
  // The C descriptor's callbacks, then its context and its release, until a
  // call takes them.
  #fields: readonly bigint[] | undefined;

  constructor(kind: Kind, fields: readonly bigint[]) {
    this.kind = kind;
    this.#fields = fields;
  }

  /**
   * Takes the fields of a registration of `kind` once: null for another kind,
   * and undefined after the first take.
   *
   * @internal
   */
  static take(
    value: NativeRegistration,
    kind: string,
  ): readonly bigint[] | undefined | null {
    if (value.kind !== kind) {
      return null;
    }
    const fields = value.#fields;
    value.#fields = undefined;
    return fields;
  }
}

/**
 * Creates a native registration of `kind` from its fields in C order, and
 * checks that each one is a bigint that a native pointer can hold.
 */
export function nativeRegistration<Kind extends string>(
  kind: Kind,
  fields: Readonly<Record<string, bigint>>,
): NativeRegistration<Kind> {
  for (const [name, value] of Object.entries(fields)) {
    if (typeof value !== "bigint") {
      throw new TypeError(`${kind}.native: ${name}: expected a bigint`);
    }
    if (value < 0n || value > MAX_U64) {
      throw new RangeError(
        `${kind}.native: ${name}: expected a bigint from 0 to 2^64-1`,
      );
    }
  }
  return new NativeRegistration(kind, Object.values(fields));
}

/**
 * Takes the fields of a native registration for the call that the addon
 * converts. Returns undefined for a registration that an earlier call took,
 * which the addon reports as an invalid state, and throws a TypeError for a
 * value that is no native registration of `kind`.
 */
export function takeNativeRegistration(
  value: unknown,
  kind: string,
  path: string,
): readonly bigint[] | undefined {
  const fields =
    value instanceof NativeRegistration
      ? NativeRegistration.take(value, kind)
      : null;
  if (fields === null) {
    throw new TypeError(
      `${path}: expected a native ${kind} registration from ${kind}.native()`,
    );
  }
  return fields;
}
