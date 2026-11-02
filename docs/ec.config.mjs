// Expressive Code options. They live here because the Code component can only
// load plugins from this file.
import { defineEcConfig } from "@astrojs/starlight/expressive-code";

import { languagePicker } from "./src/integrations/language-picker.ts";
import { languages } from "./src/languages.ts";

export default defineEcConfig({
  plugins: [languagePicker(languages)],
});
