# Security Policy

> 中文版本请见 [SECURITY_CN.md](SECURITY_CN.md)。

PikiwiDB (Pika) takes the security of our software and users seriously. This
document describes how to report vulnerabilities, our response commitments, how
we grade severity, and how fixes are disclosed.

---

## Supported Versions

We provide security fixes for the following release lines. Older versions may
not receive security updates.

| Version | Supported          |
|---------|--------------------|
| 4.0.x   | :white_check_mark: |
| 3.5.x   | :white_check_mark: |
| < 3.5   | :x:                |

---

## Reporting a Vulnerability

**Please do NOT report security vulnerabilities through public GitHub issues.**

Instead, report them through either of the following private channels:

1. **Email**: `g-infra-bada@360.cn`
2. **GitHub Security Advisories**: use the
   ["Report a vulnerability"](https://github.com/OpenAtomFoundation/pikiwidb/security/advisories/new)
   button under the repository's **Security** tab.

To help us triage quickly, please include as much of the following as possible:

- A description of the vulnerability and its potential impact
- Steps to reproduce or a proof-of-concept
- Affected version(s), commit hash, and deployment environment
- Any suggested mitigation, if known

---

## Response SLA

We commit to the following timeline after receiving a report:

| Stage                   | Commitment                                         |
|-------------------------|----------------------------------------------------|
| Initial acknowledgement | Within **72 hours**                                |
| Severity assessment     | Within 7 business days                             |
| Fix or mitigation plan  | Communicated based on severity (see below)         |
| Public disclosure       | After a fix is released, coordinated with reporter |

We will keep the reporter informed of progress throughout the process.

---

## Severity Grading

We assess and grade reported vulnerabilities in accordance with the Chinese
national standard **GB/T 30279-2020** (Information Security Technology —
Guidelines for Categorization and Classification of Cybersecurity
Vulnerabilities).

Severity is divided into four levels. Our target remediation windows are:

| Level    | Description                                                                               | Target fix window    |
|----------|-------------------------------------------------------------------------------------------|----------------------|
| Critical | Remotely exploitable without authentication; full system compromise or mass data leakage  | 7 days               |
| High     | May lead to data leakage, privilege escalation, or denial of service                      | 30 days              |
| Medium   | Requires specific conditions or authentication; limited impact                            | 90 days              |
| Low      | Minor impact with strict exploitation preconditions                                       | Next regular release |

---

## Disclosure Process

1. Reporter submits the vulnerability through a private channel above.
2. We acknowledge receipt within 72 hours and begin triage.
3. We grade the severity per GB/T 30279-2020 and develop a fix.
4. We prepare a fixed release and, where applicable, a GitHub Security Advisory
   / CVE.
5. After the fix is released, we publicly disclose the issue and credit the
   reporter (unless anonymity is requested).

We follow a **coordinated disclosure** model and ask reporters not to publicly
disclose the issue until a fix is available.

---

## Bug Bounty

At this time, PikiwiDB does **not** offer a monetary bug bounty program. We
deeply appreciate every responsible disclosure and will publicly acknowledge
reporters in our release notes and/or a security acknowledgements list.

---

## Vulnerability & Bug Management

Beyond security-specific reports, general bugs are tracked and resolved through
our public process:

- **Report channel**: structured issue templates under
  [`.github/ISSUE_TEMPLATE/`](.github/ISSUE_TEMPLATE/) (bug report, feature
  request, docs bug).
- **Triage**: maintainers label and prioritize issues; regressions and crashes
  are treated with higher priority.
- **Fix & traceability**: fixes are submitted as PRs that reference the
  originating issue number, so every reported bug has an auditable
  report → fix → merge trail (see the `fix:` commits in the project history).
- **CI gates**: every PR must pass build, tests, and CodeQL security scanning
  before merge.
