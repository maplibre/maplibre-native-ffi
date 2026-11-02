import fs from "node:fs";

import type { SatteriProcessorOptions } from "@astrojs/markdown-satteri";

import { languages } from "../languages";

type MdastPluginEntry = NonNullable<
  SatteriProcessorOptions["mdastPlugins"]
>[number];

// Each C declaration's name and reference page by language.
type Index = Record<string, Record<string, { name: string; url: string }>>;

const indexPath = new URL("../generated/api-index.json", import.meta.url);

let index: Index | undefined;

function loadIndex(): Index | undefined {
  if (!fs.existsSync(indexPath)) {
    return undefined;
  }
  return JSON.parse(fs.readFileSync(indexPath, "utf8")) as Index;
}

function escape(text: string): string {
  return text
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

/**
 * Turns `[](api:mln_map_set_style_url)` into the declaration's name in the
 * reader's language, linked to its page in that language's API reference.
 * Every language renders, and the site's head styles show only the reader's.
 * A declaration that some language's reference doesn't show fails the build.
 *
 * `generate-api-index` writes the index from the built references. `astro
 * sync` processes Markdown before that, so links stay unchanged until then.
 */
export function apiLinksPlugin(base: string): MdastPluginEntry {
  return () => {
    index ??= loadIndex();
    const known = index;
    return {
      name: "api-links",
      link(node, ctx) {
        if (known === undefined || !node.url.startsWith("api:")) return;
        const of = node.url.slice("api:".length);
        if (node.children.length > 0) {
          throw new Error(`write [](api:${of}), with no link text`);
        }
        const entries = known[of];
        if (entries === undefined) {
          throw new Error(`api:${of}: no binding exposes this declaration`);
        }
        const missing = languages.filter(({ id }) => !(id in entries));
        if (missing.length > 0) {
          const labels = missing.map(({ label }) => label).join(", ");
          throw new Error(`api:${of}: no reference page in ${labels}`);
        }
        ctx.replaceNode(node, {
          type: "html",
          value: languages
            .map(({ id }) => {
              const { name, url } = entries[id]!;
              const href = escape(`${base}/reference/${url}`);
              return `<a data-for-language="${id}" href="${href}"><code>${escape(name)}</code></a>`;
            })
            .join(""),
        });
      },
    };
  };
}
