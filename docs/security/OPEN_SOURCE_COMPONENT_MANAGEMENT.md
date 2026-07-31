# Open Source Component Security Management & Vulnerability Handling

> 中文版本请见 [OPEN_SOURCE_COMPONENT_MANAGEMENT_CN.md](OPEN_SOURCE_COMPONENT_MANAGEMENT_CN.md).

This document describes how the PikiwiDB (Pika) project performs Software
Composition Analysis (SCA) on the third-party open source components it depends
on, how discovered vulnerabilities are graded, and the concrete process,
timelines, and responsibilities for handling **high-severity and above** open
source component vulnerabilities, with real-world examples.

This policy complements the project's overall security policy. For vulnerability
reporting and response, see [../../SECURITY.md](../../SECURITY.md).

---

## 1. Scope

This policy applies to all third-party open source components used by the
PikiwiDB main repository and its accompanying tools, including but not limited
to:

- C/C++ components pulled at build time via CMake `ExternalProject_Add`;
- Components introduced as Git submodules;
- Toolchain dependencies introduced via Go modules (`gomod`);
- Third-party GitHub Actions used in CI/CD.

Current major third-party components (updated as versions evolve):

| Component | Purpose | Current version |
|-----------|---------|-----------------|
| RocksDB | Underlying persistent storage engine | v8.7.3 |
| protobuf | Serialization | v3.17.3 |
| zstd / lz4 / snappy / zlib / bz2 | Compression | v1.5.4 / v1.9.4 / 1.1.7 / v1.3.1 |
| gflags / glog | CLI flags & logging | v2.2.2 / v0.6.0 |
| fmt | Formatting | 10.2.1 |
| jemalloc / gperftools | Memory allocation | 5.3.0 |
| hiredis | Redis client | v1.2.0 |
| gtest | Unit testing | - |

> The complete, authoritative component list is defined by the **SBOM (Software
> Bill of Materials)** shipped with each release.

---

## 2. Scanning Tools & Frequency

The project uses Software Composition Analysis (SCA) tools to detect open source
components and their known vulnerabilities across the full codebase and its
third-party dependencies. The tools used include:

- **Qicai Lengjing (七彩棱镜)** — open source component & compliance scanning
- **MurphySec (墨菲)** — open source component vulnerability & license detection
- **ChainGraph (链图)** — dependency graph & supply-chain risk scanning

Scan triggers:

| Trigger | Description |
|---------|-------------|
| Before a release | A full SCA scan must be run before every official release, and the report retained |
| Periodic | A full scan of the main maintenance branch at least monthly |
| On dependency change | PRs that add or upgrade a component trigger a targeted scan |
| Intelligence-driven | When a major upstream vulnerability (e.g. a CVE) is disclosed, immediately assess the affected component |

In addition, the repository has GitHub `CodeQL` (static security analysis) and
`Dependabot` (dependency update alerts) enabled as continuous automated
supplements.

---

## 3. Severity Grading

Open source component vulnerabilities found by SCA scans are graded uniformly
per the Chinese national standard **GB/T 30279-2020** (Information Security
Technology — Guidelines for Categorization and Classification of Cybersecurity
Vulnerabilities), into four levels. When judging exploitability, we also account
for the component's **actual usage and exposure within PikiwiDB** (reachability
analysis) to avoid false positives on unreachable code paths.

| Level | Criteria | Blocks release? |
|-------|----------|-----------------|
| Critical | Remotely exploitable without authentication, with the relevant code reachable in PikiwiDB; may lead to RCE, mass data leakage, or cluster compromise | Yes — must be handled first |
| High | May lead to data leakage, privilege escalation, or denial of service; or a critical flaw with limited trigger conditions | Yes — must be handled first |
| Medium | Requires specific conditions or authentication; limited impact | No — fix within a deadline |
| Low | Minor impact, strict preconditions, or the code path is unreachable | No — may be handled in a regular release |

---

## 4. Handling Process for High-Severity and Above

When an SCA scan finds a **High or Critical** open source component
vulnerability, the following closed-loop process is executed.

### 4.1 Guiding Principle

**A release will not ship until all High-and-above vulnerabilities are
resolved (release blocker).**

### 4.2 Steps

1. **Registration & grading**
   - Record the vulnerability in the project's defect/security tracking system,
     noting component name, version, CVE id, and originating SCA tool;
   - Grade it per Section 3 and perform reachability analysis to confirm whether
     it is genuinely exploitable within PikiwiDB.

2. **Impact analysis**
   - Determine the affected component version(s), which modules reference it, and
     which released branches are affected (e.g. `4.0` / `3.5`);
   - Check whether an official fixed version or patch already exists.

