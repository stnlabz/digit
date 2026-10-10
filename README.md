# Digit

**STN-LABZ** · Modular, platform-independent autonomous-agent development

**Engineering principle:** Small, deterministic, and easy to use. **Determinism ≠ Probably.**

Digit follows **Core → Modules → Mission**. The common Core binds through its platform interface, with target-specific behavior outside shared Core semantics. Architecture, authorization and deployment remain under human control.

## General Orders

1. Remain at the assigned mission.
2. Follow established policies and authorized instructions.
3. Report anything outside delegated authority to the appropriate next level of authority.

Modules and mission packages cannot override these orders.

## Current development status

**Evidence updated: 2026-10-09 (operator runtime observations, GitHub pipeline test evidence and historical screenshots).** Interface **1.6.10** was previously operator-qualified and hotloaded; the **1.6.11** build subsequently passed the reported 46/46 scope-parser tests and 39/39 historical regression suites. A 1.6.11 HOTLOAD_ACTIVE event was not separately provided in this update. Keep compilation/tests, runtime activation, and behavioral acceptance distinct.

| Milestone | Qualification | Runtime evidence |
| --- | --- | --- |
| 1.5.4 | 14/14 dedicated SA checks; historical suites passed | GREEN / ACTIVE 05:15:52 UTC |
| 1.5.5 | TLS remote access / HTTPS; historical tests passed | Initial startup failed without cert/key; subsequently HTTPS verified locally and from Windows |
| 1.5.6 | 14/14 dashboard checks; 34/34 historical suites | Operator reported installed; runtime dashboard displayed SA aggregates |
| 1.5.7 | Version increment for refreshed dashboard release | Tested/installed status not separately established in this summary |
| 1.5.8 | 14/14 channel-name boundary checks; historical suites passed | GREEN / ACTIVE 14:44:07 UTC |
| 1.5.9 | 18/18 organization SA policy checks; 36/36 historical suites | GREEN / ACTIVE 15:17:54 UTC |
| **1.5.10** | **16/16 project-list checks; 37/37 historical suites** | **GREEN / ACTIVE 15:25:50 UTC** |
| 1.6.10 | Scoped channels, organization SA roster and restricted project-member directory | Operator-confirmed GREEN / active; Windows live member-directory screenshots |
| **1.7.0** | **Project-member response distinguishes verified active account, restricted membership and organization SA authority** | **46/46 scope-parser tests; 39/39 historical suites, zero failures; hotload event not separately shown** |

The historical Interface 1.3.x–1.4.x milestone record is retained in [Development Roadmap](docs/ROADMAP.md). Unit tests, module qualification, activation, and end-to-end authorization are **distinct** evidence gates.

## Current language-pipeline release gate — 2026-10-09

**Disposition: OPEN / NOT END-TO-END QUALIFIED.** Individual modules may compile and pass their unit/qualification checks without establishing that Digit understands and answers an operator's request correctly. No integrated conversational release is accepted merely because its constituent modules report GREEN.

The current pipeline spans **Corpus → Interpretation → Intent → Dispatcher → Response → Validator**, with the authenticated Interface responsible for supplying the permitted session and conversation scope. Core and the external ABI remain outside the authorized scope of this language repair.

**Live operator observations** (the results below are observations, not the completion of an acceptance suite):

| Operator request | Observed Digit reply | Disposition |
| --- | --- | --- |
| “What's your name and what do you do?” | “I don't have enough grounded information to answer that.” | FAIL — does not compose related established facts |
| “Digit are you here?” | “Yes I am here” | PASS for this observed interaction |
| “Digit, who are you?” | “I am Digit, the STN-Labz Autonomous Engineering Agent.” | PASS for this observed interaction |
| “What does wut mean?” | “wut means What.” | PASS for this observed interaction |

These four exchanges are not a comprehensive regression result. The same grounded concepts must work under ordinary variations, multi-part requests, negative examples and scoped conversational follow-ups, without canned prompts or hardcoded agent biographies.

**Coordinated-repair requirement:** Treat the language pipeline as **one release unit**. Agree the cross-module request/evidence/response/validation contracts, implement all interdependent repairs together, exercise Codex's complete 14-case behavioral audit and all existing module suites, add negative authorization/grounding/regression cases, and evaluate actual end-to-end outputs. Correct failures in the same coordinated release candidate. Do not promote sequential, individually GREEN module patches as evidence of system acceptance.

**Current CI evidence:** A prior GitHub Actions language-pipeline run passed module build/test stages (Corpus, Interpretation, Intent, Response, Validator, Dispatcher and Interface). This is **build and test evidence**, not proof that the full live conversation acceptance corpus passed. Do not label the coordinated language repair QUALIFIED or ACTIVE until integrated tests and operator review establish those states.

**Independent operational issue:** Qualification persistence reached its fixed 128-entry implementation capacity and prevented an Interface startup after restart despite prior hotload qualification evidence. The source currently declares `DIGIT_QUALIFICATION_MAX 128` in `include/qualification.h`. This is not a qualification-history retention policy. Increasing the constant is only temporary capacity relief; an uncapped, history-preserving design requires separately authorized Core engineering. No Core modification is authorized by this README.

### Structured Alerts channel projection — 2026-10-09

The Interface now presents already-authorized Core alert records in the protected operations Alerts channel as labeled incident fields consumed by Digit Desktop: `severity`, `summary`, `module`, `subsystem`, `version`, `event_id`, `timestamp`, `cause`, `action`, plus original `detail`, `operational_state` and acknowledgment status. It retains the existing Core service, organization-specific SA/project-membership checks, channel binding and fail-closed response handling.

