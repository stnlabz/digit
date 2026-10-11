# Agent Root 1.0 — Integration Contract (DRAFT)

Status: IMPLEMENTATION DESIGN — APPROVED [PM: 20261011:0225Z]
Date: 2026-10-10
Scope: Digit agent Root; reusable common Root contract for future STN-LABZ agents.
Authority: Human-directed STN-LABZ development. This document does not grant deployment or Core-edit authorization.

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

## 3. Proposed integration mechanics (not yet authorized)

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

## 5. Decision gate before source changes

Explicit operator approval is required for the precise Core files, API exposure mechanism, startup failure semantics, integrity trust anchor, and build-time choice of agent-specific descriptor.

Until that approval: no Core source edits, no ABI change, no install, no hotload.

Engineering principle: Determinism != Probably.
