# Digit Development Roadmap

**Organization:** STN-LABZ  
**Status:** APPROVED — PM 20261009:0123 UTC  
**Last confirmed active Interface:** 1.4.7 — Qualification GREEN / Hotload ACTIVE (2026-10-09 02:55:15 UTC)  
**Latest tested Interface:** 1.4.7 — 18/18 milestone checks, 25/25 historical suites; zero failures  
**Prepared:** 2026-10-08

> This roadmap is an approved development plan. Approval does not independently authorize architectural changes, supersede controlled documentation, or establish module qualification.

## Mission

Develop Digit into a deterministic, modular, platform-independent autonomous agent capable of performing assigned work within explicitly delegated authority, with human oversight, traceable decisions, and controlled access to organizational resources.

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

## Confirmed baseline and evidence

**Latest operator-confirmed Interface activation:** **1.4.7**, with `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE` at **2026-10-09 02:55:15 UTC**. Its dedicated milestone ran **18/18** assertions, and **25/25** historical regression suites passed with zero failures.

**Prior operational milestone:** 1.4.6, with **21/21** milestone assertions, **24/24** historical suites and `MODULE QUALIFICATION_GREEN` / `MODULE HOTLOAD_ACTIVE` at **2026-10-09 02:46:47 UTC**.

**Deployment investigation:** A read-only investigation of the running process established that **Interface 1.4.5 was actually mapped and active**, despite the earlier incomplete 1.4.5 event excerpt. The investigation also found that installed Core binaries differ from the server's build artifacts and that registry audit capacity had reached **125/128** during historical 1.4.2–1.4.4 replacement deferrals. Preserve the distinction between directly inspected deployment evidence, reported runtime events and passing unit tests.

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
| 1.4.5 | 17/17 | 23/23 | UPDATE_CANDIDATE 02:17:55; later log excerpt incomplete; separately verified active via process-mapped library inspection |
| 1.4.6 | 21/21 | 24/24 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 02:46:47 |
| 1.4.7 | 18/18 | 25/25 | QUALIFICATION_GREEN / HOTLOAD_ACTIVE 02:55:15 |

Milestone checks and event timestamps above are **operator-reported**, except for the separately documented read-only inspection of the live 1.4.5 library. Test success, Core runtime readiness, module qualification and module activation are distinct evidence gates. The investigation confirmed that audit capacity exhaustion caused 1.4.2–1.4.4 replacement deferrals; later 1.4.6 and 1.4.7 events explicitly confirm successful activation. Earlier Interface 1.3.2 evidence: 41/41 project-provisioning assertions, GREEN / ACTIVE 00:18:44 UTC.

Implemented Interface capabilities include:

- Local HTTP interface on `127.0.0.1:8081`; authenticated remote access is not yet enabled.
- Local account and session authentication.
- Protected, scoped channel access-control records.
- Qualified Security Administrator (SA) verification.
- Private project provisioning, project identity validation, and security membership.
- Core-backed restricted security-channel binding and ownership checks.
- Negative validation for malformed records, permissions, hard links, and unauthorized access.
- Source-attributed Corpus query and record retrieval, canonical UTF-8 validation, safe identifier checks and explicit `UNKNOWN` results.
- Legacy Corpus endpoint hardening against oversized Core result counts, invalid records and malformed query/record identifiers (1.4.6).
- HTTP response handling with complete writes and disconnected-peer/SIGPIPE protection (1.4.6).
- Bounded Core channel message-list validation, channel attribution and duplicate/invalid message rejection (1.4.7).

These observations establish a baseline, not a claim that every project-wide test suite has been independently reviewed in this roadmap.

## Development phases

### Phase 1 — Interface foundation

**Status:** Implemented and operator-qualified for the reported foundation milestones. Interface qualification and activation are now confirmed through 1.4.7; broader behavioral and Core lifecycle gaps remain separate.

Scope: HTTP interface, session authentication, protected ACL records, project security provisioning, SA authorization, Core channel binding, and negative security validation.

### Phase 2 — Complete Interface security

**Status:** Completed through 1.3.10 — operator-reported qualification GREEN / hotload ACTIVE.

Milestones 1.3.3–1.3.10 cover protected membership and metadata consistency, directory ownership, channel/project authorization scope, SA and membership revocation, HTTP framing and resource limits, security audit event formatting, and final regression stabilization. Qualification applies to the reported test coverage; it does not establish untested behavior.