**Provenance rule:** `module` is filled only when the Core alert source explicitly uses `module:<valid-id>`. The originating `source` is retained as `subsystem`. Core's existing alert record does not separately encode a verified cause, module version or remediation action, so the projection returns `UNKNOWN` for those fields; it never invents a diagnosis. The Desktop provides readable incident cards and detail expansion for this representation.

**Disposition:** Interface source COMMITTED. No Linux build, integrated GUI/Interface runtime acceptance, or operator activation has been verified for this new change. Reliable module attribution for other alert-source formats requires upstream alert-producer evidence and integrated qualification, not a client-side guess. No Core change is implied or authorized here.

## Interface and Windows GUI

- HTTPS endpoint: **`https://digit.stn-labz.com:8081`**. Interface listens on the IPv4 wildcard address and requires TLS. Windows HTTPS validation was confirmed against the operator-installed temporary self-signed certificate; replace it with an appropriately trusted certificate for sustained operations.
- `poemei` authenticated through Digit GUI. The Core-controlled STN-LABZ SA read-access check was observed successful.
- The read-only `GET /admin/dashboard` reports Core channel/alert totals; the GUI displayed four channels and two alerts in operator testing. Dashboard counts do **not** confer channel ACL access.
- GUI and Interface 1.5.9 introduced SA project creation and Security binding controls. Later releases implement organization-scoped project listing, scoped channels, separate SA administration and an SA-authorized restricted project-members directory. The 1.6.11 member response independently exposes account-active verification, restricted project membership and organization SA verification.
- `POST /channels` in 1.5.8 allows an authorized SA to request Core channel creation; this does **not** automatically grant visibility or membership.

### Project storage and explicit authority

Protected project root: `/opt/digit/state/projects/<organization>/<project>/`. Project provisioning records include `project.tsv`, `security.tsv` and `READY`; restricted Security-channel binding uses `security_channel.id`. The channel-access registry is **`/opt/digit/state/auth/channel_grants.tsv`**, not `channel_acl.tsv`. Missing, invalid or unauthorized grants fail closed.

Organization and project scopes are separate. A valid SA assignment to **stn-labz** does not authorize **team-chaos**. The Founder may hold both assignments independently, subject to the controlled roster; that second assignment has not been evidenced as provisioned. Project and channel access also require their corresponding membership/grants.

**Observed GUI communications baseline:** The native Windows client presents organization → project → `#channel` navigation, retained channel conversations, a right-side Users display, and `Digit [AI]` in `#General`. Digit can be invoked from other authorized channels via the existing request path; this does not establish that she understands channel intent or history. The current Users display derives from an SA-protected project directory (and a General-channel Digit projection), **not** authoritative live channel presence. On-screen proof exists for STN-LABZ `#General` and `#Security`, with working replies and limitations in natural-language interpretation. The status line can remain at "Waiting for Digit..." after a response and needs verification. The GUI layout and member details were visually confirmed after a Windows GREEN build.

**Operator-directed target layout:**
- **STN-Labz:** `learn`, permanent SA `Alerts`, permanent SA `Security`.
- **Team ChAoS:** permanent SA `Security`.
- The existing legacy `general`, `learn`, `alerts`, and `Team ChAoS` channel records are not automatically migrated into those scopes. Preserve their message history.
- The right-side GUI panel is intended to become **channel Presence**. Operational events belong in the permanent organization-scoped Alerts channel; live presence must be tracked separately.

**Not yet complete:** Authenticated channel-specific presence (join/leave/expiry), end-to-end multi-organization acceptance, remaining channel-grant lifecycle work, permanent administrative channel lifecycle policy, legacy channel migration, conversational understanding, channel/actor/history context propagation, reliable intent/action routing, evidence-grounded recommendations, and consistent GUI waiting/ready status. Do not equate the 1.5.9 policy test or 1.5.10 listing test with these features being live.

## Build and qualification

From `modules/interface`, with the sibling external ABI available:

```sh
make clean && make && make test
```

For the native Windows GUI, use its repository's `build.cmd` in an MSVC environment. Do not publish credentials, account hashes, private keys or authorization registries.

Historical Interface 1.7.0: operator-reported **46 scope-parser tests and 39 historical suites passed**, zero failures. The previous 1.6.10 activation is separately evidenced; 1.7.0 hotload is not established by test output alone. Any new source change requires compilation, positive/negative tests, complete regression qualification, and operator review before activation.

## Next development phases

1. **Communications foundation:** Preserve the IRC-like (not IRC protocol) user experience with Digit-native channel communications, scoped organization/project access, `#` display prefixes and agent participation. The GUI foundation is observed; real per-channel human presence remains pending.
2. **Context propagation:** Attach authenticated actor, organization, project, channel, conversation history and applicable mission context to the Dispatcher/Response pipeline with explicit validation and scope boundaries.
3. **Understanding and response:** Distinguish greetings, questions, follow-ups, operational inquiries and authorized action requests; provide an accurate agent identity and helpful bounded clarification rather than a generic command failure when appropriate.
4. **Evidence-grounded support:** Return known/unknown explicitly, cite verified records, interpret qualified error classes and offer tested corrective recommendations; do not claim diagnosis without evidence or act beyond authorization.
5. **Operational acceptance:** Test positive/negative cross-org behavior, channel-aware dialog, status-state transitions, history safety, presence isolation, failure modes and complete regressions before promotion.

These are **planned objectives** where not otherwise evidenced, not release or deployment authorization.

## Engineering boundaries

Core, external ABI and common autonomous-agent architecture changes require explicit human authorization. Preserve deterministic behavior, annotated source changes, negative tests, historical regression, and the Module Creation Request process when applicable.

See [Development Roadmap](docs/ROADMAP.md) for detailed evidence, development phases and outstanding gates.

*Engineering systems worthy of trust when trust matters most.*
