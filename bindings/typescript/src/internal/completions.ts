// Promises for asynchronous native operations.
//
// Each submission registers a pending entry whose token the generated addon
// function passes as the completion's user data, with the realm's delivery
// queue. The entry settles when the realm drains the completion, never inside
// the submitting call.

import { type CompletionHeader, native } from "./addon.ts";
import { MaplibreError } from "./errors.ts";
import { addCompletion, removeCompletion } from "./realm.ts";

/**
 * The generations that a committed command published. A failed or cancelled
 * command rejects with a {@link MaplibreError} instead.
 */
export interface CommandCompletion {
  /**
   * The map snapshot generation that a map command published, or zero for a
   * runtime command.
   */
  readonly generation: bigint;
  /**
   * The first map render update that includes the command, or zero when
   * `generation` is zero.
   */
  readonly renderUpdateGeneration: bigint;
}

type Start = (completion: number) => void;

/**
 * Submits an operation and returns its Promise. Throws when native code
 * rejects the submission, so a caller can keep its state unchanged.
 */
function submit<T>(
  start: Start,
  settle: (index: number, header: CompletionHeader) => T,
): Promise<T> {
  let resolve!: (value: T) => void;
  let reject!: (error: unknown) => void;
  const promise = new Promise<T>((fulfil, fail) => {
    resolve = fulfil;
    reject = fail;
  });
  const token = addCompletion({
    fail: reject,
    settle(index, header) {
      try {
        resolve(settle(index, header));
      } catch (error) {
        reject(error);
      }
    },
  });
  try {
    start(token);
  } catch (error) {
    removeCompletion(token);
    throw error;
  }
  return promise;
}

function check(header: CompletionHeader): void {
  if (header.status !== 0) {
    throw new MaplibreError(header.status, header.diagnostic);
  }
}

/** Like {@link run}, but throws when native code rejects the submission. */
export function submitRun(start: Start): Promise<void> {
  return submit(start, (_index, header) => check(header));
}

function rejected<T>(body: () => Promise<T>): Promise<T> {
  try {
    return body();
  } catch (error) {
    return Promise.reject(error);
  }
}

/** An operation that completes with no value. */
export function run(start: Start): Promise<void> {
  return rejected(() => submitRun(start));
}

/**
 * A map or runtime command, which resolves once it commits. A failed or
 * cancelled command rejects with the generation that it published.
 */
export function command(start: Start): Promise<CommandCompletion> {
  return rejected(() =>
    submit(start, (_index, header) => {
      if (header.status !== 0) {
        throw new MaplibreError(
          header.status,
          header.diagnostic,
          header.generation,
        );
      }
      return {
        generation: header.generation,
        renderUpdateGeneration: header.renderUpdateGeneration,
      };
    }),
  );
}

/**
 * An operation whose value the addon converts through the decoder that its
 * export recorded. A value that fails to convert rejects the Promise, and the
 * batch disposes it.
 */
export function query<T>(start: Start): Promise<T> {
  return rejected(() =>
    submit(start, (index, header) => {
      check(header);
      return native.completionValue(index) as T;
    }),
  );
}

/**
 * An operation that creates its owner at once in a pending state, such as a
 * map creation or a render-session attach, and reports it ready later. The
 * readiness keeps the owner reachable until it settles.
 */
export function attach<T>(
  start: (completion: number) => bigint,
  adopt: (handle: bigint) => T,
): [T, Promise<void>] {
  let handle = 0n;
  const ready = submitRun((completion) => {
    handle = start(completion);
  });
  let owner: T;
  try {
    owner = adopt(handle);
  } catch (error) {
    ready.catch(() => {});
    throw error;
  }
  return [owner, ready.then(() => void owner)];
}
