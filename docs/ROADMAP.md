# Digit Development Roadmap

**Organization:** STN-LABZ  
**Status:** APPROVED — PM 20261009:0123 UTC  
**Last confirmed active Interface:** 1.4.1 — Qualification GREEN / Hotload ACTIVE (2026-10-09 01:58:21 UTC)  
**Latest tested Interface:** 1.4.5 — 17/17 milestone checks, 23/23 historical suites; runtime activation unconfirmed  
**Prepared:** 2026-10-08

> This roadmap is an approved development plan. Approval does not independently authorize architectural changes, supersede controlled documentation, or establish module qualification.

## Mission

Develop Digit into a deterministic, modular, platform-independent autonomous agent capable of performing assigned work within explicitly delegated authority, with human oversight, traceable decisions, and controlled access to organizational resources.

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

## Confirmed baseline and evidence

**Last explicitly confirmed Interface activation:** 1.4.1, with `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE` at **2026-10-09 01:58:21 UTC**.

**Latest operator-tested candidate:** 1.4.5, with **17/17** dedicated milestone assertions and **23/23** historical regression suites passing. The build previously failed due to an introduced source-formatting error; that error was corrected, and the operator's subsequent successful test run confirms the build/test stage. Runtime output for this attempt ends after `CORE READY`, `CORE RUNTIME_ACTIVE`, and `MODULE UPDATE_CANDIDATE` at **2026-10-09 02:17:55 UTC**. It does **not** confirm `MODULE QUALIFICATION_GREEN` or `MODULE HOTLOAD_ACTIVE` for 1.4.5.

### Interface milestone evidence

| Version | Dedicated assertions | Historical regression suites | Operator-reported runtime evidence (UTC, 2026-10-09) |
| --- | --- | --- | --- |
| 1.3.3 | Prior audit regression 10/10; dedicated count not established | Not recorded | HOTLOAD_ACTIVE 00:55:16 |
| 1.3.4 | 12/12 | 11/11 | HOTLOAD_ACTIVE 01:04:33 |
| 1.3.5 | 7/7 | 12/12 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:13:57 |
| 1.3.6 | 15/15 | 13/13 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:23:34 |
| 1.3.7 | 15/15 | 14/14 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:29:22 |
| 1.3.8 | 14/14 | 15/15 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:34:28 |
| 1.3.9 | 19/19 | 16/16 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:40:02 |
| 1.3.10 | 18/18 | 17/17 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:43:53 |
| 1.4.0 | 17/17 | 18/18 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:48:31 |
| 1.4.1 | 17/17 | 19/19 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:58:21 |
| 1.4.2 | 17/17 | 20/20 | QUALIFICATION_GREEN / REGISTRY_AUDIT_FULL 02:01:41; HOTLOAD_ACTIVE not reported |
| 1.4.3 | 18/18 | 21/21 | QUALIFICATION_GREEN / REGISTRY_AUDIT_FULL 02:08:26; HOTLOAD_ACTIVE not reported |
| 1.4.4 | 21/21 | 22/22 | QUALIFICATION_GREEN / REGISTRY_AUDIT_FULL 02:11:49; HOTLOAD_ACTIVE not reported |
| 1.4.5 | 17/17 | 23/23 | CORE READY / RUNTIME_ACTIVE 02:17:47; UPDATE_CANDIDATE 02:17:55; later hotload state not reported |

These results are **operator-reported**. In particular, test pass, Core runtime readiness, Interface qualification, and Interface activation are distinct evidence gates. `REGISTRY_AUDIT_FULL` was observed for 1.4.2 through 1.4.4, but its cause and impact on hotload are not established. Do not infer the same event for 1.4.5 solely from prior occurrences. Earlier Interface 1.3.2 evidence: 41/41 project-provisioning assertions, GREEN / ACTIVE 00:18:44 UTC.

Implemented Interface capabilities include:

