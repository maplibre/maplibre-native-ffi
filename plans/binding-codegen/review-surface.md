# Binding review surface

The audit compares `main` at `eb1a1c40a`, the executor branch before generation
at `5f6976bdf`, and the generated branch at `695ebdc9b`. Counts are physical
text lines, including comments and blank lines. They measure files that
maintainers edit, rather than executable statements.

| Area                                                    |    Main | Before generation | Generated branch |
| ------------------------------------------------------- | ------: | ----------------: | ---------------: |
| Binding files without a generated marker                | 132,797 |           125,860 |           25,077 |
| Binding compiler and emitters                           |       0 |                 0 |           20,635 |
| Native `src/` and `include/`, without generated markers |  43,752 |            48,738 |           51,105 |
| Combined maintained files above                         | 176,549 |           174,598 |           96,817 |
| Separate test files                                     |  73,801 |            73,672 |           73,251 |
| Documentation and planning files                        |   9,088 |             9,132 |            9,189 |
| Files marked generated, including lockfiles             |  75,932 |            83,145 |          220,416 |

The binding files and compiler together shrink from 125,860 to 45,712 lines
against the executor parent, a 63.7% reduction. Including the native core gives
a 44.5% reduction. The new compiler consists of 4,183 frontend, schema, and
shared compiler lines, plus 16,452 language-emitter lines.

## Counting rules

The inventory reads tracked blobs from each commit with `git ls-tree` and
`git cat-file`, and resolves that commit's attributes with
`git check-attr --source=<commit>`. It excludes submodules, vendored files, and
binary blobs. A `linguist-generated=true` attribute classifies a complete file
as generated before other path rules apply.

Documentation paths and Markdown files form the documentation category. Test
directories, test source sets, fixtures, and conventional test filenames form
the separate-test category. Remaining files under `bindings/`, `tools/bindgen/`,
and `src/` or `include/` form the three maintained categories in the table.
Examples and other tooling are outside that subtotal.

Inline tests remain in their enclosing file's category. For example, trailing
Rust test modules account for 2,790 lines in the current binding subtotal and
5,611 before generation. These figures therefore are not production-only LOC.

## Review order

Review the C contracts and shared compiler rules first, then each language's
emitter and handwritten runtime. Header mutations, lifetime regressions, and
public binding suites exercise the generated behavior. Deterministic
regeneration checks the emitted files against those rules.

Generated files account for 71.5% of added lines against the executor parent,
and 58.8% against `main`. GitHub's generated-file attributes make those outputs
separate from the code that maintainers change. The compiler and runtime remain
the primary review surface.
