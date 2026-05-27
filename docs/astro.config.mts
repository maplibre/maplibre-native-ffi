// @ts-check

import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightCopyButton from "starlight-copy-button";
import starlightLinksValidator from "starlight-links-validator";
import starlightLlmsTxt from "starlight-llms-txt";

// https://astro.build/config
export default defineConfig({
  site: "https://maplibre.org",
  base: "/maplibre-native-ffi",
  integrations: [
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
          exclude: ["/maplibre-native-ffi/reference/**"],
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
        { label: "Concepts", slug: "concepts" },
        {
          label: "Usage",
          autogenerate: { directory: "guides" },
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
              label: "Java FFM API",
              link: "/reference/java-ffm/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Java JNI API",
              link: "/reference/java-jni/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Rust API",
              link: "/reference/rust/maplibre_native/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
            {
              label: "Zig API",
              link: "/reference/zig/",
              attrs: { target: "_blank", rel: "noopener noreferrer" },
            },
          ],
        },
        {
          label: "Development",
          autogenerate: { directory: "development" },
        },
      ],
    }),
  ],
});
