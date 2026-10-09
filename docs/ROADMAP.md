# Digit Development Roadmap

**Organization:** STN-LABZ  
**Status:** PROPOSED — subject to human review and approval  
**Planning baseline:** Interface 1.3.2 — Qualification GREEN / Hotload ACTIVE  
**Prepared:** 2026-10-08

> This roadmap is a proposed engineering plan. It does not approve an architectural change, supersede controlled documentation, or independently establish qualification.

## Mission

Develop Digit into a deterministic, modular, platform-independent autonomous agent capable of performing assigned work within explicitly delegated authority, with human oversight, traceable decisions, and controlled access to organizational resources.

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

## Confirmed baseline

Interface 1.3.2 has operator-reported **41/41 passing project provisioning assertions**, zero failures, and runtime events confirming **MODULE QUALIFICATION_GREEN** and **MODULE HOTLOAD_ACTIVE** (2026-10-09 00:18:44 UTC).

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

**Status:** Implemented and qualified through Interface 1.3.2.

Scope: HTTP interface, session authentication, protected ACL records, project security provisioning, SA authorization, Core channel binding, and negative security validation.

### Phase 2 — Complete Interface security

**Proposed versions:** 1.3.3–1.3.10.

Finish record consistency validation, concurrent-update detection, authorization-revocation tests, resource limits, security audit coverage, and full applicable regression qualification.

### Phase 3 — Controlled communications

**Proposed versions:** 1.4.x.

Implement structured channel messaging, authorized employee-to-Digit communication, message delivery tracking, and auditable communication records. No automatic guest-to-employee authorization or unrestricted employee direct messages.

### Phase 4 — Corpus and knowledge integration

**Proposed versions:** 1.5.x.

Connect authorized Corpus/RAG retrieval to message handling while preserving source provenance, bounded input validation, and explicit unknown responses. Language content belongs in Corpus/RAG rather than hardcoded chatbot response banks.

### Phase 5 — Autonomous mission operations

**Proposed versions:** 1.6.x and beyond.

Introduce bounded mission tasks, delegated execution, routine anomaly handling within authority, structured status reporting, escalation, recovery, and audit trails.

## Proposed Interface milestones

| Version | Engineering objective | Completion evidence |
| --- | --- | --- |
| 1.3.3 | Security membership snapshot consistency | Concurrent modifications rejected |
| 1.3.4 | Project metadata and readiness race protection | Mutation and replacement regression tests |
| 1.3.5 | Project-directory ownership validation | Unauthorized directory ownership denied |
| 1.3.6 | Channel and project authorization consistency | Cross-project scope borrowing denied |
| 1.3.7 | SA revocation and membership lifecycle | Revocation effective on next authorization request |
| 1.3.8 | HTTP request and resource limits | Oversized/malformed requests rejected |
| 1.3.9 | Security audit event coverage | Authorized and rejected operations traceable |
| 1.3.10 | Interface regression stabilization | Full applicable test suite and runtime qualification GREEN |
| 1.4.0 | Controlled communication milestone | End-to-end authorized channel exchange validated |

Version assignments are **planning targets**, not authorization to change architecture. The STN-LABZ `MAJOR.MINOR.REVISION` sequence uses revisions 0 through 10, rolling over after `X.Y.10` to `X.(Y+1).0`.

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

**Interface 1.3.3 — Security Membership Snapshot Consistency**

Detect changes to protected `security.tsv` metadata during membership evaluation, fail closed on concurrent mutation, add negative regression coverage, and preserve current authorization boundaries.

**Acceptance evidence:** Affected tests pass, the full applicable regression suite passes, and Digit reports `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE`.

---

**Roadmap disposition:** PROPOSED for human approval.  
**Operational baseline:** Interface 1.3.2 GREEN / ACTIVE.  
**Next development target:** Interface 1.3.3.

*Engineering systems worthy of trust when trust matters most.*
