---
name: docs-writing
description: Writing style, structure, and terminology for prose in this repository — documentation site pages, contributor docs, specifications, header comments, and repository markdown. Use when writing or editing any prose, and when adding snippets under docs/snippets.
---

# Writing

These rules cover prose everywhere in the repository. The sentence-level rules
apply to all of it. The page structure rules apply to everything under
`docs/src/content/docs/`, and specifications add two rules on top.

## Readers

The site serves two audiences.

Integrators embed a C API from the language they already work in. Most know
their own platform well and know MapLibre barely. Explain MapLibre concepts, and
assume platform knowledge.

Contributors work on this repository, and read the pages under
`docs/src/content/docs/development/`. Assume they know the languages and tools
in the project map, and explain decisions this project made.

Many readers in both groups read English as a second or third language, so
plainness matters more than rhythm.

## Sentences

### Use positive wording for guidance

Reserve negative wording for real prohibitions, safety rules, and hard
boundaries.

- Avoid: "Examples should not grow into full applications."
- Prefer: "Examples stay small and focused."
- Avoid: "This layer should not try to manage execution models for every
  possible host."
- Prefer: "Higher-level adapters may add execution models above this layer."

### State what is true

Describe an absence only when the reader arrives with a specific expectation,
and name that expectation in the same sentence. Everything a library does not do
is otherwise an infinite set.

- Avoid: "MapLibre gives you no flush and no final event, so read the state you
  mirror from events while the map is still live."
- Prefer: "Read the state you mirror from events while the map is live.
  Destroying a map discards that map's queued events immediately."
- Prefer: "Unlike the MapLibre Android and iOS SDKs, this API has no map view
  that drives a frame loop."

### Put the payload in the main clause

A trailing `which` or `so` clause carries subordinate detail only.

- Avoid: "A render session takes the thread that attached it, which need not be
  the map's."
- Prefer: "A render session takes the thread that attached it. That thread can
  differ from the map's owner thread."

### Use plain verbs

Replace phrasal verbs and metaphors with the literal verb. A term of art defined
once, such as **pump**, is fine; decoration is not.

- Avoid: "Queries hang off the render session." "State rides along with the
  renderer." "Light steers fill-extrusion shading." "Reach for the setter."
- Prefer: "Queries belong to the render session." "State belongs to the
  renderer." "Light controls fill-extrusion shading." "Use the setter."

### Cut the contrast when the positive statement stands alone

- Avoid: "Budget for it as work, not as a fixed per-frame slice."
- Prefer: "One call can span an entire style parse. Budget for it as variable
  work."

### End a paragraph on the sentence that matters

Lead with the fact rather than saving a short sentence for emphasis. A closing
fragment reads as significance, so it draws attention by position instead of by
importance.

- Avoid: "Payload and message pointers stay valid only until the next poll. Copy
  what you keep."
- Prefer: "Copy any value you keep, because payload and message pointers stay
  valid only until the next poll."

### Describe an API as a thing rather than as a person

- Avoid: "A parent refuses to close while a child is live."
- Prefer: "A parent returns `MLN_STATUS_INVALID_STATE` while a child is live."

### Keep the syntax explicit

Keep `that` after a verb, keep relative pronouns, and keep articles.

- Avoid: "The runtime drains the work the owner thread queued."
- Prefer: "The runtime drains the work that the owner thread queued."

### Give each step one instruction

Keep procedural sentences under about twenty words.

## Say it once, and say it plainly

Link to another page instead of copying from it. A copy drifts from its source
and doubles the edit.

Each statement stands on its own, without pointing at an example or at the
current state of the tree.

Cut hedges. "Or equivalent" and vague outcomes leave the reader to guess what
the rule is.

Scope by constraint: general sections state general behavior, and platform- or
API-specific rules belong in clearly labeled subsections.

## One mode per page

