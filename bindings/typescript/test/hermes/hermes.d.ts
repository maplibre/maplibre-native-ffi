// The globals that test/hermes/host gives the bundled suite.

declare const hermesHost: {
  /** Loads a Node-API addon into a Node-API environment of its own. */
  loadAddon(path: string): unknown;
  /** Keeps a timer from holding the event loop open. */
  unrefTimer(timer: unknown): void;
  /** The exit code of the host once its event loop is empty. */
  exit(code: number): void;
  readonly env: Readonly<Record<string, string | undefined>>;
};