### Phase 3 — Controlled communications

**Planned series:** 1.4.x. **Partially implemented and hardened through 1.4.7.** Version 1.4.0 validates structured channel messages and provides Core persistence receipts; 1.4.7 adds bounded, identity-checked channel message-list responses. A persistence receipt does not prove end-to-end delivery or that a recipient read a message. The channel ask pipeline does not yet propagate conversation history, channel identity or authenticated actor to Dispatcher. No automatic guest-to-employee authorization or unrestricted employee direct messages. Remaining delivery-tracking and audit guarantees require separate evidence.

### Phase 4 — Corpus and knowledge integration

**Planned series:** 1.5.x for broader integration. **Interface groundwork and boundary hardening proceeded during 1.4.1–1.4.7**, without skipping the approved sequential revision scheme. Existing `corpus.search` and `corpus.get` services back authenticated evidence-only retrieval. Evidence records retain source identifiers and provenance; the Interface returns explicit `UNKNOWN` on no match. These milestones do not yet establish per-record Corpus authorization, conversational answer synthesis, or end-to-end RAG qualification. Language content belongs in Corpus/RAG rather than hardcoded response banks.

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
| 1.4.5 | Consistent safe record IDs for bulk and exact retrieval | 17/17; 23 historical suites; deployed Interface 1.4.5 independently confirmed mapped/active by read-only investigation; runtime event excerpt incomplete |
| 1.4.6 | Legacy Corpus boundary hardening, SIGPIPE-safe complete HTTP replies and executable Interface qualification checks | 21/21; 24 historical suites; GREEN / ACTIVE 02:46:47 UTC |
| 1.4.7 | Channel message-list count, attribution and identity validation; fail-closed serialization | 18/18; 25 historical suites; GREEN / ACTIVE 02:55:15 UTC |

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

## Outstanding investigation findings (not closed by Interface qualification)

The read-only investigation reproduced incorrect answers for action, arithmetic and comparison requests in the Dispatcher/Response pipeline, and identified these separate unresolved areas:

- **Behavioral pipeline:** action instructions are treated as retrieval text; supported actions are not consistently routed to executable capabilities, and irrelevant evidence can be reported as an answer.
- **Qualification evidence:** several other modules still report fixed passing counters without executable qualification checks. Interface 1.4.6 replaced its own hardcoded counters with executable smoke checks, but this is not end-to-end behavior qualification.
- **Core hotload lifecycle:** installed Core provenance differs from repository build output; audit capacity safeguards, transactional replacement behavior, service-call concurrency and qualification deadlines require separate Core/ABI-authorized engineering.
- **Response comparison evidence:** comparison validation can use its own output as supporting evidence rather than independent retained records.
- **Conversation context:** Interface channel history is persisted, but Dispatcher requests currently receive text without authenticated actor, channel identity or history.
- **Acceptance testing:** full authenticated HTTP behavior, real dispatcher actions, cross-module service results and lifecycle failure modes remain to be qualified.

These findings do **not** revoke the observed Interface 1.4.7 GREEN/ACTIVE runtime event. They constrain what that event establishes.

## Immediate next objective

**Interface 1.4.8 — Next sequential target, not yet committed or qualified.**

Prioritize narrowly scoped, deterministic improvements to Interface boundary validation and authenticated service handling based on remaining investigation findings. Define service-level positive/negative assertions, preserve all historical suites and maintain source/record attribution. Do not claim conversational history or action authorization reaches Dispatcher until the existing contract demonstrably carries it.

**Engineering boundary:** Modifications remain limited to `modules/interface` unless explicitly authorized. Digit Core and the external ABI remain off-limits. Do not reset audit records, change production binaries, or infer system-wide qualification from Interface test results.

**Qualification evidence required:** successful build, dedicated assertions, complete historical regression and operator-confirmed `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE`.


---

**Roadmap disposition:** APPROVED development plan (PM 20261009:0123 UTC); factual evidence/status update, not a new policy approval.  
**Last confirmed active Interface:** 1.4.7 (operator-reported 2026-10-09 02:55:15 UTC).  
**Latest operator-qualified milestone:** 1.4.7 (18/18 dedicated assertions, 25/25 historical suites, GREEN / ACTIVE).  
**Next development target:** Interface 1.4.8 — planning only; unresolved cross-module investigation findings remain open.

*Engineering systems worthy of trust when trust matters most.*
