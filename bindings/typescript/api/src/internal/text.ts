/**
 * Turning library-owned bytes into strings, wherever those bytes live.
 *
 * A browser refuses `TextDecoder.decode` a view backed by a `SharedArrayBuffer`,
 * and a threaded WebAssembly module's memory is exactly that. Node decodes such
 * a view without complaint, so the difference only appears in the host the
 * payload is built for. Copying first is what both accept.
 */

const decoder = new TextDecoder();

/** Decodes UTF-8 bytes that may sit in memory shared with another agent. */
export function decodeUtf8(bytes: Uint8Array): string {
  // The copy is taken only when the source is shared, so the common case of a
  // string read out of a private buffer still decodes in place.
  return decoder.decode(
    bytes.buffer instanceof ArrayBuffer ? bytes : new Uint8Array(bytes),
  );
}
