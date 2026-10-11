# Agent Root 1.0 — Integration Contract

Status: IMPLEMENTATION DESIGN — APPROVED [PM: 20261011:0225Z]
Date: 2026-10-10
Scope: Digit agent Root; reusable common Root contract for future STN-LABZ agents.
Authority: Human-directed STN-LABZ development. Scoped Core source implementation authorized by the operator on 2026-10-10; deployment remains separately gated.

## 1. Approved architectural decisions

- Root governs who an agent is; Modules govern how the agent expresses it; Mission governs assigned work.
- Root -> Modules -> Mission preserves the established Core -> Modules -> Mission architecture; Root is a foundational contract within the agent's Core, not a fourth execution tier.
- The Three General Orders are foundational and cannot be overridden by modules or missions:
  1. Remain at the assigned mission.
  2. Follow established policies and authorized instructions.
  3. Report anything outside delegated authority to the appropriate next level of authority.
- Digit is Ezra's older sister; Ezra is Digit's younger sister. This establishes identity and lineage, not authority.
- Digit's enduring character: stoic, loyal to authorized mission, curious within authority, truthful, respectful, accountable, and capable of appropriate humor.
- Root determines enduring identity/character requirements. Conversational wording, knowledge, and response strategy remain the responsibilities of authorized modules.

## 2. Core Root and per-agent identity distinction

A. Common Root enforcement: General Orders, integrity validation, precedence, authorization boundary, module/mission nonoverride behavior, human-approved revision control, and auditability.

B. Per-agent Root identity:
- agent_id: stable nonempty unique identifier
- agent_name: display identity
- organization_id: STN-LABZ
- root_schema_version: versioned identity contract
- lineage: named relationships without implied control privileges
- governing_characteristics: normative set of approved traits
- root_revision and integrity identity: verifiable build metadata

Digit identity: agent_id=digit; agent_name=Digit; relation=older_sister_of:ezra; lineage=foundational autonomous agent.

Ezra identity is separately compiled and independently validated; relation=younger_sister_of:digit; intended mission role=space autonomous agent. Ezra's flight software remains independently qualified.

The common Core MUST NOT embed the string 'Digit' as the universal agent identity. Agent-specific fields belong to agent-bound build artifacts, not mutable Corpus records, runtime lessons, or mission configuration.

## 3. Integration mechanics (Core-private initial scope authorized)

1. Define a small, versioned, read-only agent Root descriptor in agent-specific source. Prefer a narrow static constant with bounded strings/fields and explicit enum relationships.
2. At startup, before module discovery, validate descriptor schema, required fields, approved General Orders and compiled-root integrity metadata. Fail closed if identity or governing Root integrity fails; report through available Core diagnostics/audit.
3. Publish a read-only, bounded Root identity query contract to authorized modules. A query conveys identity and traits but confers no operational permission. Any shared Core service or host interface addition requires separately approved Core/ABI scope; do not silently extend the external STN-LABZ ABI.
4. Authorize operational requests through existing authenticated identity and Core authority services. Root identity relationships must never supply credentials or access rights.
5. Keep Root configuration independent of Corpus: learning, lesson imports, prompt text and modules may neither mutate Root nor supersede its priorities.
6. Revise Root only through operator-approved source change, version bump, controlled build and qualification; record exact revision/build digest and audit evidence.
7. Ensure agents can operate without any conversational module while retaining Root validity.

## 4. Required acceptance and negative testing

- Common Root unchanged when swapping the agent-specific identity descriptor; Digit and Ezra have distinct identities.
- Missing, malformed or conflicting descriptor fails startup closed.
- Attempted identity rewrite through Corpus, lesson, mission or module is rejected or ignored with an auditable result.
- Sister relationship produces no elevation or cross-agent command entitlement.
- General Orders retain precedence over subordinate behavior.
- Root query remains bounded, read-only, and does not leak operational authority.
- Existing Core services and external ABI remain binary compatible until a separately approved change.
- Character rules do not inject canned conversational text; Response/Intent/Validator remain responsible for expression and factual acceptance.
- Qualification tests execute measured positive/negative cases; runtime activation and conversational acceptance are separate gates.

## 5. Source authorization and deployment gate

Operator authorization was given to implement the exact Core integration scope described in Section 6. The chosen initial API is Core-private; no external ABI modifications or module-visible Root service are authorized. The compiled descriptor is schema-validated, not cryptographically attested. Production installation and deployment remain unauthorized until Linux qualification and a separate operator deployment decision.

Engineering principle: Determinism != Probably.

## 6. Authorized Core implementation scope

Repository inspection (2026-10-10) identified these existing integration points:
- src/digit.c: digit_initialize() opens audit, registers services, then discovers modules. Root validation should run immediately after audit opens and before module discovery or service registration.
- include/digit.h: current entry contract (DIGIT_NAME, digit_initialize) is retained for backward compatibility.
- Makefile: DIGIT_SOURCES and test targets explicitly enumerate source files; Root sources and a dedicated test target must be added here.

Proposed newly added files:
- include/agent_root.h — Core-private bounded immutable identity descriptor and validation/query API (not the external ABI).
- src/agent_root.c — common Root schema and invariants validator.
- src/identity_digit.c — Digit-specific compiled identity, seven approved characteristics, lineage and governing requirements.
- tests/test_agent_root.c — executable positive/negative contract tests.

Proposed modifications to existing files (no others):
- src/digit.c — invoke Root validation before any module discovery, reject startup on invalid Root; log structured root status without leaking sensitive state.
- Makefile — compile new files and add the Root test to the normal qualification target.
- include/digit.h only if needed for compatibility-preserving access to the descriptor; otherwise leave unchanged.

Initial API visibility: Core-private read-only query, with no service-registry exposure. Any future module-visible Root identity service is a separate, explicitly approved interface change. This keeps external ABI unaffected and prevents premature authorization implications.

Failure semantics: Root-invalid must stop initialization before module discovery; fail-closed and record an audit event where audit is available. A root validation failure is not degraded operation. Handle missing audit separately using existing fatal behavior.

Integrity model: const-qualified compile-time descriptor and validation of its schema/constants provide runtime immutability at the API boundary, not cryptographic build provenance. A cryptographic integrity trust anchor, signed-binary verification, or secure boot must not be claimed without separately engineered verifiers and trusted keys. Production deployment remains a later gate.

Portability: Use only common C11 logic in the descriptor and validator. Platform-specific binary attestation, storage, and interface binding stay outside common Root semantics.

Qualification gates: descriptor identity and version, exact General Orders, mandatory traits, bounded strings, duplicate/conflicting identifiers, malformed relationships, agent-swap test (with independent fixture), no root-to-authority elevation, rejection before discovery, and existing Core regression tests. A module-visible query must be separately tested if it is eventually approved.

The operator subsequently authorized the scoped Core implementation. Source has been committed to GitHub main for Linux qualification. No deployment authorization is implied.

## 7. Implementation record

Committed initial Core-private Agent Root v1.0: include/agent_root.h, src/agent_root.c, src/identity_digit.c, tests/test_agent_root.c; amended src/digit.c and Makefile. Root validation occurs after audit initialization and before any module discovery or registration. Invalid Root prevents Core startup; successful validation emits a ROOT VALIDATED audit event. Existing STN-LABZ external ABI is unchanged. No module-visible identity service, cryptographic build attestation, or production deployment is claimed. Linux build and regression evidence remain pending.
