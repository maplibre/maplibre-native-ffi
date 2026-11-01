import { definePlugin } from "@astrojs/starlight/expressive-code";
import { h, select } from "@astrojs/starlight/expressive-code/hast";

import type { Language } from "../languages";

/**
 * Adds a language picker to the header of a code block whose meta names its
 * language, as in `picker=rust`. The picker starts on that language. Picking
 * another shows that language's block instead; `Snippet.astro` handles the
 * switch.
 */
export function languagePicker(languages: Language[]) {
  return definePlugin({
    name: "language-picker",
    hooks: {
      postprocessRenderedBlock({ codeBlock, renderData }) {
        const current = codeBlock.metaOptions.getString("picker");
        if (current === undefined) return;
        const header = select("figcaption.header", renderData.blockAst);
        if (header === undefined) return;
        const logo = languages.find(({ id }) => id === current)?.logo;
        header.children.push(
          h("label.language-picker", [
            h("span.sr-only", "Language"),
            h("img.fallback-logo", { src: logo, alt: "", dataLogo: current }),
            h("select", { autocomplete: "off", dataCurrent: current }, [
              h("button", [h("selectedcontent")]),
              ...languages.map(({ id, label, logo }) =>
                h("option", { value: id, selected: id === current }, [
                  h("img", { src: logo, alt: "", dataLogo: id }),
                  h("span", label),
                ]),
              ),
            ]),
          ]),
        );
      },
    },
  });
}
