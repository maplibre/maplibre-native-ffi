# ArkUI example

An OpenHarmony app with one ArkUI page that shows a continuous map with the
TypeScript binding's OHPM packages, `@maplibre/native-ffi` and
`@maplibre/native-ffi-opengl`. It renders with OpenGL ES into an `XComponent`
surface.

## Run it

From the repository root, build the native library, the binding's HARs, and the
app, then install and start it on this worktree's emulator:

```bash
mise -E ohos run //examples/arkui:run
```

The emulator is x64, so this task builds for `ohos-x64-egl`. To install on
another device, pass its hdc connect key with `--device <key>`. To build for
another preset, run `mise -E ohos run //examples/arkui:build <preset>`.

## How it fits together

The app has no native code of its own. It installs the HARs that
`//bindings/typescript:pack-ohpm` packs, as `ohpm install` would.

`entry/src/main/ets/pages/Index.ets` shows the `XComponent` and reports its
surface. `map/MapController.ets` attaches a render session to that surface,
gives the session each new surface, and paces its frames to the display, as the
browser example in `examples/browser` and the React Native example in
`examples/react-native` do. The session renders on a library thread, in a
graphics context that the library makes for the window. `map/gestures.ets` turns
ArkUI's pan, pinch, and rotation gestures into camera changes, and passes their
speed at release so that the map keeps moving.

DevEco Studio builds apps with hvigor. Its packages are on the OpenHarmony npm
registry, but their license is unknown: their `package.json` names a
`LICENSE.txt` that they don't include. So `scripts/build-hap.sh` calls the
OpenHarmony SDK's tools instead, and packs and signs the HAP with
`tools/ohos/pack-hap`, as the TypeScript binding's ArkTS suite does. ets-loader
compiles the pages with its webpack configuration. `scripts/webpack.config.cjs`
gives that configuration the app's API floor, so ArkTS linter errors fail the
build as they do in DevEco Studio. In SDK 7.0, the configuration has these
limits:

- It doesn't rewrite `@kit.*` imports, so the example imports the `@ohos.*`
  modules instead.
- Its `tsconfig.esm.json` lacks `NodeContainer`, `ContentSlot`, `SymbolGlyph`,
  `FolderStack`, and `MovingPhotoView`.
- Its `components/common_attrs.json` lacks `layoutGravity`.
- `$r()` needs the resource table that restool makes, and the build doesn't run
  restool.
