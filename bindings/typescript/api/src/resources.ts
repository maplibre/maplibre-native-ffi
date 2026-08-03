/**
 * Resource interception.
 *
 * MapLibre asks a transform what a URL should become, and asks a provider
 * whether it will serve a request, and it asks on its own threads with an answer
 * due immediately. JavaScript arrives later than that, so the two families work
 * differently here:
 *
 * - a transform is a table of rewrite rules the native adapter evaluates, so the
 *   answer is ready when MapLibre asks for it;
 * - a provider is a table of routes plus a handler. The routes decide, at once
 *   and in native code, whether a request is claimed; a claimed request is
 *   copied and handed to the handler, which may answer whenever it can.
 */

import { NamedValue } from "./events.ts";
import {
  MLN_ADAPTER_RESOURCE_ROUTE_FLAGS,
  MLN_RESOURCE_KIND,
} from "./raw/enums.ts";

/** What MapLibre is loading. */
export class ResourceKind extends NamedValue {
  static readonly unknown = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_UNKNOWN,
    "unknown",
  );
  static readonly style = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_STYLE,
    "style",
  );
  static readonly source = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_SOURCE,
    "source",
  );
  static readonly tile = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_TILE,
    "tile",
  );
  static readonly glyphs = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_GLYPHS,
    "glyphs",
  );
  static readonly spriteImage = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_SPRITE_IMAGE,
    "spriteImage",
  );
  static readonly spriteJson = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_SPRITE_JSON,
    "spriteJson",
  );
  static readonly image = new ResourceKind(
    MLN_RESOURCE_KIND.MLN_RESOURCE_KIND_IMAGE,
    "image",
  );

  static readonly #known: readonly ResourceKind[] = [
    ResourceKind.unknown,
    ResourceKind.style,
    ResourceKind.source,
    ResourceKind.tile,
    ResourceKind.glyphs,
    ResourceKind.spriteImage,
    ResourceKind.spriteJson,
    ResourceKind.image,
  ];

  static fromRawValue(rawValue: number): ResourceKind {
    return (
      ResourceKind.#known.find((value) => value.rawValue === rawValue) ??
      new ResourceKind(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** Matches every resource kind. */
export const ANY_RESOURCE_KIND = 0xffff_ffff;

/**
 * One URL rewrite.
 *
 * The rule matches the complete requested URL. A rule with no replacement
 * leaves the URL unchanged, which is how a table exempts one URL from a broader
 * rule ahead of it.
 */
export interface ResourceRewriteRule {
  /** The resource kind this rule matches, or every kind when absent. */
  readonly kind?: ResourceKind;
  readonly url: string;
  readonly replacementUrl?: string;
}

/** Which URL a provider route compares, and how. */
export interface ResourceRoute {
  /** The resource kind this route claims, or every kind when absent. */
  readonly kind?: ResourceKind;
  /**
   * The value the route compares. Comparison is literal and case-sensitive:
   * no glob expansion, no URL parsing, and no normalization.
   */
  readonly url: string;
  /** Compares the start of the request URL instead of the whole of it. */
  readonly matchPrefix?: boolean;
  /**
   * Compares the requested URL rather than the resolved one.
   *
   * The requested URL keeps configured scheme aliases and custom schemes, and
   * is the request's cache-facing identity. The resolved URL is what a provider
   * would fetch.
   */
  readonly useRequestedUrl?: boolean;
}

/** @internal Renders a route's flags. */
export function routeFlags(route: ResourceRoute): number {
  let flags =
    MLN_ADAPTER_RESOURCE_ROUTE_FLAGS.MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE;
  if (route.matchPrefix === true) {
    flags |=
      MLN_ADAPTER_RESOURCE_ROUTE_FLAGS.MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX;
  }
  if (route.useRequestedUrl === true) {
    flags |=
      MLN_ADAPTER_RESOURCE_ROUTE_FLAGS.MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL;
  }
  return flags;
}
