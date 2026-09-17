# Development workflow

ESP32 Marauder supports many hardware targets, so the full firmware matrix is
reserved for final validation and releases. It is not an iteration loop.

## Iteration

1. Work on a local branch without opening a pull request.
2. Run native unit tests and static checks locally.
3. Compile only the hardware targets requested for physical testing.
4. Clearly identify locally compiled test binaries by target and exact commit.
5. Revise locally until the requested hardware behavior is accepted.

## Pull request gate

Push the branch and open a pull request only when the change is ready for
review. Pull requests run the comparatively inexpensive native unit-test
workflow. Additional commits should be batched instead of pushed after every
small edit.

## Full-matrix gate

When a pull request is merge-ready, a maintainer applies the
`full-hardware-ci` label. That label runs **Build and Push Parallel** against the
pull request's exact current head. Any later source push invalidates that
evidence, so the label must be removed and applied again to validate the new
head before merge.

The workflow can also be dispatched manually against a final candidate branch
with `create_release` disabled. Do not use the full matrix for ordinary
iteration or to obtain one or two test binaries.

Untrusted fork code must not receive protected-build credentials. Private V8,
Mini V3, and Dual Mini C5 validation is performed only after review, from an
exact-head branch controlled by the maintainers.

For a firmware release, dispatch the same workflow against the exact release
source commit with `create_release` enabled. The workflow remains the
authoritative producer of the draft release and its public binaries.
