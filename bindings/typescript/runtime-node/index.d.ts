/** Metadata describing what this payload was built for. */
export interface RuntimeMetadata {
  readonly transport: "node-api";
  readonly target: string;
  readonly backend: string;
  readonly abiFingerprint: string;
  readonly addon: string;
}

export declare const runtime: RuntimeMetadata;

/**
 * The compiled Node-API addon.
 *
 * Its shape is the facade's `NodeApiAddon`, which is declared there because the
 * facade owns every public type.
 */
export declare const addon: unknown;
