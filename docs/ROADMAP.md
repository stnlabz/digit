# Digit Development Roadmap

**Organization:** STN-LABZ  
**Status:** APPROVED — PM 20261009:0123 UTC  
**Current operational baseline:** Interface 1.3.10 — Qualification GREEN / Hotload ACTIVE  
**Prepared:** 2026-10-08

> This roadmap is an approved development plan. Approval does not independently authorize architectural changes, supersede controlled documentation, or establish module qualification.

## Mission

Develop Digit into a deterministic, modular, platform-independent autonomous agent capable of performing assigned work within explicitly delegated authority, with human oversight, traceable decisions, and controlled access to organizational resources.

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

## Confirmed baseline

Interface **1.3.10** is the latest operator-confirmed operational baseline. The 1.3.10 milestone reported **18/18 passing assertions**, **17/17 historical regression suites passing**, zero failures, and runtime `MODULE QUALIFICATION_GREEN` / `MODULE HOTLOAD_ACTIVE` on **2026-10-09 at 01:43:53 UTC**.

### Interface security milestone evidence

| Version | Milestone tests | Historical regression suites | Runtime evidence (UTC, 2026-10-09) |
| --- | --- | --- | --- |
| 1.3.3 | Prior operator-confirmed audit regression 10/10; dedicated milestone count not established here | Not recorded here | HOTLOAD_ACTIVE 00:55:16 |
| 1.3.4 | 12/12 | 11/11 | HOTLOAD_ACTIVE 01:04:33 |
| 1.3.5 | 7/7 | 12/12 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:13:57 |
| 1.3.6 | 15/15 | 13/13 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:23:34 |
| 1.3.7 | 15/15 | 14/14 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:29:22 |
| 1.3.8 | 14/14 | 15/15 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:34:28 |
| 1.3.9 | 19/19 | 16/16 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:40:02 |
| 1.3.10 | 18/18 | 17/17 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 01:43:53 |

All results above are **operator-reported** and were not independently reproduced by this roadmap update. Earlier Interface 1.3.2 baseline: 41/41 project-provisioning assertions; GREEN / ACTIVE at 00:18:44 UTC.

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

**Status:** Implemented and operator-qualified through Interface 1.3.10.

Scope: HTTP interface, session authentication, protected ACL records, project security provisioning, SA authorization, Core channel binding, and negative security validation.

### Phase 2 — Complete Interface security

**Status:** Completed through 1.3.10 — operator-reported qualification GREEN / hotload ACTIVE.

Milestones 1.3.3–1.3.10 cover protected membership and metadata consistency, directory ownership, channel/project authorization scope, SA and membership revocation, HTTP framing and resource limits, security audit event formatting, and final regression stabilization. Qualification applies to the reported test coverage; it does not establish untested behavior.

### Phase 3 — Controlled communications

**Proposed versions:** 1.4.x.

Implement structured channel messaging, authorized employee-to-Digit communication, message delivery tracking, and auditable communication records. No automatic guest-to-employee authorization or unrestricted employee direct messages.

### Phase 4 — Corpus and knowledge integration

**Proposed versions:** 1.5.x.

Connect authorized Corpus/RAG retrieval to message handling while preserving source provenance, bounded input validation, and explicit unknown responses. Language content belongs in Corpus/RAG rather than hardcoded chatbot response banks.

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
| 1.4.0 | Controlled communication milestone | **NEXT — PLANNED / NOT QUALIFIED** |

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

**Interface 1.4.0 — Controlled Communication Milestone**

Implement and validate authorized, structured channel communication, delivery tracking, and auditable communication records within the established Core service contracts and existing module boundaries. No automatic guest-to-employee authorization and no unrestricted employee direct messages.

**Scope:** Interface module changes only unless explicitly authorized otherwise. Digit Core and the external ABI are not in scope.

**Acceptance evidence:** Dedicated positive and negative 1.4.0 milestone tests, full applicable regression validation, operator review, and runtime `MODULE QUALIFICATION_GREEN` / `MODULE HOTLOAD_ACTIVE`. Planning does not establish qualification.


---

**Roadmap disposition:** APPROVED: PM 20261009:0123 UTC.  
**Operational baseline:** Interface 1.3.10 GREEN / ACTIVE (operator-confirmed 2026-10-09 01:43:53 UTC).  
**Next development target:** Interface 1.4.0.

*Engineering systems worthy of trust when trust matters most.*
