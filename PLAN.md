# Reference documentation: static HTML per language

Plan for publishing idiomatic API reference sites under the MapLibre Native FFI
docs deployment, replacing the current C API Markdown pipeline and adding Java,
Rust, and Zig references.

## Goal

After this work, the published site exposes one static HTML tree per binding,
all served from the same Astro/Starlight deployment:

| URL path                               | Generator      | Documented surface                                         |
| -------------------------------------- | -------------- | ---------------------------------------------------------- |
| `/maplibre-native-ffi/reference/c/`    | Doxygen HTML   | `include/**/*.h`                                           |
| `/maplibre-native-ffi/reference/java/` | Javadoc HTML   | Exported `org.maplibre.nativeffi.*` (not `internal.c`)     |
| `/maplibre-native-ffi/reference/rust/` | rustdoc HTML   | `maplibre-native` crate (not `sys` / `core`)               |
| `/maplibre-native-ffi/reference/zig/`  | `zig doc` HTML | Public Zig package (`maplibre_native`), not raw `@cImport` |

Starlight continues to own guides, concepts, and development docs. API reference
pages are **not** Starlight Markdown; they are copied into `docs/public/` and
deployed as static assets with the rest of `docs/dist`.

Kotlin modules are out of scope until they exist; use Javadoc for Java now and
revisit Dokka when Kotlin sources land.

## Current state

