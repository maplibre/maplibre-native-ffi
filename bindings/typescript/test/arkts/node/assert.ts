// The part of node:assert/strict that the suite calls, for the ArkTS bundle.
// ArkTS has no node: modules; the bundle resolves node:assert/strict here.

export class AssertionError extends Error {
  override name = "AssertionError";
}

type Validator =
  | RegExp
  | (abstract new (...args: never[]) => unknown)
  | ((error: unknown) => boolean);

function show(value: unknown): string {
  if (typeof value === "bigint") {
    return `${value}n`;
  }
  try {
    return (
      JSON.stringify(value, (_key, member: unknown) =>
        typeof member === "bigint" ? `${member}n` : member,
      ) ?? String(value)
    );
  } catch {
    return String(value);
  }
}

function fail(message: string | undefined, detail: string): never {
  throw new AssertionError(message ? `${message}\n${detail}` : detail);
}

function deepEqualValues(actual: unknown, expected: unknown): boolean {
  if (Object.is(actual, expected)) {
    return true;
  }
  if (
    typeof actual !== "object" ||
    typeof expected !== "object" ||
    actual === null ||
    expected === null ||
    Object.getPrototypeOf(actual) !== Object.getPrototypeOf(expected)
  ) {
    return false;
  }
  if (ArrayBuffer.isView(actual) && ArrayBuffer.isView(expected)) {
    const left = new Uint8Array(
      actual.buffer,
      actual.byteOffset,
      actual.byteLength,
    );
    const right = new Uint8Array(
      expected.buffer,
      expected.byteOffset,
      expected.byteLength,
    );
    return (
      left.length === right.length &&
      left.every((byte, index) => byte === right[index])
    );
  }
  const keys = Object.keys(actual);
  const other = Object.keys(expected);
  return (
    keys.length === other.length &&
    keys.every(
      (key) =>
        Object.prototype.hasOwnProperty.call(expected, key) &&
        deepEqualValues(
          (actual as Record<string, unknown>)[key],
          (expected as Record<string, unknown>)[key],
        ),
    )
  );
}

function matches(error: unknown, validator: Validator | undefined): boolean {
  if (validator === undefined) {
    return true;
  }
  if (validator instanceof RegExp) {
    return validator.test(String(error));
  }
  if (
    validator.prototype !== undefined &&
    (validator === Error || validator.prototype instanceof Error)
  ) {
    return error instanceof validator;
  }
  return (validator as (error: unknown) => boolean)(error) === true;
}

function ok(value: unknown, message?: string): asserts value {
  if (!value) {
    fail(message, `expected a truthy value, got ${show(value)}`);
  }
}

const assert = Object.assign(ok, {
  ok,
  equal(actual: unknown, expected: unknown, message?: string): void {
    if (!Object.is(actual, expected)) {
      fail(message, `expected ${show(expected)}, got ${show(actual)}`);
    }
  },
  notEqual(actual: unknown, expected: unknown, message?: string): void {
    if (Object.is(actual, expected)) {
      fail(message, `expected a value other than ${show(expected)}`);
    }
  },
  deepEqual(actual: unknown, expected: unknown, message?: string): void {
    if (!deepEqualValues(actual, expected)) {
      fail(message, `expected ${show(expected)}, got ${show(actual)}`);
    }
  },
  match(actual: string, expected: RegExp, message?: string): void {
    if (!expected.test(actual)) {
      fail(message, `expected ${show(actual)} to match ${String(expected)}`);
    }
  },
  throws(body: () => unknown, validator?: Validator, message?: string): void {
    try {
      body();
    } catch (error) {
      if (!matches(error, validator)) {
        fail(message, `the thrown error did not validate: ${String(error)}`);
      }
      return;
    }
    fail(message, "expected the call to throw");
  },
  async rejects(
    promise: Promise<unknown>,
    validator?: Validator,
    message?: string,
  ): Promise<void> {
    try {
      await promise;
    } catch (error) {
      if (!matches(error, validator)) {
        fail(message, `the rejection did not validate: ${String(error)}`);
      }
      return;
    }
    fail(message, "expected the Promise to reject");
  },
});

export default assert;
