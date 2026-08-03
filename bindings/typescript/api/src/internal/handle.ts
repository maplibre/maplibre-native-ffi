/**
 * Owned handle state.
 *
 * A public wrapper owns native state across calls, so it carries the handle id,
 * its live/releasing/closed state, its parent's state when native validity
 * depends on one, and the leak-reporting token that says an unclosed handle was
 * collected.
 *
 * The id itself stays private. A public identity value carries no operations, so
 * no public code can turn one back into something that reaches native code.
 */

import { MaplibreError } from "../errors.ts";
import { EP } from "../raw/entrypoints.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

/**
 * Reports handles the host dropped without closing.
 *
 * A finalizer runs on whatever context the collector chooses and cannot know
 * whether the owner thread still exists, so it reports rather than destroys.
 * Destroying a thread-affine handle from here would be a use from the wrong
 * thread at best.
 */
const leakReports = new FinalizationRegistry<{ native: Native; token: Ptr }>(
  ({ native, token }) => {
    native.scope((scope) => {
      native.raw(scope, EP.mln_adapter_handle_leak_report, [token]);
    });
  },
);

export type HandleLifecycle = "live" | "releasing" | "closed";

export class HandleState {
  readonly native: Native;
  readonly typeName: string;
  readonly #parent: HandleState | undefined;
  #id: bigint;
  #lifecycle: HandleLifecycle = "live";
  #children = 0;
  #leakToken: Ptr = 0n;
  #owner: object | undefined;

  constructor(
    native: Native,
    typeName: string,
    id: bigint,
    parent?: HandleState,
  ) {
    this.native = native;
    this.typeName = typeName;
    this.#id = id;
    this.#parent = parent;
    parent?.retainChild();
  }

  get lifecycle(): HandleLifecycle {
    return this.#lifecycle;
  }

  get isClosed(): boolean {
    return this.#lifecycle === "closed";
  }

  /**
   * Starts reporting this handle as leaked if `owner` is collected while the
   * handle is still open.
   */
  watchForLeaks(owner: object): void {
    const token = this.native.scope((scope) => {
      const name = this.native.cString(
        scope,
        this.typeName,
        "handle type name",
      );
      return this.native.raw(scope, EP.mln_adapter_handle_leak_token_create, [
        name,
        this.#id,
      ]) as Ptr;
    });
    if (token === 0n) {
      return;
    }
    this.#leakToken = token;
    this.#owner = owner;
    leakReports.register(owner, { native: this.native, token }, this);
  }

  /** Reports the id for a call, failing before native code when it cannot. */
  use(operation: string): bigint {
    switch (this.#lifecycle) {
      case "live":
        return this.#id;
      case "releasing":
        throw new MaplibreError(
          "releaseInProgress",
          `${this.typeName} is being closed, so ${operation} cannot run`,
          { operation },
        );
      case "closed":
        throw new MaplibreError(
          "closedHandle",
          `${this.typeName} is closed, so ${operation} cannot run`,
          { operation },
        );
    }
  }

  /** Records a live child, which keeps this handle open. */
  retainChild(): void {
    this.#children += 1;
  }

  releaseChild(): void {
    this.#children -= 1;
  }

  /**
   * Runs the release operation.
   *
   * Release marks the handle so later calls fail before reaching native code,
   * calls native release once, and restores the live state when native release
   * fails, so a caller can retry. A second release succeeds without crossing
   * into C.
   */
  close(destroy: (id: bigint) => void): void {
    if (this.#lifecycle === "closed") {
      return;
    }
    if (this.#lifecycle === "releasing") {
      throw new MaplibreError(
        "releaseInProgress",
        `${this.typeName} is already being closed`,
      );
    }
    if (this.#children > 0) {
      throw new MaplibreError(
        "childrenLive",
        `${this.typeName} has ${this.#children} live child handles, ` +
          `which keep the native object valid`,
      );
    }
    this.#lifecycle = "releasing";
    try {
      destroy(this.#id);
    } catch (error) {
      // Native release failed, so the handle is still live and the caller may
      // retry. Restoring the state is what makes retrying meaningful.
      this.#lifecycle = "live";
      throw error;
    }
    this.#lifecycle = "closed";
    this.#parent?.releaseChild();
    this.#retireLeakToken();
  }

  #retireLeakToken(): void {
    if (this.#owner !== undefined) {
      leakReports.unregister(this);
      this.#owner = undefined;
    }
    if (this.#leakToken !== 0n) {
      const token = this.#leakToken;
      this.#leakToken = 0n;
      this.native.scope((scope) => {
        this.native.raw(scope, EP.mln_adapter_handle_leak_token_destroy, [
          token,
        ]);
      });
    }
  }
}