- **C:** Doxygen emits XML only (`GENERATE_HTML = NO`).
  [moxygen](https://www.npmjs.com/package/moxygen) converts XML →
  `docs/src/content/docs/reference/c.md` (~11k lines). Starlight autogenerates
  the Reference sidebar from `reference/`.
- **Dependencies:** `moxygen`, `remark-frontmatter`, `remark-gfm` (docs
  devDeps); Handlebars templates under `docs/api/c/templates/cpp/`.
- **CI:** `mise run //docs:build` runs `api:c` then `astro build`; GitHub Pages
  uploads `docs/dist`.
- **Rust / Java / Zig:** No doc generation tasks yet. Comment styles are already
  compatible with rustdoc, Javadoc, and Zig doc comments.

## Target architecture

```text
include/*.h ──► doxygen ──► docs/api/c/gen/html/ ──► docs/public/reference/c/
bindings/java-ffm ──► javadoc ──► .../reference/java/
cargo doc (maplibre-native) ──► .../reference/rust/
zig build docs ──► .../reference/zig/
                              │
                              ▼
                    astro build → docs/dist
                              │
                              ▼
              https://maplibre.org/maplibre-native-ffi/reference/{c,java,rust,zig}/
```

**Mise orchestration (docs monorepo):**

- `docs:api:c` — Doxygen HTML + install to `public/reference/c/`
- `docs:api:java` — Javadoc + install to `public/reference/java/`
- `docs:api:rust` — `cargo doc` + install to `public/reference/rust/`
- `docs:api:zig` — `zig build docs` + install to `public/reference/zig/`
- `docs:api` — depends on all four (or staged subsets during rollout)
- `docs:build` — depends on `docs:api`, then `astro build`

Generated trees stay **gitignored** or live only under `docs/public/reference/`
(produced at build time, not committed).

## Implementation order

Order is intentional: establish the static-HTML pattern with C (smallest
conceptual change from today), remove Markdown/moxygen plumbing, then add
bindings in decreasing integration risk.

### Phase 1 — C API: Doxygen HTML site

**Why first:** Same tool and `Doxyfile`; only change output format and install
path. Validates `public/reference/` + sidebar links before adding
Gradle/Cargo/Zig.

1. **Update `docs/api/c/Doxyfile`**
   - Set `GENERATE_HTML = YES`, `GENERATE_XML = NO` (XML only existed for
     moxygen).
   - Set `HTML_OUTPUT = html` (under `api/c/gen/`).
   - Tune HTML options as needed: `GENERATE_TREEVIEW`, search, stylesheet. Start
     with Doxygen defaults; add `doxygen-awesome-css` later only if we want
     visual parity with other MapLibre projects.
   - Keep C-oriented settings (`OPTIMIZE_OUTPUT_FOR_C`, `INPUT = ../include`,
     etc.).

2. **Replace `docs/mise.toml` task `api:c`**
   - Clean: `api/c/gen`, old `src/content/docs/reference/c.md`, legacy paths.
   - Run: `doxygen api/c/Doxyfile`
   - Install: copy or rsync `api/c/gen/html/` → `docs/public/reference/c/`
   - Ensure `index.html` exists at the public URL (Doxygen `index.html`).

3. **Starlight sidebar**
   - Stop autogenerating Reference from `src/content/docs/reference/` for API
     pages.
   - Add explicit sidebar links, e.g. C API → `/reference/c/`, with parallel
     entries added in later phases.
   - Optional: thin Starlight stub pages under `src/content/docs/reference/`
     that redirect or link prominently to the static trees (only if we want
     breadcrumbs inside Starlight for API landing).

4. **Verify locally**
   - `mise run //docs:build`
   - Open `/maplibre-native-ffi/reference/c/` (dev and preview).
   - Confirm internal links and asset paths work with Astro
     `base: "/maplibre-native-ffi"`. Doxygen may need `HTML_EXTRA_STYLESHEET` /
     relative path checks; fix if CSS or JS 404s under the subpath.

5. **CI**
   - Existing `docs` job should pass with no workflow change if `//docs:build`
     still works.

### Phase 2 — Remove moxygen and Markdown reference plumbing

**Why second:** Avoid maintaining two C pipelines during binding rollout.

1. **Delete or stop using**
   - `docs/api/c/templates/cpp/` (moxygen Handlebars templates)
   - `moxygen` from `docs/package.json` and `pnpm-lock.yaml`
   - `remark-frontmatter` / `remark-gfm` if only used for moxygen output linting
   - `docs/.mdxlintrc.json` references to templates if removed

2. **Update `.gitignore`**
   - Remove `docs/src/content/docs/reference/c.md`
   - Ignore `docs/public/reference/**` (all generated API HTML)
   - Keep or adjust `docs/api/c/gen/` ignore

3. **Update `dprint.jsonc`**
   - Remove `docs/api/c/templates/**` exclude if directory deleted

4. **Docs copy and links**
   - `docs/src/content/docs/index.mdx` — keep link to `/reference/c/`
   - `README.md`, `development/overview.md` — describe generated HTML under
     `docs/public/reference/`, not Markdown export
   - `AGENTS.md` Diátaxis note: C reference is Doxygen HTML attached to headers,
     not Starlight Markdown

5. **`starlight-llms-txt`**
   - Confirm `exclude: ["reference/**"]` still makes sense (hand-written
     reference stubs only, or remove category if empty)

6. **Run `mise run //docs:fix`** (pnpm install / lockfile refresh)

### Phase 3 — Rust: rustdoc HTML

1. **Add `bindings/rust` or root mise task** (or `docs:api:rust` calling cargo):
   ```bash
   cargo doc -p maplibre-native --no-deps --document-private-items=false
   ```
   - Document only the public crate; do not publish `maplibre-native-sys` or
     `maplibre-native-core` unless we explicitly want contributor docs.
   - Output: `target/doc/maplibre_native/` (crate name underscore) — confirm
     path when implementing.

2. **Install to `docs/public/reference/rust/`**
   - Copy rustdoc output; fix up `index.html` entry point.

3. **Sidebar** — link “Rust API” → `/reference/rust/`

4. **CI consideration:** `docs:api:rust` may require a successful native build
   only if rustdoc needs it (usually not for doc-only). Keep task independent of
   full variant matrix if possible.

### Phase 4 — Java (FFM): Javadoc HTML

1. **Gradle `javadoc` task** in `bindings/java-ffm/build.gradle.kts`
   - Source: `src/main/java`, exclude `org/maplibre.nativeffi.internal.**`
   - Module path / release 25 aligned with compile task
   - Output: `build/docs/javadoc/` (default)

2. **`docs:api:java` mise task**
   - Depends on Java compile (or `//bindings/java-ffm:build`)
   - Run Gradle javadoc
   - Copy to `docs/public/reference/java/`

3. **Sidebar** — link “Java API” → `/reference/java/`

4. **Later (Kotlin):** When Kotlin modules arrive, evaluate Dokka HTML for
   Kotlin + shared Java in one Gradle project; do not block this phase on Dokka.

### Phase 5 — Zig: emitted documentation HTML

1. **`bindings/zig/build.zig`**
   - Add `docs` step using `getEmittedDocs()` on the library module and
     `addInstallDirectory` (see Zig guide: Generating Documentation).
   - `zig build docs` → `zig-out/docs/` (confirm layout on 0.16)

2. **`docs:api:zig` mise task**
   - Run zig docs with same `-Dcmake-artifact-dir` / include flags as build
   - Copy to `docs/public/reference/zig/`

3. **Sidebar** — link “Zig API” → `/reference/zig/`

4. **Scope:** Document `maplibre_native` module surface; keep `c.zig` /
   `@cImport` internal.

### Phase 6 — Polish and consistency

1. **Unified `docs:api` task** — all four generators; `docs:build` depends on
   it.

2. **Reference landing (optional)**
   - Starlight page `reference/index.md` listing C / Java / Rust / Zig with one
     line each (hand-written, not autogenerated from MD API files).

3. **Link checker**
   - `starlight-links-validator` may not crawl static `public/` trees; manually
     verify external links in guides point to `/reference/{lang}/`.

4. **Base URL / trailing slashes**
   - Align trailing-slash behavior across Doxygen, rustdoc, Javadoc, Zig doc.

5. **Contributor docs**
   - `development/overview.md`: commands to regenerate API reference locally.

## Files likely touched (by phase)

| Phase | Paths                                                                                                                                   |
| ----- | --------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | `docs/api/c/Doxyfile`, `docs/mise.toml`, `docs/astro.config.mts`, `.gitignore`                                                          |
| 2     | `docs/package.json`, `docs/pnpm-lock.yaml`, `docs/api/c/templates/**`, `dprint.jsonc`, prose in `docs/src/content/docs/**`, `README.md` |
| 3     | `docs/mise.toml`, `bindings/rust/mise.toml` or root `mise.toml`, `Cargo.toml` doc metadata if needed                                    |
| 4     | `bindings/java-ffm/build.gradle.kts`, `docs/mise.toml`                                                                                  |
| 5     | `bindings/zig/build.zig`, `bindings/zig/mise.toml`, `docs/mise.toml`                                                                    |
| 6     | `docs/src/content/docs/reference/index.md` (optional), `development/overview.md`                                                        |

## Verification checklist

- [ ] `mise run //docs:build` produces `docs/dist/reference/{c,java,rust,zig}/`
- [ ] Each URL loads CSS/JS (no broken relative paths under site base)
- [ ] CI `docs` job and `Docs Pages` workflow succeed on the feature branch
- [ ] No committed generated API HTML or `reference/c.md`
- [ ] Sidebar links work from Overview and Development pages
- [ ] `starlight-llms-txt` output remains sensible (API HTML excluded)

## Out of scope (this plan)

- Kotlin / Dokka integration
- Publishing `maplibre-native-sys`, `internal.c`, or raw C import layers
- Custom cross-language search across four HTML generators
- Replacing hand-written guides with generated content
- docs.rs or external hosting of Rust docs

## Risks and mitigations

| Risk                                                   | Mitigation                                                                                                                                                                                                  |
| ------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Doxygen assets break under `/maplibre-native-ffi` base | Test subpath early; set Doxygen `HTML_OUTPUT` install layout; use Astro `public/` (no base prefix on static files — URLs are `/maplibre-native-ffi/reference/c/...` which maps to `public/reference/c/...`) |
| Large `docs:build` time                                | Cache API gen in CI per job; parallel mise tasks where independent                                                                                                                                          |
| Javadoc needs compiled classes                         | Wire `docs:api:java` after `compileJava`                                                                                                                                                                    |
| Zig doc step API drift on Zig upgrades                 | Pin behavior to 0.16; document in `bindings-zig.md`                                                                                                                                                         |

## Suggested PR breakdown

One feature branch can land in stacked commits or follow-up PRs:

1. **PR 1:** Phase 1 + 2 (C HTML + moxygen removal) — unblocks production C
   reference
2. **PR 2:** Phase 3 (rustdoc)
3. **PR 3:** Phase 4 (javadoc)
4. **PR 4:** Phase 5 + 6 (zig doc + polish)

Alternatively a single PR if each phase stays green in CI.
