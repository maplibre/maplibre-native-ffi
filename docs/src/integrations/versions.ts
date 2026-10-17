import type { SatteriProcessorOptions } from "@astrojs/markdown-satteri";
import fs from "node:fs";

type MdastPluginEntry = NonNullable<
  SatteriProcessorOptions["mdastPlugins"]
>[number];

let versions: Record<string, string> | undefined;

/**
 * Substitutes `{{name}}` in code with the latest release version that
 * `generate-versions` wrote. MDX expressions do not run inside code blocks.
 */
export const versionsPlugin: MdastPluginEntry = () => {
  versions ??= JSON.parse(
    fs.readFileSync(
      new URL("../generated/versions.json", import.meta.url),
      "utf8",
    ),
  ) as Record<string, string>;
  const known = versions;
  const substitute = (value: string) =>
    value.replace(/\{\{(\w+)\}\}/g, (placeholder, name: string) => {
      const version = known[name];
      if (version === undefined) {
        const names = Object.keys(known).join(", ");
        throw new Error(
          `unknown version ${placeholder}; known versions: ${names}`,
        );
      }
      return version;
    });
  return {
    name: "versions",
    code(node, ctx) {
      ctx.setProperty(node, "value", substitute(node.value));
    },
    inlineCode(node, ctx) {
      ctx.setProperty(node, "value", substitute(node.value));
    },
  };
};
