# Digit

**STN-LABZ** · Modular, platform-independent autonomous-agent development

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

Digit uses the **Core → Modules → Mission** architecture. The common Core binds through the applicable platform interface; target-specific implementation behavior stays outside common Core semantics. Human authority governs architectural decisions, assignments, and approvals.

## General Orders

1. Remain at the assigned mission.
2. Follow established policies and authorized instructions.
3. Report anything outside delegated authority to the appropriate next level of authority.

These orders govern the agent and cannot be overridden by module or mission packages.

## Current Interface status

**Operator-reported evidence through 2026-10-09 (UTC).**

| Release | Milestone tests | Historical regression suites | Runtime disposition |
| --- | --- | --- | --- |
| **1.4.0** | 17/17 | 18/18 | Qualification GREEN; HOTLOAD_ACTIVE |
| **1.4.1** | 17/17 | 19/19 | Qualification GREEN; HOTLOAD_ACTIVE |
| **1.4.2** | 17/17 | 20/20 | Qualification GREEN; REGISTRY_AUDIT_FULL; activation not confirmed |
| **1.4.3** | 18/18 | 21/21 | Qualification GREEN; REGISTRY_AUDIT_FULL; activation not confirmed |
| **1.4.4** | 21/21 | 22/22 | Qualification GREEN; REGISTRY_AUDIT_FULL; activation not confirmed |
| **1.4.5** | 17/17 | 23/23 | Build and tests pass after source correction; UPDATE_CANDIDATE observed; qualification and activation not confirmed |

**Last explicitly confirmed active Interface:** **1.4.1**, hotloaded at 2026-10-09 01:58:21 UTC. **Latest tested Interface candidate:** **1.4.5**. Core `RUNTIME_ACTIVE` is not equivalent to Interface `HOTLOAD_ACTIVE`.

The recurring `REGISTRY_AUDIT_FULL` messages were reported for **1.4.2–1.4.4**. Their cause and effect on activation have not been proven. The partial 1.4.5 log ends at `MODULE UPDATE_CANDIDATE`; no subsequent registry or hotload outcome was supplied.

### Implemented Interface work

- Local HTTP interface and session/account authentication.
- Protected channel access controls, Security Administrator checks, and scoped project provisioning.
- Channel message validation and Core-backed **persistence receipts** (not proof of delivery).
- Authenticated knowledge search via the existing `corpus.search` service, with source-attributed evidence and explicit `UNKNOWN` on no match.
- Authenticated exact record retrieval via `corpus.get`, with record-identity checks.
- Bounded result sets, duplicate ID rejection, canonical UTF-8 validation, and consistent safe record identifiers for bulk and single-record lookup.

**Not established by these milestones:** unrestricted employee direct messaging, end-to-end delivery confirmation, record-level Corpus authorization, conversational RAG synthesis, or remote network access.

### Interface network boundary

The Interface listens on **`127.0.0.1:8081`**. Authenticated *remote* access is not yet enabled. Local checks must be performed on the Digit host; external clients cannot be assumed to reach this endpoint.

## Interface build and qualification

From the repository's `modules/interface` directory, with the required external ABI available at the expected sibling location:

```sh
git pull
make clean && make && make test
```

The current Makefile prints the **1.4.5 milestone assertions** individually and runs **23 historical regression suites**, reporting a summary unless a suite fails.

Compiling and passing module unit tests do **not** alone establish operational activation. The operator must inspect runtime evidence, including `MODULE QUALIFICATION_GREEN` and `MODULE HOTLOAD_ACTIVE`. Do not report an unobserved event as successful.

### Current investigation

The 1.4.5 candidate passed its tests after correction of a malformed source-code newline insertion. An additional HTTP reply/audit formatting refactor occurred during that correction; its live request behavior has not been independently verified. Runtime observation for 1.4.5 ended at candidate detection. The specific stage and cause of any subsequent stall are **unknown**.

No audit reset, registry bypass, or unauthorized Core change is prescribed.

## Engineering boundaries

- **Current authorized modification scope:** `modules/interface` only.
- **Digit Core and external ABI:** no changes without explicit human authorization.
- Source changes require annotation, positive/negative testing, historical regression coverage, and operator qualification evidence.
- Milestones advance sequentially using the approved `MAJOR.MINOR.REVISION` scheme (revisions 0–10 before rollover).
- New modules require the applicable Module Creation Request process.

For the full plan, historical milestone evidence, and unresolved runtime gates, see **[Development Roadmap](docs/ROADMAP.md)**.

*Engineering systems worthy of trust when trust matters most.*
