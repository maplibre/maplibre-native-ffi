// @ts-check

import { satteri } from "@astrojs/markdown-satteri";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightCopyButton from "starlight-copy-button";
import starlightLinksValidator from "starlight-links-validator";

import { apiLinksPlugin } from "./src/integrations/api-links";
import { externalLinksPlugin } from "./src/integrations/external-links";
import { mermaidPlugin } from "./src/integrations/mermaid";
import { publicDirectoryIndex } from "./src/integrations/public-directory-index";
import { versionsPlugin } from "./src/integrations/versions";
import {
  defaultLanguage,
  languageStorageKey,
  languages,
} from "./src/languages";

const site = "https://maplibre.org";
const base = "/maplibre-native-ffi";

// Hides every element written for a language other than the reader's, or
// other than the default before the reader picks one.
const languageRules = [
  ...languages.map(({ id }) => [`:root[data-language="${id}"]`, id]),
  [":root:not([data-language])", defaultLanguage],
]
  .map(
    ([root, shown]) =>
      `${root} [data-for-language]:not([data-for-language="${shown}"]) { display: none; }`,
  )
  .join("\n");

// Gives each element tagged with a language its logo, which the sidebar shows.
const logoRules = languages
  .map(({ id, logo }) => `[data-logo="${id}"] { --logo: url("${logo}"); }`)
  .join("\n");

// A sidebar link to a language's API reference, which opens in a new tab.
function reference(id: string, label: string, link: string) {
  return {
    label,
    link,
    attrs: { target: "_blank", rel: "noopener noreferrer", "data-logo": id },
  };
}

// https://astro.build/config
export default defineConfig({
  site,
  base,
  markdown: {
    processor: satteri({
      mdastPlugins: [mermaidPlugin, versionsPlugin, apiLinksPlugin(base)],
      hastPlugins: [externalLinksPlugin(site + base)],
    }),
  },
  integrations: [
    publicDirectoryIndex(base),
    starlight({
      title: "MapLibre Native FFI",
      logo: {
        light: "./src/assets/maplibre-logo-square-for-light-bg.svg",
        dark: "./src/assets/maplibre-logo-square-for-dark-bg.svg",
      },
      editLink: {
        baseUrl:
          "https://github.com/maplibre/maplibre-native-ffi/edit/main/docs/",
      },
      routeMiddleware: "./src/banner.ts",
      components: {
        EditLink: "./src/components/EditLink.astro",
        MarkdownContent: "./src/components/MarkdownContent.astro",
        SocialIcons: "./src/components/SocialIcons.astro",
      },
      customCss: ["./src/styles/custom.css"],
      head: [
        { tag: "style", content: languageRules + "\n" + logoRules },
        // Restores the reader's language before the page draws.
        {
          tag: "script",
          content: `try {
  const language = localStorage.getItem(${JSON.stringify(languageStorageKey)});
  if (language) document.documentElement.dataset.language = language;
} catch {}`,
        },
      ],
      plugins: [
        starlightCopyButton(),
        starlightLinksValidator({
          // The reference and the demo are built outside the site.
          exclude: [`${base}/reference/**`, `${base}/demo/`, `${base}/demo/**`],
        }),
      ],
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/maplibre/maplibre-native-ffi",
        },
      ],
      sidebar: [
        { label: "Overview", link: "/" },
        {
          label: "Install",
          items: [{ autogenerate: { directory: "install" } }],
        },
        {
          label: "Guides",
          items: [
            "guides/create-a-map",
            "guides/choose-a-target",
            "guides/images",
            "guides/window",
            "guides/texture",
            "guides/camera",
            "guides/style",
            "guides/query",
            "guides/events-and-logs",
            "guides/network",
            "guides/offline",
          ],
        },
        {
          label: "Reference",
          items: [
            reference("c", "C API", "/reference/c/"),
            reference("zig", "Zig API", "/reference/zig/"),
            reference("kotlin", "Kotlin API", "/reference/kotlin/"),
            reference("dart", "Dart API", "/reference/dart/"),
            reference("python", "Python API", "/reference/python/"),
            reference("dotnet", ".NET API", "/reference/dotnet/"),
            reference("go", "Go API", "/reference/go/"),
            reference("gobject", "GObject API", "/reference/gobject/"),
            reference(
              "swift",
              "Swift API",
              "/reference/swift/documentation/maplibrenativeffi/",
            ),
            reference("typescript", "TypeScript API", "/reference/typescript/"),
          ],
        },
        {
          label: "Development",
          items: [{ autogenerate: { directory: "development" } }],
        },
      ],
    }),
  ],
});
