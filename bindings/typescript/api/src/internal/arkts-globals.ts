/**
 * The standard globals ArkTS does not define.
 *
 * The rest of this package encodes and decodes UTF-8 through `TextEncoder` and
 * `TextDecoder`, which every other runtime it supports provides globally. ArkTS
 * offers them only as members of a system module, under a shape that has moved
 * between API versions, so this installs conforming ones instead of teaching
 * the shared code which runtime it is on.
 *
 * Importing this module installs them; it does nothing where they already
 * exist. The conformance suite passes every string it uses through them, so
 * these are exercised by the same cases that run everywhere else rather than
 * trusted.
 */

const REPLACEMENT = 0xfffd;

class Utf8Encoder {
  readonly encoding = "utf-8";

  encode(input = ""): Uint8Array {
    const bytes: number[] = [];
    for (const character of input) {
      let point = character.codePointAt(0)!;
      if (point < 0x80) {
        bytes.push(point);
      } else if (point < 0x800) {
        bytes.push(0xc0 | (point >> 6), 0x80 | (point & 0x3f));
      } else if (point < 0x10000) {
        bytes.push(
          0xe0 | (point >> 12),
          0x80 | ((point >> 6) & 0x3f),
          0x80 | (point & 0x3f),
        );
      } else {
        bytes.push(
          0xf0 | (point >> 18),
          0x80 | ((point >> 12) & 0x3f),
          0x80 | ((point >> 6) & 0x3f),
          0x80 | (point & 0x3f),
        );
      }
      // An unpaired surrogate has no encoding, and iteration yields it alone.
      if (point >= 0xd800 && point <= 0xdfff) {
        bytes.length -= 3;
        point = REPLACEMENT;
        bytes.push(0xef, 0xbf, 0xbd);
      }
    }
    return Uint8Array.from(bytes);
  }

  encodeInto(
    source: string,
    destination: Uint8Array,
  ): { read: number; written: number } {
    const bytes = this.encode(source);
    const written = Math.min(bytes.length, destination.length);
    destination.set(bytes.subarray(0, written));
    return { read: source.length, written };
  }
}

class Utf8Decoder {
  readonly encoding = "utf-8";
  readonly fatal = false;
  readonly ignoreBOM = false;

  decode(input?: ArrayBufferView | ArrayBuffer): string {
    if (input === undefined) {
      return "";
    }
    const bytes = ArrayBuffer.isView(input)
      ? new Uint8Array(input.buffer, input.byteOffset, input.byteLength)
      : new Uint8Array(input);
    // Built one run at a time: a string grown a character at a time is
    // quadratic, and the C ABI hands back whole documents.
    const parts: string[] = [];
    let points: number[] = [];
    for (let index = 0; index < bytes.length; ) {
      const lead = bytes[index]!;
      let point: number;
      let width: number;
      if (lead < 0x80) {
        point = lead;
        width = 1;
      } else if ((lead & 0xe0) === 0xc0) {
        point = lead & 0x1f;
        width = 2;
      } else if ((lead & 0xf0) === 0xe0) {
        point = lead & 0x0f;
        width = 3;
      } else if ((lead & 0xf8) === 0xf0) {
        point = lead & 0x07;
        width = 4;
      } else {
        point = REPLACEMENT;
        width = 1;
      }
      if (width > 1) {
        if (index + width > bytes.length) {
          point = REPLACEMENT;
          width = bytes.length - index;
        } else {
          for (let offset = 1; offset < width; offset += 1) {
            const continuation = bytes[index + offset]!;
            if ((continuation & 0xc0) !== 0x80) {
              point = REPLACEMENT;
              width = offset;
              break;
            }
            point = (point << 6) | (continuation & 0x3f);
          }
        }
      }
      index += width;
      if (point > 0x10ffff) {
        point = REPLACEMENT;
      }
      if (point > 0xffff) {
        const surrogate = point - 0x10000;
        points.push(0xd800 + (surrogate >> 10), 0xdc00 + (surrogate & 0x3ff));
      } else {
        points.push(point);
      }
      if (points.length >= 4096) {
        parts.push(String.fromCharCode(...points));
        points = [];
      }
    }
    if (points.length > 0) {
      parts.push(String.fromCharCode(...points));
    }
    return parts.join("");
  }
}

const globals = globalThis as Record<string, unknown>;
if (typeof globals["TextEncoder"] !== "function") {
  globals["TextEncoder"] = Utf8Encoder;
}
if (typeof globals["TextDecoder"] !== "function") {
  globals["TextDecoder"] = Utf8Decoder;
}
