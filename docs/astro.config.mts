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
        { tag: "style", content: languageRules },
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
          exclude: [`${base}/reference/**`],
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
            {
              label: "C API",
              link: "/reference/c/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Rust API",
              link: "/reference/rust/maplibre_native_ffi/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Zig API",
              link: "/reference/zig/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Kotlin API",
              link: "/reference/kotlin/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Dart API",
              link: "/reference/dart/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Python API",
              link: "/reference/python/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: ".NET API",
              link: "/reference/dotnet/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Go API",
              link: "/reference/go/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Swift API",
              link: "/reference/swift/documentation/maplibrenativeffi/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "TypeScript API",
              link: "/reference/typescript/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
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
