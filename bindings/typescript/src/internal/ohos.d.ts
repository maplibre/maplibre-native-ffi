// The addon as the ArkTS entry imports it: ArkTS resolves a native module
// by the name of the library packed into the application.

declare module "libmaplibre_native_ffi.so" {
  const addon: unknown;
  export default addon;
}
