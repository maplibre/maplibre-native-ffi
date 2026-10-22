// The addon as the Hermes entry imports it: the embedder's bundler resolves
// this specifier to the exports of the addon that the embedder loaded.

declare module "maplibre_native_ffi.node" {
  const addon: unknown;
  export default addon;
}
