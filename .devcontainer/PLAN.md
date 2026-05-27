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
| Registry                  | Public GHCR, e.g. `ghcr.io/<org>/maplibre-native-ffi/devcontainer`.                                                             |
| Tags                      | **Single moving tag per arch** (see [Tags](#tags)); no PR/sha tags in phase 1.                                                  |
| Rebuild triggers          | All mise config (`.miserc.toml`, `mise.toml`, `mise.lock`, `.mise/**`), devcontainer files, Docker/base Dockerfiles used by CI. |
| `MISE_LOCKED` in image CI | Not required; match runtime CI (`mise-action` + lockfile in repo).                                                              |
| `postCreate`              | `mise trust -y && mise install` (full `postinstall` hooks, same as today).                                                      |
| CI rendering env          | Match Linux CI software GL/Vulkan env vars in `devcontainer.json` `containerEnv`.                                               |
| Host mise                 | Not required for devcontainer path; local non-container workflow unchanged.                                                     |
| Staleness                 | `mise install` on create/lockfile change (same mental model as any dev env).                                                    |
| Editor                    | `customizations.vscode` from `.vscode/extensions.json` + settings from `.vscode/settings.json` where applicable.                |
| `remoteUser`              | Non-root if base image provides a standard dev user (see [User ID](#user-id)).                                                  |
| Verification              | Owner validates locally with Docker (not cloud agent VM).                                                                       |

---

## Architecture

```text
┌─────────────────────────────────────────────────────────────────┐
│  CI (ubuntu-latest + ubuntu-24.04-arm), on config/lock changes   │
├─────────────────────────────────────────────────────────────────┤
│  1. Build thin BASE image (apt: Mesa/EGL packages)               │
│  2. mise oci build --from <base>  (tools → /mise layers)         │
│  3. mise oci push → ghcr.io/.../devcontainer:<tag>               │
│     (one job per arch, or buildx — see TODOs)                    │
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

**Recommended phase 1 pipeline:** small `Dockerfile.base` → build/tag base →
`mise oci build --from <base-ref>` → `mise oci push`. Apt stays **below** mise
tool layers. No need for a fat final Dockerfile unless tooling requires it (e.g.
adding `devcontainer.json` metadata labels).

**Base image choice (resolved):** `debian:bookworm-slim` + apt, **or**
`mcr.microsoft.com/devcontainers/base:ubuntu` + apt — latter gives a `vscode`
user and devcontainer conventions; former is smaller. Prefer **devcontainers
base + apt** for non-root UX unless image size is prohibitive.

Optional `[oci]` in `mise.toml` (phase 1 can use CLI flags instead):

```toml
[oci]
from = "ghcr.io/<org>/maplibre-native-ffi/devcontainer-base:main"
# workdir = "/workspaces/maplibre-native-ffi"  # only if we standardize path
```

---

## Tags

**Decision:** one moving tag per architecture, tied to `main`:

| Arch  | Example tag                                                 |
| ----- | ----------------------------------------------------------- |
| amd64 | `ghcr.io/<org>/maplibre-native-ffi/devcontainer:main`       |
| arm64 | `ghcr.io/<org>/maplibre-native-ffi/devcontainer:main-arm64` |

Alternative: OCI index / single `main` multi-arch manifest (one tag, two
platforms) — nicer UX, slightly more CI wiring.

**Not in phase 1:** `:latest` alias, `:sha`, PR tags.

<!-- TODO: Confirm org/package name matches GitHub org (maplibre? maplibre-native-ffi?). -->

---

## `devcontainer.json` (target shape)

Start from `mise generate devcontainer`, then edit:

| Generator output                                         | Phase 1 action                                                                                                      |
| -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `"image": "mcr.microsoft.com/devcontainers/base:ubuntu"` | Replace with `ghcr.io/.../devcontainer:main` (or arch-specific tag / devcontainer `features` for arch — see TODOs). |
| `"features": { "ghcr.io/.../mise:1": {} }`               | **Remove** — mise + tools already in image.                                                                         |
| `"extensions": ["hverlin.mise-vscode"]`                  | Replace with full `.vscode/extensions.json` recommendations.                                                        |
| `"mounts": []`                                           | Keep empty unless `-m` volume desired later.                                                                        |
| `"containerEnv": {}`                                     | Add CI software GL/Vulkan vars (below).                                                                             |
| (missing)                                                | Add `postCreateCommand`, `workspaceFolder`, `remoteUser`.                                                           |

### `containerEnv` (match CI Linux)

From `.github/actions/setup-ci-deps` and pixi Linux activation:

```json
"containerEnv": {
  "EGL_PLATFORM": "surfaceless",
  "LIBGL_ALWAYS_SOFTWARE": "1"
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

### 2. Base image preference

**Resolved:** `mcr.microsoft.com/devcontainers/base:ubuntu` + Mesa/EGL apt
packages is the pragmatic default (non-root `vscode` user, familiar devcontainer
stack). `debian:bookworm-slim` remains a smaller fallback.

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

**Source:** `mise` `src/oci/builder.rs` resolves `cfg.env()` at **image build
time** and writes values into OCI image config (`docker inspect` visible).
Templates like `{{config_root}}` use the **CI checkout path** (e.g.
`/home/runner/work/...`), not the developer’s `/workspaces/...`.

**Runtime behavior:**

- Devcontainer **`containerEnv` overrides** same-named image env vars
  ([devcontainer spec](https://github.com/devcontainers/spec/blob/main/docs/specs/devcontainer-reference.md)
  — per-variable last wins).
- **`mise run` / `mise install`** load project config from the **mounted**
  workspace; activation should apply correct `config_root` for task execution.

**Phase 1 recommendation:**

- **Default:** rely on mise activation + workspace mount (no extra env in image
  config files to edit).
- **If spike shows wrong paths** in plain shells or non-mise tools: add minimal
  overrides:

  ```json
  "containerEnv": {
    "MLN_FFI_REPO_ROOT": "${containerWorkspaceFolder}"
  }
  ```

  `MLN_FFI_BUILD_DIR` is derived from variant config under `.mise/` and should
  follow once `config_root` is correct and `MISE_ENV` / `.miserc` apply.

**Spike (local):** after first image push,
`docker run -it <image> env | grep
MLN_FFI` then open devcontainer and compare
`mise exec -- env | grep MLN_FFI` from `workspaceFolder`.

### 5. `MISE_CONFIG_DIR=/etc/mise` in image

Oci build always sets `MISE_DATA_DIR=/mise` and `MISE_CONFIG_DIR=/etc/mise`
(last, cannot be shadowed). Project `mise.toml` on the mount remains the source
of truth for tasks/tools policy at runtime; synthesized config in the image
points tools at `/mise`.

### 6. `remoteUser` / `vscode`

Microsoft dev container images define user `vscode` (uid 1000). **`vscode` is
idiomatic**, not project-specific. Use `"remoteUser": "vscode"` when the base or
`--from` image includes that user.

If base is plain `debian:bookworm-slim` only, either add a `USER vscode` in
`Dockerfile.base` or run as root in phase 1 — prefer devcontainers base.

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

### 10. Apt packages (from CI)

Packages installed on Linux CI runner (not in pixi):

- `libegl-mesa0`
- `libgl1-mesa-dri`
- `mesa-utils` (optional for diagnostics; can omit in base to save size)

---

## CI workflow (sketch)

New workflow, e.g. `.github/workflows/devcontainer-image.yml`:

- **Trigger:** `push` to `main` + paths filter (mise configs, locks,
  `.devcontainer/**`).
- **Permissions:** `contents: read`, `packages: write`.
- **Jobs:** `build-amd64`, `build-arm64` (matrix or separate).
- **Steps (each job):**
  1. checkout
  2. `jdx/mise-action` (install mise on runner — bootstrap only)
  3. `docker build` → `devcontainer-base:<arch>` with Mesa/EGL
  4. `mise oci build --from devcontainer-base:<arch>` (with `MISE_ENV` unset;
     Linux `.miserc` selects variant for **required env resolution** during
     build)
  5. `mise oci push ghcr.io/<org>/maplibre-native-ffi/devcontainer:<tag>`

Login: `docker/login-action` for `ghcr.io` with `GITHUB_TOKEN`.

---

## Repository files (implementation checklist)

- [ ] `.devcontainer/PLAN.md` (this file)
- [ ] `.devcontainer/Dockerfile.base` — Mesa/EGL on devcontainers Ubuntu
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

Truly unresolved items only:

1. **GHCR package path** — exact `ghcr.io/<owner>/<name>` once org/repo naming
   is confirmed for publish.

2. **Multi-arch tagging** — separate tags (`main` / `main-arm64`) vs single
   multi-arch `main` manifest; devcontainer must pull the right arch (Docker
   usually handles manifest lists; verify Cursor/Dev Containers behavior on
   Apple Silicon).

3. **Baked path spike** — local `docker inspect` + in-container `mise exec`
   check (see
   [Research §4](#4-baked-env-paths-mln_ffi_repo_root-mln_ffi_build_dir-)). Only
   add `containerEnv` overrides if broken.

4. **Base image size vs devcontainers base** — if `devcontainer/base:ubuntu` +
   tools pushes past acceptable pull time, fall back to `debian:bookworm-slim` +
   create non-root user in `Dockerfile.base`.

5. **Whether `mesa-utils` belongs in base** — convenience vs image size.

---

## Reference links

- [mise oci](https://mise.jdx.dev/dev-tools/mise-oci.html)
- [mise generate devcontainer](https://mise.jdx.dev/cli/generate/devcontainer.html)
- [mise Docker cookbook](https://mise.jdx.dev/mise-cookbook/docker.html)
- [Dev Container spec](https://devcontainers.github.io/implementors/spec/)
- [devcontainer.json reference](https://devcontainers.github.io/implementors/json_reference/)