Each page commits to one of four modes, after [Diátaxis](https://diataxis.fr/).
Serving two modes on one page is the most common structural failure. This holds
for contributor pages as much as for integrator pages.

| Mode       | Serves                            | Contains                                 |
| ---------- | --------------------------------- | ---------------------------------------- |
| Onboarding | A reader with nothing working yet | The operations every integration needs   |
| Guide      | A reader who knows what they want | One task, start to finish                |
| Concept    | A reader building a mental model  | The model and its consequences, no steps |
| Reference  | A reader looking something up     | Tables, values, complete coverage        |

Explanation inside an onboarding page slows the reader who wants a working
result. Steps inside a concept page make it useless for lookup. Move the
material rather than blending it.

Onboarding shows the operations that every integration needs, so that a reader
reaches the reference with the shape of the API already in mind. It is not a
tutorial that builds one particular application.

Reader-facing navigation labels stay natural: "Get started", "Guides",
"Concepts", "Reference". The mode names above are for authors.

A specification is reference, with the two extra rules below.

## A guide covers a task, not an API surface

Finish the task and stop. Leave the rest of the domain alone.

A guide that names every function in an area has become reference. Parameter
semantics, presence and absence of optional fields, and edge-case values belong
in the API reference.

The test is whether a reader can finish the task, not whether the page mentions
everything.

## Prose is neutral, and snippets are concrete

One set of prose serves every language binding, every platform, and every render
backend. A snippet has to pick one of each to compile, and that is where the
specifics belong.

Keep prose as general as the API is. Say "attach a render session to your
surface", not "attach an EGL surface", when the sentence holds for Metal and
Vulkan too. A page whose prose names one backend throughout has narrowed itself
for no reason.

State the snippet's choices once, near the snippet, so a reader knows what they
are looking at and what to substitute.

- Prefer conceptual phrasing where it reads as clearly: "set the style URL",
  "pump the runtime".
- Name a C function when the name earns its place, and write it in full:
  `mln_map_set_style_url`. Readers map it to their own binding's spelling
  without difficulty.
- Divergence between bindings lives on that binding's own page. Mention it in
  shared prose only when a reader following the prose would otherwise write
  broken code, and then as a one-line pointer rather than an explanation.
- Behavior that genuinely differs per platform or backend belongs in a clearly
  labeled subsection or its own page, rather than spread through prose that
  otherwise holds everywhere.
- Installation and packaging differ per binding by nature. Keep them in pages of
  their own, and keep deep packaging detail in reference where churn stays
  quarantined.

The C API reference is canonical while binding docstrings are written by hand.
Updating a binding reference is welcome; covering all of them is not mandatory.

## Snippets

Snippet files under `docs/snippets/` compile in CI, so keep each file complete
and runnable. A page shows only the part it discusses, extracted by name.

Mark a region in the snippet, and show it with `region()`:

```c
// #region create
mln_runtime_create(&options, &runtime);
// #endregion create
```

```mdx
import { region } from "../../../snippets";

<Code code={region(snippet, "create")} lang="c" title="first-map.c" />
```

Region names beat line numbers, because reformatting a snippet cannot silently
point a page at the wrong code. A name that no longer exists fails the build.

Show four to eight meaningful lines per block. Several small blocks with prose
between them read better than one block that covers a whole file.

Comments inside a region carry the guidance that belongs next to the code. Keep
them for what a reader cannot see: a non-obvious return value, a thread rule, a
trap. Skip comments that narrate the next line, and skip anything the prose
already says.

Line markers stay out of it. Expressive Code labels, `mark={"A":15-19}`, hold
one to three characters and draw on top of longer text, and every line-numbered
range breaks the next time a formatter moves a line.

## Specifications

Specifications follow everything above, and add two rules.

Write each requirement so that a reader can check it on its own.

Use MUST, SHOULD, and MAY with their
[RFC 2119](https://www.rfc-editor.org/rfc/rfc2119) meanings, and name the party
that each one binds.

## Terminology

One term per concept, across every page.

| Use            | For                                                | Avoid                         |
| -------------- | -------------------------------------------------- | ----------------------------- |
| runtime        | The scheduler and event store for one owner thread | context, engine               |
| map            | Map state, independent of any render target        | map view, map object          |
| render session | The object that renders one map to one target      | renderer                      |
| render target  | The surface or texture that a session draws into   | render session as a synonym   |
| pump           | Calling the runtime's pump function                | drive, tick, service          |
| drain          | Reading queued events until none remain            | pump, poll as a synonym       |
| owner thread   | The thread that a handle is affine to              | owning thread, calling thread |
| handle         | An opaque object that the API returns              | pointer, object               |
| host           | The application embedding the library              | client, user, consumer        |
| binding        | A language wrapper over the C API                  | SDK, wrapper                  |

### Render targets

A render session has one kind. Render targets have three, and the kind belongs
to the target:

| Target                  | Owned by | Attach with                             |
| ----------------------- | -------- | --------------------------------------- |
| native surface          | caller   | `mln_<backend>_surface_attach`          |
| owned texture target    | session  | `mln_<backend>_owned_texture_attach`    |
| borrowed texture target | caller   | `mln_<backend>_borrowed_texture_attach` |

"Surface session" and "texture session" are not terms. The core triple is
runtime, map, render session, and the render target is what a session draws
into.

## Before you finish

- Sentence-level rules hold throughout.
- Nothing is restated that a link would cover.
- The page serves one mode.
- A guide finishes its task and skips the rest of the domain.
- Prose reads correctly for a reader on any binding.
- Terminology matches the tables above.
