# AGENTS.md

## Workflow

Use `mise run test` to build and test. Use `mise run fix` to run formatters and
linters. Read the
[development overview](docs/src/content/docs/development/overview.md) for
contributor setup, workflow commands, examples, and platform/render backend
variants. Run examples only when useful, with a brief timeout because most are
GUI apps, not one-shot tests.

Feature changes need tests through the C ABI when practical.

Campsite rules apply: leave anything you touch tidier than when you found it.

## Documentation

When working on documentation, gather necessary context first, then determine
who the audience is and whether the documentation is a Tutorial, Guide,
Reference, or Explanation, according to the
[Diátaxis Framework](https://raw.githubusercontent.com/evildmp/diataxis-documentation-framework/refs/heads/main/start-here.rst).
State your audience and category determination to the user, then load and follow
the appropriate framework before making changes:

- [Reference](https://raw.githubusercontent.com/evildmp/diataxis-documentation-framework/refs/heads/main/reference.rst)
  usually covers comments attached to source code (e.g., Doxygen).
- [Guides](https://raw.githubusercontent.com/evildmp/diataxis-documentation-framework/refs/heads/main/how-to-guides.rst)
  usually covers user-facing documentation.
- [Explanation](https://raw.githubusercontent.com/evildmp/diataxis-documentation-framework/refs/heads/main/explanation.rst)
  usually covers contributor-facing or user-facing documentation.
- [Tutorials](https://raw.githubusercontent.com/evildmp/diataxis-documentation-framework/refs/heads/main/tutorials.rst)

Use positive wording for guidance. Use negative wording for real prohibitions,
safety rules, and hard boundaries.

- Prefer: "Examples stay small and focused."
- Avoid: "Examples should not grow into full applications."
- Prefer: "Higher-level adapters may add execution models above this layer."
- Avoid: "This layer should not try to manage execution models for every
  possible host."

Before finalizing documentation changes, apply the prose review strategy from
[Writing Clearly and Concisely](https://raw.githubusercontent.com/obra/the-elements-of-style/refs/heads/main/skills/writing-clearly-and-concisely/SKILL.md#Limited%20Context%20Strategy):
use active voice, positive statements, concrete language, parallel structure,
and no needless words.

## Project Docs

Read these docs before changing related code:

- [Concepts](docs/src/content/docs/concepts.md) for project scope, ownership,
  threading, events, rendering targets, and host integration boundaries.
- [C API Conventions](docs/src/content/docs/development/c-conventions.md) before
  changing public C headers, C ABI behavior, callbacks, diagnostics, or render
  target contracts.
- [Binding Conventions](docs/src/content/docs/development/bindings.md) and the
  relevant language-specific binding note in
  `docs/src/content/docs/development/bindings-*.md` before changing a language
  binding or its generated reference docs.

## External Docs

Read these docs for related tooling:

- [mise settings](https://mise.jdx.dev/configuration/settings.html) when
  changing `[settings]` entries in `mise.toml`.
- [mise task configuration](https://mise.jdx.dev/tasks/task-configuration.html)
  when changing mise task metadata.
- [mise file tasks](https://mise.jdx.dev/tasks/file-tasks.html) when changing
  `.mise/tasks/**` task files.
- [mise task arguments](https://mise.jdx.dev/tasks/task-arguments.html) when
  changing `usage` specs or task CLI arguments.
- [mise task templates](https://mise.jdx.dev/tasks/templates.html) when changing
  task template expressions.
- [mise environments](https://mise.jdx.dev/environments/) when changing `[env]`,
  environment files, environment scripts, or mise profiles.
- [mise Python](https://mise.jdx.dev/lang/python.html) when changing Python, uv,
  or virtual environment integration.
- [hk configuration](https://hk.jdx.dev/configuration.html) when changing
  `hk.pkl`.
- [dprint configuration](https://dprint.dev/config/) when changing
  `dprint.jsonc`.
