import type { SatteriProcessorOptions } from "@astrojs/markdown-satteri";

type HastPluginEntry = NonNullable<
  SatteriProcessorOptions["hastPlugins"]
>[number];

/** Opens links to other sites in a new tab. */
export function externalLinksPlugin(site: string): HastPluginEntry {
  return () => ({
    name: "external-links",
    element: {
      filter: ["a"],
      visit(node, ctx) {
        const href = node.properties?.href;
        if (
          typeof href === "string" &&
          /^https?:\/\//.test(href) &&
          !href.startsWith(site)
        ) {
          ctx.setProperty(node, "target", "_blank");
          ctx.setProperty(node, "rel", ["noopener", "noreferrer"]);
        }
      },
    },
  });
}
