import type { SatteriProcessorOptions } from "@astrojs/markdown-satteri";

type MdastPluginEntry = NonNullable<
  SatteriProcessorOptions["mdastPlugins"]
>[number];

function escape(text: string): string {
  return text
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}

/**
 * Turns ```mermaid code blocks into diagrams. The block becomes a placeholder
 * that holds its source, and `src/mermaid.ts` draws it in the browser. Give the
 * diagram an `accTitle` for screen readers.
 */
export const mermaidPlugin: MdastPluginEntry = () => ({
  name: "mermaid",
  code(node, ctx) {
    if (node.lang !== "mermaid") return;
    ctx.replaceNode(node, {
      type: "html",
      value: `<figure class="mermaid-diagram not-content"><pre class="mermaid-source" hidden>${escape(node.value)}</pre></figure>`,
    });
  },
});
