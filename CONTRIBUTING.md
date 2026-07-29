# Contributing to PikiwiDB

> 中文版本请见 [CONTRIBUTING_CN.md](CONTRIBUTING_CN.md)。

Thank you for your interest in contributing to PikiwiDB (Pika). This document
describes our code contribution and review process, which is designed to align
with open source best practices (OpenSSF).

## Code of Conduct

All participants are expected to follow our
[Code of Conduct](CODE_OF_CONDUCT.md). Report unacceptable behavior to

 `wuxianrong@360.cn`.

## Reporting Security Issues

Do **not** open a public issue for security vulnerabilities. Follow the process
in [SECURITY.md](SECURITY.md) instead.

## Ways to Contribute

- Report bugs or request features via our
[issue templates](.github/ISSUE_TEMPLATE/).
- Improve documentation.
- Submit code changes (bug fixes, features, tests, tooling).

## Development Workflow

1. **Fork** the repository and create a topic branch from the appropriate base
  branch (`unstable` for new development; `3.5` / `4.0` for targeted fixes).
2. **Sign the CLA**. Contributors must sign the Contributor License Agreement
  (see `[docs/cla/](docs/cla/)`) before contributions can be merged.
3. **Make your change** following the coding style enforced by
  `.clang-format` and `.clang-tidy` (run `./format_code.sh` before committing).
4. **Add tests** for new behavior and bug fixes where practical.
5. **Open a Pull Request** against the correct base branch.

## Commit & Provenance Requirements

- Each commit must be made by an authenticated committer. We encourage
**signed commits** (`git commit -S`) to strengthen provenance.
- When importing third-party code, clearly state its origin and license in the
commit message (e.g. `[External] Import from <project> <version>`), and ensure
it is license-compatible with PikiwiDB's [LICENSE](LICENSE.md).

## Pull Request Requirements

- **PR title** must follow [Conventional Commits](https://www.conventionalcommits.org/)
and be written in English. It is validated automatically by the
`PR Title Checker` workflow. Allowed prefixes:
`feat`, `fix`, `test`, `refactor`, `chore`, `upgrade`, `style`, `docs`,
`perf`, `build`, `ci`, `revert`.
- All **CI checks must pass** before merge, including:
  - Build and integration tests (`Pika` workflow).
  - Static/security analysis (`CodeQL` workflow).
- Every PR requires **review and approval by a maintainer**. Reviews cover
correctness, security, performance, and compatibility.
- Link the related issue in the PR description so each change has a traceable
report → fix → merge history.

## Review & Merge Policy

- Reviews are performed by maintainers and committers listed in
[MAINTAINERS.md](MAINTAINERS.md).
- Reviewers evaluate changes across **security, performance, and
compatibility** dimensions.
- A change is merged once it has the required approvals and all CI gates are
green. Maintainers make the final merge and release decisions.

## Dependencies

Dependency updates are tracked automatically via
[Dependabot](.github/dependabot.yml). Please keep new dependencies minimal,
pinned, and license-compatible.

## Becoming a Committer or Maintainer

See the criteria and nomination process in [MAINTAINERS.md](MAINTAINERS.md).

## Questions

For governance or contribution questions, open an issue or contact `wuxianrong@360.cn`.