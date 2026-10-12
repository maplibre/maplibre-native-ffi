---
name: docs-writing
description: Writing style, structure, and terminology for prose in this repository — documentation site pages, contributor docs, READMEs, header comments, and repository markdown. Use when writing or editing any prose, and when adding snippets under docs/snippets.
---

# Writing

The sentence rules apply to all prose in the repository. The page rules apply to
the documentation site under `docs/src/content/docs/`.

Integrators read the site to embed the C API from a language that they know
well; explain MapLibre concepts and assume platform knowledge. Maintainers read
`development/` pages and READMEs; assume they know the project's languages and
tools, and explain the decisions that this project made. Many readers use
English as a second language, so plainness matters more than rhythm.

## Sentences

- Use positive wording for guidance. Reserve negatives for real prohibitions and
  hard boundaries. Prefer "Examples stay small" over "Examples should not grow
  into applications."
- State what is true. Describe an absence only when the reader expects the
  thing, and name that expectation: "Unlike the MapLibre Android and iOS SDKs,
  this API has no map view that drives a frame loop."
- Put the payload in the main clause. A trailing `which` or `so` clause carries
  subordinate detail only.
- Use plain, literal verbs, not phrasal verbs or metaphors. Prefer "Queries
  belong to the render session" over "Queries hang off the render session."
- Cut a contrast when the positive statement stands alone.
- End a paragraph on the sentence that matters, not on a short fragment for
  emphasis.
- Describe an API as a thing, not a person. Prefer "A parent returns
  `MLN_STATUS_INVALID_STATE` while a child is live" over "A parent refuses."
- Keep the syntax explicit: keep `that` after a verb, relative pronouns, and
  articles.
- Give each procedural step one instruction, in under about twenty words.
- Cut hedges such as "or equivalent", and state the rule.

## Pages

- Link to another page instead of copying from it, and say each thing once.
- Each statement stands on its own, without pointing at an example or at the
  current state of the tree.
- Each page commits to one mode:

  | Mode       | Serves                            | Contains                                 |
  | ---------- | --------------------------------- | ---------------------------------------- |
  | Onboarding | A reader with nothing working yet | The operations every integration needs   |
  | Guide      | A reader who knows what they want | One task, start to finish                |
  | Concept    | A reader building a mental model  | The model and its consequences, no steps |
  | Reference  | A reader looking something up     | Tables, values, complete coverage        |

- A heading marks a section that a reader can jump to or skip. Two or three
  usually cover a page, and a linear task stays flat. Write headings in sentence
  case, and name the task rather than the API.
- A guide leads with the reader's decision when there is one, names its shapes
  and their costs, and then gives one section per shape. It finishes its task
  and stops; parameter semantics and edge cases belong in the API reference.
- Platform- or backend-specific rules go in a labeled subsection or on their own
  page.

## Prose and snippets

One set of prose serves every binding, platform, and backend. Keep prose as
general as the API: "attach a render session to your surface", not "attach an
EGL surface." Prefer conceptual phrasing such as "an invalid-state status" or
"the repaint flag" over C identifiers, and name a C function in full only when
the reader chooses between calls. Describe options as fields that a caller sets,
not as a bit mask. Binding differences live on that binding's page.

Snippet files under `docs/snippets/` compile in CI, so keep each one complete.
Mark a region in the snippet and show it by name:

```c
// #region create
mln_runtime_create(&options, &runtime);
// #endregion create
```

```mdx
import { region } from "../../../snippets";

<Code code={region(snippet, "create")} lang="c" title="first-map.c" />
```

Show four to eight meaningful lines per block. Comment only what the reader
cannot see, such as a thread rule or a trap. Never use line-numbered markers.

## Terminology

Use one term per concept, across every page.

| Use                | For                                                             | Avoid                       |
| ------------------ | --------------------------------------------------------------- | --------------------------- |
| runtime            | The native scheduler thread and event store                     | context, engine             |
| map                | Map state, independent of any render target                     | map view, map object        |
| render session     | The object that renders one map to one target                   | renderer                    |
| render target      | The surface or texture that a session draws into                | render session as a synonym |
| completion         | The one-shot callback that reports a submission's outcome       | promise, future             |
| published snapshot | A synchronous copy of state that a live handle publishes        | getter, accessor            |
| drain              | Reading queued events or results until none remain              | poll as a synonym           |
| wake               | The callback that tells a receiver work is ready                | notify, signal              |
| driver service     | Running a caller-driver session's queued graphics work          | tick, drive                 |
| graphics thread    | The thread a caller-driver session's driver calls are affine to | owner thread                |
| handle             | An opaque object that the API returns                           | pointer, object             |
| host               | The application embedding the library                           | client, user, consumer      |
| binding            | A language wrapper over the C API                               | SDK, wrapper                |

Render targets have three kinds, and the kind belongs to the target. "Surface
session" and "texture session" are not terms.

| Target                  | Owned by | Attach with                                 |
| ----------------------- | -------- | ------------------------------------------- |
| native surface          | caller   | `mln_map_attach_<backend>_surface`          |
| owned texture target    | session  | `mln_map_attach_<backend>_owned_texture`    |
| borrowed texture target | caller   | `mln_map_attach_<backend>_borrowed_texture` |