- Local HTTP interface on `127.0.0.1:8081`; authenticated remote access is not yet enabled.
- Local account and session authentication.
- Protected, scoped channel access-control records.
- Qualified Security Administrator (SA) verification.
- Private project provisioning, project identity validation, and security membership.
- Core-backed restricted security-channel binding and ownership checks.
- Negative validation for malformed records, permissions, hard links, and unauthorized access.

These observations establish a baseline, not a claim that every project-wide test suite has been independently reviewed in this roadmap.

## Development phases

### Phase 1 — Interface foundation

**Status:** Implemented and operator-qualified for the reported foundation milestones; later knowledge work has additional runtime gates.

Scope: HTTP interface, session authentication, protected ACL records, project security provisioning, SA authorization, Core channel binding, and negative security validation.

### Phase 2 — Complete Interface security

**Status:** Completed through 1.3.10 — operator-reported qualification GREEN / hotload ACTIVE.

Milestones 1.3.3–1.3.10 cover protected membership and metadata consistency, directory ownership, channel/project authorization scope, SA and membership revocation, HTTP framing and resource limits, security audit event formatting, and final regression stabilization. Qualification applies to the reported test coverage; it does not establish untested behavior.

### Phase 3 — Controlled communications

**Planned series:** 1.4.x. **Partially implemented.** Version 1.4.0 validates structured channel messages and provides Core persistence receipts. A persistence receipt does not prove end-to-end delivery or that a recipient read a message. No automatic guest-to-employee authorization or unrestricted employee direct messages. Remaining delivery-tracking and audit guarantees require separate evidence.

### Phase 4 — Corpus and knowledge integration

**Planned series:** 1.5.x for broader integration. **Interface groundwork started during 1.4.1–1.4.5**, without skipping the approved sequential revision scheme. Existing `corpus.search` and `corpus.get` services back authenticated evidence-only retrieval. Evidence records retain source identifiers and provenance; the Interface returns explicit `UNKNOWN` on no match. These milestones do not yet establish per-record Corpus authorization, conversational answer synthesis, or end-to-end RAG qualification. Language content belongs in Corpus/RAG rather than hardcoded response banks.

### Phase 5 — Autonomous mission operations

**Proposed versions:** 1.6.x and beyond.

Introduce bounded mission tasks, delegated execution, routine anomaly handling within authority, structured status reporting, escalation, recovery, and audit trails.

## Interface milestones and disposition

| Version | Engineering objective | Disposition |
| --- | --- | --- |
| 1.3.3 | Security membership snapshot consistency | ACTIVE; earlier milestone-specific concurrency coverage not fully recorded |
| 1.3.4 | Project metadata and readiness race protection | ACTIVE; alias rejection/restoration tested; deterministic concurrent mutation testing not proven in recorded evidence |
| 1.3.5 | Project-directory ownership validation | GREEN / ACTIVE; actual foreign-owned filesystem denial not exercised in the reported suite |
| 1.3.6 | Channel and project authorization consistency | GREEN / ACTIVE; same-organization and cross-organization scope borrowing denied in tests |
| 1.3.7 | SA revocation and membership lifecycle | GREEN / ACTIVE; revocation, restoration and role downgrade covered |
| 1.3.8 | HTTP request and resource limits | GREEN / ACTIVE; bounded framing, malformed/oversized input rejection covered |
| 1.3.9 | Security audit event coverage | GREEN / ACTIVE; event classification and sensitive-data exclusion tested; delivery remains best-effort, not independently durable |
| 1.3.10 | Interface regression stabilization | GREEN / ACTIVE; 18 milestone assertions and 17 historical suites passed |
| 1.4.0 | Controlled channel message validation and Core persistence receipts | 17/17; 18 historical suites; GREEN / ACTIVE |
| 1.4.1 | Bounded knowledge query, source-attributed evidence and explicit UNKNOWN | 17/17; 19 historical suites; GREEN / ACTIVE |
| 1.4.2 | Duplicate/invalid evidence result-set rejection | 17/17; 20 historical suites; GREEN; REGISTRY_AUDIT_FULL; activation unconfirmed |
| 1.4.3 | Validated exact record lookup with identity checks | 18/18; 21 historical suites; GREEN; REGISTRY_AUDIT_FULL; activation unconfirmed |
| 1.4.4 | Canonical UTF-8 query/evidence validation | 21/21; 22 historical suites; GREEN; REGISTRY_AUDIT_FULL; activation unconfirmed |
| 1.4.5 | Consistent safe record IDs for bulk and exact retrieval | 17/17; 23 historical suites; compilation corrected and tests pass; candidate detected; qualification/hotload not yet evidenced |

