# Digit Development Roadmap

**Organization:** STN-LABZ  
**Status:** APPROVED — PM 20261009:0123 UTC  
**Last confirmed active Interface:** 1.5.10 — Qualification GREEN / HOTLOAD_ACTIVE (2026-10-09 15:25:50 UTC)  
**Latest tested Interface:** 1.5.10 — 16/16 milestone checks, 37/37 historical suites; zero failures  
**Prepared:** 2026-10-08  
**Evidence updated:** 2026-10-09 UTC; operator-reported milestones through Interface 1.5.10

> This roadmap is an approved development plan. Approval does not independently authorize architectural changes, supersede controlled documentation, or establish module qualification.

## Mission

Develop Digit into a deterministic, modular, platform-independent autonomous agent capable of performing assigned work within explicitly delegated authority, with human oversight, traceable decisions, and controlled access to organizational resources.

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

## Confirmed baseline and evidence

**Current evidence supersedes the historical 1.4.7 snapshot below:** Interface **1.5.10** passed **16/16** project inventory tests and **37/37** historical regression suites; the operator reported `MODULE VERSION_CHANGED`, `MODULE QUALIFICATION_GREEN`, HTTPS listener activation, and `MODULE HOTLOAD_ACTIVE` at **2026-10-09 15:25:50 UTC**. Historical 1.4.x evidence remains for traceability.


**Historical 1.4.x operator-confirmed Interface activation:** **1.4.7**, with `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE` at **2026-10-09 02:55:15 UTC**. Its dedicated milestone ran **18/18** assertions, and **25/25** historical regression suites passed with zero failures.

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

## Interface 1.5.x completion record (operator-reported, 2026-10-09 UTC)

| Version | Implemented milestone | Evidence / operational qualification |
| --- | --- | --- |
| 1.5.4 | Digit account/session authorization and Core-owned STN-LABZ SA admission | 14/14 SA checks; GREEN / ACTIVE 05:15:52 |
| 1.5.5 | TLS-only authenticated remote Interface | Missing certificate initially prevented startup; TLS 1.3, Windows remote HTTPS health and certificate trust subsequently verified |
| 1.5.6 | Read-only SA Core channel/alert dashboard | 14/14 dashboard checks; 34/34 historical suites; GUI SA access and counts observed |
| 1.5.7 | Reissued dashboard version for already-installed 1.5.6 baseline | Version bump committed; separate runtime qualification not established here |
| 1.5.8 | SA-only Core-backed channel creation; GUI all-alert listing | 14/14 name checks; GREEN / ACTIVE 14:44:07; channel ACL grants remain separate |
| 1.5.9 | Organization SA policy, project create and Security bind GUI controls | 18/18 policy checks; 36/36 historical suites; GREEN / ACTIVE 15:17:54 |
| **1.5.10** | **Per-organization SA project inventory with project membership filtering** | **16/16 dedicated checks; 37/37 historical suites; GREEN / ACTIVE 15:25:50** |

**Live observations and limits:** Founder `poemei` authenticated and Core SA admission returned authorized. The GUI reported four Core channels and two alerts. An initial channel-list 503 was investigated; the malformed `'general` record was removed under controlled maintenance, preserving the valid `general` identifier and message history. A later empty GUI channel list was attributed to missing `/opt/digit/state/auth/channel_grants.tsv`, which denies access by design. Core inventory is not account channel visibility. Channel provisioning, project inventory, and SA policy tests do not prove completed permanent-channel lifecycles or cross-org end-to-end isolation.

**Approved operator-directed organizational targets:** STN-Labz owns `learn` plus permanent SA `Alerts` and `Security`; Team ChAoS owns its own permanent SA `Security`. Each has independent organization-scoped qualifications, assignments, security membership and channel grants. The Founder may receive explicit assignments in both; ownership alone never bypasses access checks. Existing legacy `alerts` and `Team ChAoS` named Core channels are not evidence of protected organizational migration.

**Presence direction:** GUI right-side window becomes authorized channel presence, not an operational-alert feed. Events reside in the permanent organization's Alerts channel. Presence needs join/leave, expiry, and organization/project/channel visibility boundaries. **This is still planned, not implemented.**

## 2026-10-09 development evidence addendum — Interface 1.6 and Digit Desktop

**Document control:** This addendum records operator-reported test output and Windows GUI observations. It does not alter the existing APPROVED planning disposition or independently approve architectural changes. Historical statements below retain their original evidence cutoff.

| Milestone | Observed state | Evidence limit |
| --- | --- | --- |
| Interface 1.6.10 | Operator-confirmed GREEN and active; scoped administration and verified restricted project-member listing exercised | Does not prove authenticated channel presence |
| Interface 1.6.11 | Operator-reported scope-parser **46 passed, 0 failed** and historical regression **39 suites passed, 0 failed** | A separate 1.6.11 HOTLOAD_ACTIVE log was not supplied with this result |
| Digit Desktop GUI 1.6.10 lineage | Windows BUILD GREEN and screenshots show grouped organizations/projects, `#` channel labels, right-hand Users display, `Digit [AI]` in STN-LABZ `#General`, project-member details and reply history | GUI title remains 1.6.10; screenshot is not proof of channel presence lifecycle or full conversational competence |

