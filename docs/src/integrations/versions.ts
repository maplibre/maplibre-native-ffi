import fs from "node:fs";

import type { SatteriProcessorOptions } from "@astrojs/markdown-satteri";

type MdastPluginEntry = NonNullable<
  SatteriProcessorOptions["mdastPlugins"]
>[number];

const versionsPath = new URL("../generated/versions.json", import.meta.url);

let versions: Record<string, string> | undefined;

function loadVersions(): Record<string, string> | undefined {
  if (!fs.existsSync(versionsPath)) {
    return undefined;
  }
  return JSON.parse(fs.readFileSync(versionsPath, "utf8")) as Record<
    string,
    string
  >;
}

/**
 * Substitutes `{{name}}` in code with the latest release version that
 * `generate-versions` wrote. MDX expressions do not run inside code blocks.
 *
 * `astro sync` processes Markdown before that file exists, so placeholders
 * stay unchanged until the docs build writes the versions.
 */
export const versionsPlugin: MdastPluginEntry = () => {
  versions ??= loadVersions();
  const known = versions;
  const substitute = (value: string) => {
    if (known === undefined) {
      return value;
    }
    return value.replace(/\{\{(\w+)\}\}/g, (placeholder, name: string) => {
      const version = known[name];
      if (version === undefined) {
        const names = Object.keys(known).join(", ");
        throw new Error(
          `unknown version ${placeholder}; known versions: ${names}`,
        );
      }
      return version;
    });
  };
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