Version assignments are **approved planning targets**, not authorization to change architecture. The STN-LABZ `MAJOR.MINOR.REVISION` sequence uses revisions 0 through 10, rolling over after `X.Y.10` to `X.(Y+1).0`.

## Architectural boundaries

```text
Mission
  └── Modules (Interface, Corpus/RAG, mission capabilities)
        └── Core / Dispatcher / Response
              └── Platform interface
                    └── Target-specific host adapters
```

The three General Orders govern all Digit agent components:

1. Remain at the assigned mission.
2. Follow established policies and authorized instructions.
3. Report anything outside delegated authority to the appropriate next level of authority.

Modules and mission packages cannot override these orders. The common Core binds only to the applicable platform interface; target-specific implementation behavior remains outside common Core semantics. Human authority governs architectural decisions, mission assignments, and approvals.

## Qualification and release gates

Each milestone requires:

1. Source review and code annotation under STN-LABZ engineering policy.
2. Successful compilation.
3. Positive and negative regression validation; module qualification requirements apply.
4. Full requalification after changes, correction of regressions, and operator review of evidence.
5. Runtime confirmation where applicable.

Maintain distinct evidence states: **COMMITTED**, **TESTED**, **QUALIFIED**, and **ACTIVE**. Compilation alone is not qualification.

New modules follow the Module Creation Request (MCR) process and applicable controlled documentation requirements.

## Immediate next objective

**Interface 1.4.5 — Resolve and establish runtime disposition before advancing.**

1. Review the precise runtime sequence after `MODULE UPDATE_CANDIDATE`, including candidate inspection, qualification, incumbent stop, loader admission, registry activation and module start. The last operator log does not identify the halted step.
2. Investigate the recurrent `MODULE REGISTRY_AUDIT_FULL` events seen for 1.4.2–1.4.4 without assuming their cause or applying an unapproved Core change.
3. Review the 1.4.5 change to `interface_reply()`; this HTTP audit/reply refactor was outside the record-identity milestone and its live behavior has not been independently exercised.
4. Require operator-observed `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE` before marking a candidate fully active. If activation remains blocked, report the exact observed failure rather than advancing status speculatively.

**Engineering boundary:** Only `modules/interface` may be changed under current authorization. Digit Core and the external ABI remain off-limits unless the human operator explicitly authorizes changes. Do not delete or reset historical audit evidence as a workaround. The Interface binds to `127.0.0.1:8081` only; authenticated remote access is not enabled.

**Version discipline:** Following 1.4.5, the next sequential revision is **1.4.6**, but no automatic progression or qualification is implied.


---

**Roadmap disposition:** APPROVED development plan (PM 20261009:0123 UTC); evidence/status maintenance update; no new policy approval asserted.  
**Last confirmed active Interface:** 1.4.1 (operator-reported 2026-10-09 01:58:21 UTC).  
**Latest operator-tested candidate:** 1.4.5 (17/17 milestone, 23/23 historical, runtime activation unconfirmed).  
**Next development target:** 1.4.5 runtime disposition and investigation before 1.4.6.

*Engineering systems worthy of trust when trust matters most.*