**Approved communications direction as clarified by operator:** Digit is an **IRC-like, non-IRC** communications application **driven by Digit**. The user panel may list Digit as an agent participant, and Digit must be invocable from **any authorized channel**; showing her by default in `#General` does not confine invocation to General. Organization boundaries and server-mediated access control remain in force. A list of restricted project members and SA assignments does not equal an authenticated per-channel user-presence list. Presence must ultimately establish authenticated join, leave, expiry and channel-scoped visibility.

**Observed interpretation gaps:** The agent answered a simple greeting in `#General` but responded to “who are you?”, channel-reporting questions and “are you here?” with a fixed inability-to-interpret message in some cases. Conversations are stored per channel, but the Interface/Dispatcher path has not been shown to receive the authenticated actor, organization, project, channel identity and relevant prior messages as structured context. The GUI screenshot also showed “Waiting for Digit...” after displayed responses; track as a separate client state issue. These are **open engineering items**, not qualified abilities.

### Prioritized conversational engineering milestones (planned)

1. **Channel and speaker context contract.** Define validated identity, organization/project/channel, message origin, approved prior-message window and retention/access boundaries between Interface and Dispatcher. Fail closed across organizations; no implicit authority from conversational text.
2. **Intent and dialog handling.** Reliably distinguish ordinary conversation, greetings, identity questions, channel inquiries, follow-up questions, unsupported requests and commands; offer accurate, bounded clarifications. Keep deterministic authorization independent of language parsing.
3. **Grounded responses.** Use authorized Corpus/RAG and module evidence with source attribution; explicitly distinguish evidence, inference and unknown. Do not invent capabilities, identity, incidents or remedial actions.
4. **Error understanding and recommendations.** Map documented errors/alerts to verified causes, severity, relevant logs and bounded mitigation advice. Only qualified diagnostics may propose corrective action; execution requires separately authorized modules and approved procedures.
5. **Agent role and presence.** Display Digit as a distinct `[AI]` channel participant (including General); make invocation available wherever channel access and the request contract permit it. Build authenticated, organization-isolated live user presence rather than treating SA rosters as online users.
6. **Client state and usability.** Verify Waiting/Ready status lifecycle, reconnect behavior, message attribution, channels/users refresh and controlled failure displays.
7. **Acceptance and release.** Add positive, negative and cross-organization dialog/presence tests; malformed contextual data, prompt injection, unsupported actions, stale histories and privilege escalation must fail safely. Retain full module regression, operator review and runtime qualification before ACTIVE disposition.

**Acceptance principle:** Natural conversation and evidence-based recommendations may improve; **authority remains deterministic, explicitly delegated, and human controlled**. Test passing never substitutes for demonstrated runtime or mission behavior.

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

**Proposed versions:** 1.6.x and beyond. Interface 1.5.10 is the latest operator-confirmed baseline; 1.6 must first close organizational grants, permanent administrative channels and presence before asserting completed multi-organization operations.

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

## Immediate next objective — 1.6 planning gate

**Interface 1.5.10 is ACTIVE, not merely a candidate.** The next work should complete the authority model, not infer features from dashboard counts.

1. **GUI-managed scoped access:** Introduce human-directed creation and review of organization/project/channel grants; preserve private regular files and fail-closed validation, no global SA bypass.
2. **Permanent administrative channels:** Bootstrap organization-owned Security and Alerts channels, prohibit ordinary removal, audit provisioning and preserve the existing legacy data without assuming migration.
3. **Presence:** Replace the right-hand GUI operational-alert list with authenticated channel membership/presence, scoped by each operator's grants; route retained operational notices into the organization's Alerts channel.
4. **Cross-organization tests:** Explicitly prove STN-Labz-only SA cannot enumerate/manage Team ChAoS, and vice versa. Founder access to both requires two explicit active assignments.
5. **Runtime acceptance:** Dedicated executable qualification tests, complete historical regression and operator-confirmed qualified activation, plus real client positive/negative access tests.

**Still unresolved outside this milestone:** Dispatcher/Response behavior and evidence correctness, action routing, conversation context propagation, module qualification depth, durable audit/lifecycle guarantees and broader Core/ABI research. These remain bounded by the existing human authorization requirements.

**Engineering boundary:** Architectural, Core and external ABI modifications require explicit human approval. No production change is authorized by this roadmap update alone.

---

**Roadmap disposition:** Existing APPROVED planning document; evidence update dated 2026-10-09 UTC, not a new policy approval.  
**Last confirmed active Interface:** 1.5.10 (operator-reported 2026-10-09 15:25:50 UTC).  
**Latest operator-qualified milestone:** 1.5.10 (16/16 dedicated assertions, 37/37 historical suites, GREEN / ACTIVE).  
**Next development series:** 1.6.x — planning, implementation and qualification pending.

*Engineering systems worthy of trust when trust matters most.*
