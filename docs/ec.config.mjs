// @ts-check

import { defineEcConfig } from "@astrojs/starlight/expressive-code";
import { pluginCollapsibleSections } from "@expressive-code/plugin-collapsible-sections";

// Guides embed whole compiled snippet files and collapse the boilerplate, so a
// reader sees the lines under discussion while CI still checks the whole file.
// Plugins live here rather than in astro.config.mts because the <Code>
// component needs a separately loadable config.
export default defineEcConfig({
  plugins: [pluginCollapsibleSections()],
});
