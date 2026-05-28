# AI-assisted contributions

[MapLibre's AI Policy](https://github.com/maplibre/maplibre/blob/main/AI_POLICY.md)
is the authority for this repository. The notes below are a short supplement on
**how to contribute well** within that policy. They draw on practices from
[uv](https://github.com/astral-sh/.github/blob/main/AI_POLICY.md),
[ripgrep](https://github.com/BurntSushi/ripgrep/blob/master/AI_POLICY.md), and
[Ghostty](https://github.com/ghostty-org/ghostty/blob/main/AI_POLICY.md).

## Stay in the loop

Before you mark a pull request ready for review:

- Read every line you are asking maintainers to merge.
- Be able to explain the change, how it fits the codebase, and how you validated
  it—without leaning on the tool to answer review questions.
- Write the PR description yourself: motivation, approach, impact, and anything
  you are unsure about. Spell-checking or translation help is fine; the ideas
  and structure should be yours.

Design the change. Use AI to draft, explore, or speed up typing—not to replace
understanding the problem or the existing code.

## Talk to maintainers in your own voice

Issues, PR bodies, and review replies are a conversation between humans. Write
them yourself.

- State problems and proposals in your own words.
- When a maintainer asks a question, answer from your understanding. Do not
  paste model output as your reply.
- Trim verbosity. Say what matters for the review.

If you used AI to polish English, read the result once and adjust it so it
sounds like you. For translation, writing in your native language and adding an
English version in a quote block works well.

## When AI context belongs in a thread

Sometimes a snippet from an AI session helps reviewers (for example, a design
option you rejected). Share it in a way that keeps the thread readable:

| Amount of AI text     | What to do                                                                                                                                                                                               |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| A few lines           | Put it in a quote block (`>`), label it as AI-generated, and add a short note in your own words on why it is relevant.                                                                                   |
| More than a few lines | Put the full text in a [GitHub Gist](https://gist.github.com/) (or similar) and link it. In the comment, summarize in your own words what the gist contains and what you want reviewers to take from it. |

Do not dump long, unedited chat logs into issues or pull requests.

## Disclosure in pull requests

MapLibre asks you to disclose substantial AI-assisted work and to note tool
usage (including models and prompts) in the PR. Disclosure is not penalized.

Use a short block in the PR description when the change goes beyond single-line
completion—for example whole functions, tests, docs sections, or design
iterations you relied on heavily.

### Suggested format

Copy the template below into your PR and fill in what applies. Omit sections
that do not apply; keep prompts out of the main description when they are long
(link a gist instead).

```markdown
## AI assistance

<!-- Required when disclosure applies; see MapLibre AI Policy. -->

**Tools:** <!-- e.g. Cursor, Copilot, Claude Code, ChatGPT --> **Models:**

<!-- e.g. claude-sonnet-4-6, gpt-4.1 — omit if unknown --> **Harness:**
<!-- optional: IDE agent, cloud agent, chat, inline completion -->

**Scope:** <!-- one line: what the AI did vs what you did -->

<!-- e.g. "AI drafted the Zig binding tests; I designed the API shape, fixed two lifetime bugs, and ran mise run test." -->

**Prompts:** <!-- one of: -->

<!-- - Link: https://gist.github.com/you/... -->
<!-- - "Ad-hoc in editor; no saved prompt." -->
<!-- - "Mostly inline completion; no standalone prompt." -->
```

### Field guide

| Field       | Purpose                                                                                                                                    |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| **Tools**   | Product or service names (Ghostty-style). Helps maintainers see patterns across PRs.                                                       |
| **Models**  | Model IDs when you know them. Useful when behavior or quality differs by model.                                                            |
| **Harness** | _How_ the tool was wired: cloud agent, local agent, chat tab, tab-completion only. Optional but helpful.                                   |
| **Scope**   | Honest split of work. Focus on what you verified and own, not a percentage score.                                                          |
| **Prompts** | MapLibre asks for prompt detail when disclosure applies. Short prompts can sit inline; long ones belong in a gist linked from **Prompts**. |

### Examples

Minimal (heavy tab completion on a small fix):

```markdown
## AI assistance

**Tools:** Cursor\
**Models:** (default)\
**Harness:** inline completion\
**Scope:** Completion suggested a null check; I verified the call path and added
the test.\
**Prompts:** Mostly inline completion; no standalone prompt.
```

Typical (agent-assisted feature):

```markdown
## AI assistance

**Tools:** Cursor\
**Models:** claude-sonnet-4-6\
**Harness:** cloud agent\
**Scope:** Agent produced an initial Rust binding sketch; I reworked error
handling, aligned naming with bindings.md, and ran
`mise run //bindings/rust:ci`.\
**Prompts:** https://gist.github.com/you/abc123...
```

If nothing beyond autocomplete applies, you can skip the **AI assistance**
section entirely.
