// @ts-check

import { publicDirectoryIndex } from "./src/integrations/public-directory-index";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightCopyButton from "starlight-copy-button";
import starlightLinksValidator from "starlight-links-validator";
import starlightLlmsTxt from "starlight-llms-txt";

const base = "/maplibre-native-ffi";

// https://astro.build/config
export default defineConfig({
  site: "https://maplibre.org",
  base,
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
      customCss: ["./src/styles/custom.css"],
      plugins: [
        starlightCopyButton(),
        starlightLlmsTxt({ exclude: ["reference/**"] }),
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
        { label: "Install", slug: "install" },
        { label: "Concepts", slug: "concepts" },
        {
          label: "Guides",
          items: [{ autogenerate: { directory: "guides" } }],
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