3. **Choose a remediation strategy** (by priority)

   | Priority | Strategy | When it applies |
   |----------|----------|-----------------|
   | 1 | **Upgrade** the component to a fixed, secure version | Upstream has released a compatible fix |
   | 2 | **Patch / backport the fix** | Upstream fixed it but the upgrade is incompatible; backport the critical patch |
   | 3 | **Configuration mitigation / disable affected feature** | Cannot upgrade short-term; work around via config or build options |
   | 4 | **Replace the component** | The component is unmaintained with no fix |
   | 5 | **Risk acceptance (unreachable / low real risk only)** | Reachability analysis confirms it is not exploitable; must be documented and approved by a Maintainer |

4. **Fix & verification**
   - Submit the fix as a PR whose description links the vulnerability record and
     CVE;
   - Pass full CI (build, integration tests, `CodeQL`);
   - Re-run the SCA scan to confirm the vulnerability is cleared and no new
     high-severity items were introduced.

5. **Release & disclosure**
   - Decide whether to ship a patch/minor release based on severity;
   - Record in the Release Notes / CHANGELOG: affected component, CVE,
     remediation method, and the upgraded version;
   - For issues with user-facing security impact, publish an advisory following
     the disclosure process in [SECURITY.md](../../SECURITY.md).

6. **Archiving**
   - Retain SCA reports (before + after), the vulnerability record, and the fix
     PR link to form an auditable evidence chain.

### 4.3 Remediation SLA

| Level | Target remediation window (from confirmation) |
|-------|-----------------------------------------------|
| Critical | Fix or effective mitigation within 7 days |
| High | Fix or effective mitigation within 30 days |
| Medium | Handle within 90 days |
| Low | Handle in the next regular release |

### 4.4 Responsibilities

| Role | Responsibility |
|------|----------------|
| Security / maintenance lead | Organize scans, confirm grading, track SLAs, decide on release blocking |
| Component owner / module maintainer | Design and implement the remediation, submit the fix PR |
| Reviewing maintainer | Review fix correctness and compatibility, approve merge |
| PMC | Final sign-off on "risk acceptance" decisions and release go/no-go |

---

## 5. Worked Examples

The following examples, based on PikiwiDB's actual dependencies, illustrate how
the process is applied.

### Example 1: High-severity compression library flaw → upgrade

- **Discovery**: A pre-release SCA scan (MurphySec) reports a high-severity
  vulnerability in `zstd 1.5.4` (illustrative CVE-XXXX-XXXX) — an out-of-bounds
  memory access when decompressing crafted data.
- **Analysis**: PikiwiDB uses zstd for data compression via RocksDB; the code
  path is reachable, so it is graded **High** and blocks the release.
- **Handling**: The preferred strategy is upgrade — bump the zstd version pulled
  in `CMakeLists.txt` to the official fixed version; submit a PR linked to the
  record.
- **Verification**: Pass CI (build + integration tests) and re-run all three SCA
  tools to confirm the item is cleared.
- **Release & archive**: CHANGELOG records "upgrade zstd to vX.Y.Z, fixes
  CVE-XXXX-XXXX"; before/after scan reports are retained. **Completed within 30
  days of confirmation, meeting the High SLA.**

### Example 2: Critical transitive dependency flaw → backported patch

- **Discovery**: ChainGraph reports a critical parsing vulnerability in the
  underlying `protobuf v3.17.3` that can cause remote denial of service.
- **Analysis**: Upgrading to the latest major version would break compatibility;
  graded **Critical**, blocks the release.
- **Handling**: Use the "patch / backport" strategy — backport upstream's
  critical fix for this CVE into the current build without a breaking upgrade.
- **Verification & release**: After CI and an SCA re-scan pass, ship a patch
  release and publish a security advisory per the SECURITY process. **Completed
  within 7 days, meeting the Critical SLA.**

### Example 3: Unreachable vulnerability → risk acceptance (approved & recorded)

- **Discovery**: Qicai Lengjing reports a vulnerability in the test-only
  component `gtest`.
- **Analysis**: `gtest` is used only for unit tests and does not enter release
  artifacts; the code is unreachable in production deployments, so the real risk
  is negligible.
- **Handling**: After reachability analysis, apply **risk acceptance**, document
  the rationale in the vulnerability record, and obtain Maintainer approval;
  plan to address it in a later routine upgrade. **The decision is recorded and
  auditable.**

---

## 6. Related Documents

- [SECURITY.md](../../SECURITY.md) — reporting channels, response SLA, disclosure
- [CONTRIBUTING.md](../../CONTRIBUTING.md) — third-party code intake & provenance
- [MAINTAINERS.md](../../MAINTAINERS.md) — roles and responsibilities
