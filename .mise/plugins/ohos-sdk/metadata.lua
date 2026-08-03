-- An OpenHarmony SDK tool for mise, which has no registry entry for one.
--
-- The plugin lives in the repository because it is the only consumer so far. It
-- has no repository-specific knowledge, so moving it to its own repository later
-- is a matter of replacing the link the root config's preinstall hook makes with a
-- `[plugins]` entry naming its URL.
--
-- Only the `native` component is installed, which is what a C, C++, or Rust cross
-- build needs. The published SDK carries the ArkTS toolchain and previewer as
-- well, and extracting those would multiply the install size for nothing.

PLUGIN = {
  name = "ohos-sdk",
  version = "0.1.0",
  description = "OpenHarmony SDK, native component",
  author = "MapLibre",
}
