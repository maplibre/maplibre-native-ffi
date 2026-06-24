# Upstream patches

## `maplibre-native-emdawnwebgpu.patch`

Minimal emdawnwebgpu support for [maplibre-native](https://github.com/maplibre/maplibre-native):
`MLN_WEBGPU_IMPL_DAWN` + `MLN_WEBGPU_EMDAWN` (4 files, ~30 lines).

Base commit when generated: `f6d70e954b07fdadf6a5adda8da49e73178298c6`.

### Apply

From a `maplibre-native` checkout:

```bash
git apply /path/to/maplibre-native-emdawnwebgpu.patch
# or, from this repo:
git apply "$MLN_FFI_REPO_ROOT/patches/maplibre-native-emdawnwebgpu.patch"
```

Configure a browser build:

```bash
cmake -DMLN_WITH_WEBGPU=ON -DMLN_WEBGPU_IMPL_DAWN=ON -DMLN_WEBGPU_EMDAWN=ON ...
# Emscripten link flags must include --use-port=emdawnwebgpu
```

Same commit on fork branch `cursor/webgpu-emdawn-minimal-bcb7` (`sargunv/maplibre-native`).
