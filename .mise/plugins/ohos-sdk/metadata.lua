-- An OpenHarmony SDK tool for mise, which has no registry entry for one.
--
-- The plugin lives in the repository because it is the only consumer so far. It
-- has no repository-specific knowledge, so moving it to its own repository later
-- is a matter of replacing the link the root config's preinstall hook makes with a
-- `[plugins]` entry naming its URL.
--
-- The `native` component supplies the cross-compilation SDK, and `toolchains`
-- supplies device tools such as hdc. The published SDK also carries the ArkTS
-- toolchain and previewer, which this plugin leaves packed and discards.

PLUGIN = {
  name = "ohos-sdk",
  version = "0.1.0",
  description = "OpenHarmony native SDK and device tools",
  author = "MapLibre",
}
