# Dev container — Phase 1 plan

Internal implementation plan for a prebuilt Linux dev container (GHCR + Dev
Containers). Captures decisions from design discussion (May 2026). Phase 2+
(submodule/vendor caching, warm native builds, GUI-in-Docker) is out of scope.

## Goals (phase 1)

- Contributors can open the repo in a Dev Container **without host `mise`**.
- **Mise-managed tools** are preinstalled in a public GHCR image (not
  re-downloaded on every create).
- **Repo deps** (submodule, pixi, uv, docs pnpm, hk) still run via normal
  `mise install` / `postinstall` on container create — slow first open is
  acceptable.
- **Two Linux architectures**: `linux/amd64` and `linux/arm64` (macOS ARM host
  devs).
- Smoke success (local verification): container create →
  `mise run //examples/zig-readback:run` and/or `mise run test`.

## Non-goals (phase 1)

- Baking `third_party/maplibre-native` or pixi env into the image.
- macOS / Windows container variants.
- Contributor-facing docs (ship behavior first).
- Codespaces-specific tuning beyond sharing `devcontainer.json`.
- Port forwarding (optional later for docs site).

---

## Locked decisions

| Topic                     | Decision                                                                                                                        |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| Scope                     | Phase 1 only (see non-goals).                                                                                                   |
| Image contents            | `mise oci build` for tools + Mesa/EGL **system** packages; pixi/submodule/hooks on `postCreate`.                                |
| Experimental              | Already enabled in root `mise.toml` (`settings.experimental = true`).                                                           |
| Variant                   | Default via `.miserc.toml` inside Linux container (`linux-*-vulkan`); `-E` overrides unchanged. No separate EGL image.          |
| Registry                  | Public GHCR: `ghcr.io/maplibre/maplibre-native-ffi/devcontainer` (see [GHCR](#ghcr)).                                           |
| Tags                      | Single moving tag **`main`**, **multi-arch** manifest (`linux/amd64` + `linux/arm64`). No `:sha` / PR tags in phase 1.          |
| Base OS image             | `debian:bookworm-slim` + apt (Mesa/EGL + `mesa-utils`) — **not** `devcontainers/base` (see [Base image](#base-image)).          |
| `remoteUser`              | Non-root `dev` (uid 1000) created in `Dockerfile.base`; not the Microsoft `vscode` user.                                        |
| Rebuild triggers          | All mise config (`.miserc.toml`, `mise.toml`, `mise.lock`, `.mise/**`), devcontainer files, Docker/base Dockerfiles used by CI. |
| `MISE_LOCKED` in image CI | Not required; match runtime CI (`mise-action` + lockfile in repo).                                                              |
| `postCreate`              | `mise trust -y && mise install` (full `postinstall` hooks, same as today).                                                      |
| CI rendering env          | Match Linux CI software GL/Vulkan env vars in `devcontainer.json` `containerEnv`.                                               |
| Host mise                 | Not required for devcontainer path; local non-container workflow unchanged.                                                     |
| Staleness                 | `mise install` on create/lockfile change (same mental model as any dev env).                                                    |
| Editor                    | `customizations.vscode` from `.vscode/extensions.json` + settings from `.vscode/settings.json` where applicable.                |
| Verification              | Owner validates locally with Docker (not cloud agent VM).                                                                       |

---

## Architecture

```text
┌─────────────────────────────────────────────────────────────────┐
│  CI (ubuntu-latest + ubuntu-24.04-arm), on config/lock changes   │
├─────────────────────────────────────────────────────────────────┤
│  1. Build thin BASE image (apt: Mesa/EGL packages)               │
│  2. mise oci build --from <base>  (tools → /mise layers)         │
│  3. mise oci push per arch → merge manifest → :main              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼ pull
┌─────────────────────────────────────────────────────────────────┐
│  Dev Container (local / Cursor cloud / Codespaces)               │
├─────────────────────────────────────────────────────────────────┤
│  devcontainer.json → image: ghcr.io/.../devcontainer:<tag>       │
│  bind-mount workspace → /workspaces/<repo-basename>              │
│  postCreate: mise trust -y && mise install  (hooks, slow)        │
│  day-to-day: mise run …                                          │
└─────────────────────────────────────────────────────────────────┘
```

### Why base image + `mise oci build`, not “oci only”

Mise OCI layer order (from
[mise oci docs](https://mise.jdx.dev/dev-tools/mise-oci.html)):

1. **Base image layers** (unchanged from `--from`)
2. Mise binary
3. One layer per tool
4. Synthesized `/etc/mise/config.toml` + image config env

`mise oci build` does **not** run `apt`. Mesa/EGL packages used on Linux CI
(`libegl-mesa0`, `libgl1-mesa-dri`, etc. in `.github/actions/setup-ci-deps`)
must live in the **base** image passed to `--from`.

**Recommended phase 1 pipeline:** `Dockerfile.base` → build/tag base →
`mise oci build --from <base-ref>` → `mise oci push` per arch → publish one
**multi-arch** `:main` manifest. Apt stays **below** mise tool layers.

### Base image

**Use `debian:bookworm-slim`**, not `mcr.microsoft.com/devcontainers/base:*`.

Mise oci already supplies the **toolchain** (zig, rust, java, node, pixi binary,
etc.). The base image only needs:

- glibc Linux (mise oci default; matches prebuilt tool binaries)
- **apt packages** pixi does not ship (Mesa/EGL drivers — same class as CI)
- minimal OS deps: `ca-certificates`, `git`, `curl`, `sudo`, build-essential (or
  subset as we discover during spike)
- a **non-root user** (`dev`, uid 1000) for Dev Containers

Microsoft’s devcontainer base bundles Node, common stacks, and the `vscode` user
— redundant with mise and larger to pull. We do not need the devcontainers
**feature** for mise when tools are prebaked.

Optional `[oci]` in `mise.toml` (phase 1 can use CLI flags instead):

```toml
[oci]
from = "ghcr.io/maplibre/maplibre-native-ffi/devcontainer-base:main"
```

---

## GHCR

Per
[GitHub Container registry docs](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry):

- Image reference: `ghcr.io/NAMESPACE/IMAGE_NAME:TAG`
- **Namespace** = org or user → **`maplibre`**
- **Image name** = package name → **`maplibre-native-ffi/devcontainer`**
  (repo-scoped naming; slash is allowed in GHCR package paths)

**Phase 1 publish target:**

```text
ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main
```

- **Public** package (anonymous `docker pull`).
- CI publishes with `GITHUB_TOKEN` (`packages: write`); workflow in this repo
  auto-links package to repo when using `GITHUB_TOKEN`.
- Add OCI label in base Dockerfile:
  `org.opencontainers.image.source=https://github.com/maplibre/maplibre-native-ffi`

**Not in phase 1:** `:latest` alias, `:sha`, PR tags.

---

## Tags and multi-arch

**Decision:** one tag **`main`** pointing at a **multi-arch manifest**
(`linux/amd64`

- `linux/arm64`).

Docker / Dev Containers / Cursor pull the manifest entry matching the host
(arm64 Mac → arm64 layer; amd64 Linux → amd64). No documented incompatibility
with manifest lists; issues only arise when forcing the wrong platform
(`runArgs: --platform=linux/amd64` on Apple Silicon → emulation).

**CI sketch:**

1. Job on `ubuntu-latest`: build base → `mise oci build` → push
   `ghcr.io/.../devcontainer:main-amd64` (or digest-only intermediate).
2. Job on `ubuntu-24.04-arm`: same → push `:main-arm64`.
3. `docker buildx imagetools create -t ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main \
   ghcr.io/.../devcontainer:main-amd64 ghcr.io/.../devcontainer:main-arm64`

(Exact push tags are implementation detail; final consumer-facing tag is
`:main`.)

`devcontainer.json`:

```json
"image": "ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main"
```

No `runArgs` platform override unless we intentionally want emulation.

---

## `devcontainer.json` (target shape)

Start from `mise generate devcontainer`, then edit:

| Generator output                                         | Phase 1 action                                                         |
| -------------------------------------------------------- | ---------------------------------------------------------------------- |
| `"image": "mcr.microsoft.com/devcontainers/base:ubuntu"` | Replace with `ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main`. |
| `"features": { "ghcr.io/.../mise:1": {} }`               | **Remove** — mise + tools already in image.                            |
| `"extensions": ["hverlin.mise-vscode"]`                  | Replace with full `.vscode/extensions.json` recommendations.           |
| `"mounts": []`                                           | Keep empty unless `-m` volume desired later.                           |
| `"containerEnv": {}`                                     | Add CI software GL/Vulkan vars (below).                                |
| (missing)                                                | Add `postCreateCommand`, `workspaceFolder`, `"remoteUser": "dev"`.     |

### `containerEnv` (match CI Linux)

From `.github/actions/setup-ci-deps` and pixi Linux activation:

```json
"containerEnv": {
  "EGL_PLATFORM": "surfaceless",
  "LIBGL_ALWAYS_SOFTWARE": "true"
}
```

Pixi still supplies LVP Vulkan ICD via `VK_ADD_DRIVER_FILES` after
`pixi install`. No `sudo apt` in `postCreate` if base image already includes
Mesa/EGL packages.

### `postCreateCommand`

```bash
mise trust -y && mise install
```

Runs full `postinstall` in `mise.toml` (submodule, hk, pixi, uv, docs pnpm).

### `workspaceFolder`

Use devcontainer variable (works across clone names):

```json
"workspaceFolder": "/workspaces/${localWorkspaceFolderBasename}"
```

Mise `config_root` should match the mounted repo root when commands run from
that folder.

### VS Code customizations

Copy recommendations from `.vscode/extensions.json`:

- `dprint.dprint`, `hverlin.mise-vscode`, `okerew.shader-with-metal`,
  `ziglang.vscode-zig`, `llvm-vs-code-extensions.vscode-clangd`,
  `bierner.markdown-mermaid`, `astro-build.astro-vscode`,
  `unifiedjs.vscode-mdx`, `tombi-toml.tombi`, `vscjava.vscode-gradle`,
  `redhat.java`, `ms-python.python`, `charliermarsh.ruff`

Merge relevant `.vscode/settings.json` under `customizations.vscode.settings`
(`editor.formatOnSave`, `dprint` as default formatter, python interpreter path,
etc.).

---

## Research notes (formerly open questions)

### 1. OCI + apt layering

**Resolved:** apt belongs in the **`--from` base image**, not in
`mise oci
build`. CI builds base first, then `mise oci build --from <base>`.

### 2. Base image — why not `devcontainers/base`?

**Resolved:** Mise oci provides tools; base is only OS + apt Mesa/EGL +
`mesa-utils` + git/certs + non-root `dev` user. Use **`debian:bookworm-slim`**.

### 3. `mise generate devcontainer` — what to strip

Source:
[`src/cli/generate/devcontainer.rs`](https://github.com/jdx/mise/blob/main/src/cli/generate/devcontainer.rs)

Generator always emits:

- `features["ghcr.io/devcontainers-extra/features/mise:1"]` → **remove** when
  using prebuilt GHCR image.
- Only `hverlin.mise-vscode` in extensions → **replace** with repo
  `.vscode/extensions.json`.
- Optional `-m` adds `mise-data-volume` mount + `MISE_DATA_DIR` — **not used**
  in phase 1 (tools baked in image at `/mise`).

It does **not** emit `postCreateCommand`, `workspaceFolder`, or `remoteUser` —
add manually.

### 4. Baked `[env]` paths (`MLN_FFI_REPO_ROOT`, `MLN_FFI_BUILD_DIR`, …)

**Oci build:** `src/oci/builder.rs` resolves `cfg.env()` at **image build time**
and writes into OCI image config (CI checkout paths in `docker inspect`).

**Runtime (resolved for normal workflow):**

- **`mise run` / `mise exec` / `mise install`** re-resolve `[env]` from the
  **mounted** project config; `config_root` is computed from the config file
  location on disk (`config_root.rs`), not from stale Docker `ENV` (see
  [mise environments — config_root](https://mise.jdx.dev/environments/#config_root)).
- Variant fields (`MLN_FFI_BUILD_DIR`, etc.) come from `.mise/config.*.toml`
  merged with `.miserc` defaults when mise runs in the workspace.

**Implication for phase 1:** No `containerEnv` overrides for `MLN_FFI_*` unless
the local spike shows breakage. Stale baked values may still appear in a plain
`env` before `cd` into the repo / before activation — acceptable; all documented
workflows use `mise run`.

**Optional spike (local):** compare `docker inspect` env vs
`mise exec -- env | grep MLN_FFI` from `workspaceFolder` — expect mismatch in
the former, correct paths in the latter.

### 5. `MISE_CONFIG_DIR=/etc/mise` in image

Oci build always sets `MISE_DATA_DIR=/mise` and `MISE_CONFIG_DIR=/etc/mise`
(last, cannot be shadowed). Project `mise.toml` on the mount remains the source
of truth for tasks/tools policy at runtime; synthesized config in the image
points tools at `/mise`.

### 6. `remoteUser`

**Resolved:** `Dockerfile.base` creates user **`dev`** (uid 1000, login shell).
`devcontainer.json`: `"remoteUser": "dev"`. The Microsoft `vscode` username is
only idiomatic when using their base image; we are not.

### 7. Oci build on CI

- Run on **Linux** runners matching target arch (`ubuntu-latest` /
  `ubuntu-24.04-arm`).
- Do **not** build oci images on macOS for Linux containers (host-native binary
  mismatch — mise warns).
- `mise oci build` packages **project** `mise.toml` tools only (not
  `~/.config/mise` unless `--include-global`).
- All backends in root `mise.toml` are supported (core, aqua, github, …); no
  asdf/vfox.

### 8. `mise install` vs lockfile drift

Image pins tools at build time. After pulling a newer commit with lockfile
changes, **`mise install` in postCreate** (or manual) aligns installs — same as
host dev when CI image is stale.

### 9. Codespaces vs local

Same `devcontainer.json` works; priority **local Dev Containers > Cursor cloud >
Codespaces**. No separate config in phase 1.

### 10. Apt packages (base image)

Include in `Dockerfile.base` (same class as Linux CI host packages for EGL):

- `libegl-mesa0`
- `libgl1-mesa-dri`
- `mesa-utils` (yes — `eglinfo` / diagnostics; small cost, useful when debugging
  software GL in the container)
- plus minimal OS deps: `ca-certificates`, `git`, `curl`, `sudo`,
  `build-essential` (trim after spike if redundant with mise tools)

---

## CI workflow (sketch)

New workflow, e.g. `.github/workflows/devcontainer-image.yml`:

- **Trigger:** `push` to `main` + paths filter (mise configs, locks,
  `.devcontainer/**`).
- **Permissions:** `contents: read`, `packages: write`.
- **Jobs:** `build-amd64` (`ubuntu-latest`), `build-arm64` (`ubuntu-24.04-arm`),
  then `manifest` (merge to `:main`).
- **Per-arch steps:**
  1. checkout
  2. `jdx/mise-action` (bootstrap mise on runner only)
  3. `docker build` → local `devcontainer-base` with Mesa/EGL + `dev` user
  4. `mise oci build --from devcontainer-base` (Linux `.miserc` → variant env)
  5. `mise oci push ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main-<arch>`
- **Manifest job:**
  `docker buildx imagetools create -t
  ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main ...`

Login: `docker/login-action` for `ghcr.io` with `GITHUB_TOKEN`.

---

## Repository files (implementation checklist)

- [ ] `.devcontainer/PLAN.md` (this file)
- [ ] `.devcontainer/Dockerfile.base` — Mesa/EGL + `dev` user on Debian
      bookworm-slim
- [ ] `.devcontainer/devcontainer.json` — image ref, postCreate, containerEnv,
      customizations
- [ ] `.github/workflows/devcontainer-image.yml` — build + push
- [ ] Optional `[oci]` section in `mise.toml` once base ref is stable

---

## Verification (local)

1. Pull `ghcr.io/.../devcontainer:main` (correct arch).
2. Reopen in Container with repo mount.
3. Wait for `postCreate` (submodule + pixi — expect long first run).
4. `mise run //examples/zig-readback:run`
5. `mise run test` (optional, longer).

---

## Remaining TODOs

1. **Local spike (you)** — first image push + Dev Container open; confirm
   `mise run //examples/zig-readback:run` after postCreate. Optional:
   sanity-check `mise exec -- env | grep MLN_FFI` vs `docker inspect` baked env.

2. **CI implementation detail** — exact intermediate arch tags / `imagetools`
   commands when writing the workflow (no product decision left).

---

## Reference links

- [mise oci](https://mise.jdx.dev/dev-tools/mise-oci.html)
- [mise generate devcontainer](https://mise.jdx.dev/cli/generate/devcontainer.html)
- [mise Docker cookbook](https://mise.jdx.dev/mise-cookbook/docker.html)
- [Dev Container spec](https://devcontainers.github.io/implementors/spec/)
- [devcontainer.json reference](https://devcontainers.github.io/implementors/json_reference/)
