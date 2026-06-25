---
name: spec-audit-loop
description: Audit a binding, example, or subproject against a named project spec; coordinate focused reviewers, fix findings by severity and complexity, repeat until clean, and log deferred or controversial findings.
---

# Spec Audit Loop

Use this skill when the user asks to make a binding, example, or subproject
ready for review or merge against a project specification.

## Inputs

Resolve these from the user request and repository context:

- target subproject or diff scope;
- specification file or requirement source;
- log file for deferred or controversial findings.

If the user does not name a log file, use a project-local file under the target
directory, such as `REVIEW_NOTES.md`.

## Workflow

1. If goal tools are available, create a goal naming the target, spec, and done
   condition.
2. Read `AGENTS.md`, the named spec, and any related docs listed by `AGENTS.md`.
3. Determine the target boundary from the request, current branch, and diff.
4. Run an audit round with focused subagent reviewers:
   - spec compliance;
   - safety, lifetime, and ownership;
   - threading, concurrency, and execution model;
   - language idioms and modernization;
   - simplicity, API reduction, and code reduction.
5. Consolidate findings. Merge duplicates and reject findings outside the target
   or spec scope.
6. Classify each finding with:
   - `severity`: low, medium, or high impact if left unfixed;
   - `complexity`: low, medium, or high effort/risk to fix;
   - `decision`: fix, log for triage, or invalid.
7. Fix findings when any of these are true:
   - complexity is low;
   - severity is high;
   - complexity is lower than severity.
8. Log the remaining valid and rejected findings for user triage.
9. For substantial fixes, ask subagents to implement one area at a time. Review
   and correct their work before moving to the next.
10. Repeat the audit round after fixes. Continue until no fix-required findings
    remain.
11. Finish with a concise report: fixed findings, logged findings, validation,
    and residual risk.

## Finding Log

Use this template:

```markdown
# Review Findings

## Logged For Triage

- [ ] `id`: short title
  - severity: low|medium|high
  - complexity: low|medium|high
  - area: file, API, behavior, or spec section
  - rationale: why this is real but not fixed in this pass
  - suggested next step: concrete follow-up

## Invalidated

- `id`: short title
  - rationale: why this finding was rejected
```

## Rules

- Do not stop after the first audit, unless it was 100% clean. Fix, validate,
  and audit again.
