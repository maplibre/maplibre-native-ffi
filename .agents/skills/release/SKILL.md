---
name: release
description:
  Draft the notes for a MapLibre Native FFI release, revise them with the
  maintainer, then tag the release or update a published one. Use when asked to
  prepare or tag a release, or to write or rewrite its notes.
---

# Release

The notes travel in the release's signed, annotated tag. The publish run copies
the tag's message, below its title line, into the GitHub release body and
appends the links footer.

## Find the range

- A new release covers the previous release tag to a `main` commit whose CI
  passed: `git describe --tags --abbrev=0 --match 'v[0-9]*' <commit>` finds the
  previous tag, and `mise run ci:release-version --next` prints the version.
- A published release covers its previous release tag, found by the same command
  on `<tag>^`, to its own tag.

## Learn what changed

- List the squash merges with
  `git log --first-parent --format='%h %s' <previous>..<target>`, and read a
  PR's description with `gh pr view` when its title doesn't say what a user
  sees. Don't filter by the title's type: a `ci:` or `refactor:` merge can
  change what users get.
- Judge each change by the difference between the two endpoints, with
  `git diff <previous> <target> -- <paths>`. Something added and then reworked
  or reverted inside the range appears only as its final state, or not at all.
  Where a PR's description disagrees with the diff, trust the diff.
- For a published release, check its publish run and cover only the artifacts it
  published.

## Write the draft

- Include only what a user of the library or a binding can observe: API,
  behavior, platforms, packaging, and performance. Leave out tests, CI,
  refactors, maintainer tooling, and other internal changes.
- Organize by what changed, not by PR. Merge related PRs into one item, and
  don't link PRs.
- Open with one or two sentences that state the release's theme. Below them,
  choose section headers that fit this release's changes.
- For a breaking change, say what a host must change. When a release breaks
  much, name each kind of change with one example rather than every rename.
- Keep it short and plain, one line per change where possible. Write no title
  line and no links footer.

## Review with the maintainer

Show the draft and revise it until the maintainer approves it. Don't tag, push,
or edit a release until they do.

## Publish

For a new release, write the tag message to a file: the title line
`MapLibre Native FFI <version>`, a blank line, then the notes. Create the tag,
show it, and push it only when the maintainer says to:

```sh
git tag -s --cleanup=verbatim -F <file> v<version> <commit>
git push origin v<version>
```

For a published release, keep the links footer that its body ends with, and
replace the text above it with `gh release edit <tag> --notes-file <file>`. The
tag stays as it is.
