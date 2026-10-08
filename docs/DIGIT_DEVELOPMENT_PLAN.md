# Digit Development Plan

## Purpose

Digit is an STN-LABZ autonomous agent. Development will keep Digit small, deterministic, auditable, modular, and operator-directed.

The objective is not to make Digit merely produce plausible language. Digit must distinguish established information from generated language, preserve operator intent, validate at defined boundaries, know who is issuing a command, determine whether that operator is authorized for the requested action, and execute only through capabilities actually available to her.

Determinism is preferred over probability wherever an authoritative decision can be made by software.

---

## 1. Core Operating Model

Digit's Core remains small. Functionality belongs in modules unless it is genuinely a Core responsibility.

The General Orders remain foundational operational directives:

1. Remain at the assigned mission.
2. Follow established policies and authorized instructions.
3. Report anything outside authority to the appropriate next level of authority.

Modules do not replace, reinterpret, or override the General Orders.

Digit must not hard-code factual answers merely to make individual prompts pass. Knowledge is retained or retrieved from authoritative sources and then used to answer the operator's actual question.

---

## 2. Deterministic Request Pipeline

The intended request path is:

```text
Authenticated Operator
        |
        v
Input Validation
        |
        v
Dispatcher
        |
        +--> deterministic service/action when available
        |
        +--> retrieval / retained knowledge when required
        |
        v
Response Generation
        |
        v
Output Validation
        |
        v
Operator
```

Validation occurs at the defined input and output boundaries through the Validator module. Response and other modules will not independently recreate competing validation systems.

The operational pattern is deliberately simple:

```text
send
  -> validate
     -> reject for correction when invalid
        -> send again
           -> accept or reject
```

A validation failure must identify the actual failure. It must not silently transform the operator's request into another task.

---

## 3. Operator Identity and Authentication

Digit must know who is issuing a command.

Natural-language conversation is not sufficient proof of identity. The GUI will provide a login mechanism backed by server-side authentication. Authentication establishes the operator identity before commands are accepted as authenticated operator instructions.

The GUI may display a friendly operator name, but the internal identity must come from authenticated session state rather than text entered into the conversation.

The intended path is:

```text
Digit GUI Login
      |
      v
Server Authentication
      |
      v
Authenticated Session
      |
      v
Operator Context
```

Credentials will not be treated as ordinary conversation content and authentication will not be delegated to Llama.

---

## 4. Operator Context

An authenticated Digit session will establish authoritative operator context. At minimum, the context is intended to identify:

- operator identity;
- organizational position or role where applicable;
- qualifications and certifications;
- current mission assignment;
- current mission status; and
- resulting authorized operational scope.

These facts are deterministic session state. They are not facts that Llama is permitted to infer from wording, retrieved lessons, conversational familiarity, or apparent technical ability.

Authentication answers:

> Who is issuing this command?

Qualification and certification answer:

> What has this operator been qualified to perform or direct?

Mission assignment and status answer:

> What authority is presently applicable to this operator's work?

Authorization then determines whether the requested operation is permitted in the current context.

---

## 5. Authorization Before Generation

Authorization is not a language-model decision.

The authorization path will be deterministic:

```text
identity
   + position
   + qualification/certification
   + mission assignment/status
   + applicable policy
        |
        v
AUTHORIZED / NOT AUTHORIZED
```

Once authorization has been granted, Response or Llama must not reopen that decision because retrieved material contains generic security guidance about permissions.

Retrieved knowledge may constrain how an authorized action is performed. It does not independently revoke an authorization decision made by the authoritative authorization layer.

Likewise, an authenticated identity alone does not imply unlimited authority. Authentication and authorization remain separate decisions.

---

## 6. Preserve Operator Intent

Dispatcher must preserve the operator's requested action.

For example:

```text
create a user module controller for a test module
```

is a creation request. It must not become a source scan, a permission lecture, an explanation of module security, or a generic discussion of controllers merely because those topics occur in retrieved material.

Routing must distinguish among requests such as:

- explain something;
- retrieve retained knowledge;
- inspect source;
- search source;
- generate an artifact;
- create or modify an artifact through an available service; and
- perform another supported operational action.

Named projects and paths are context for the requested operation. Their presence alone must not change an action request into an informational source query.

---

## 7. Capability Is Separate From Authorization

Authorization does not imply that Digit currently possesses the technical capability needed to perform an action.

The decision sequence is:

```text
Who is the operator?
        |
Is the operator authorized?
        |
Does Digit have the required capability?
        |
Perform the action or report the actual capability limitation
```

If an authorized operator requests a filesystem change but Digit has no authorized source-write service, Digit must not claim that the operator lacks permission. She must report the actual limitation.

Conversely, a capability being available does not itself authorize its use.

This distinction applies broadly to filesystem operations, source modification, external communications, mission operations, and future modules.

---

## 8. Source and Development Capabilities

Digit's source-development capability will evolve beyond read-only inspection.

Existing source-oriented behavior includes project discovery, bounded search, source reading, and repository/status inspection.

Planned development behavior includes controlled source mutation so an authorized development request can result in an actual artifact rather than only generated prose.

Source-write functionality must remain bounded to authorized project/workspace locations and must preserve deterministic path handling. The development system must never claim that a file was written when no write service performed the operation.

For development requests, retained knowledge and source material may establish project conventions and constraints. They must support the requested work rather than replace it with commentary.

---

## 9. Retrieval and Evidence

Retrieval exists to supply relevant knowledge, not to hijack requests.

Evidence selection must distinguish relevance from authority. A retrieved sentence can be related to a subject without governing the requested operation.

Digit must prefer evidence that directly answers the requested subject. For ordinal or specifically referenced information, retrieval must preserve that reference rather than allowing broadly related material to outrank the requested item.

Example:

```text
Digit, what is your third General Order?
```

requires evidence establishing the Third General Order. General statements about the existence or binding nature of the General Orders do not answer that question.

Evidence narration is not automatically part of the user-facing answer. Provenance or evidence should be shown when requested or when an applicable interface/policy requires it.

---

## 10. Learning

Operator learning remains bounded and deterministic.

Interactive learning is one line per learning operation. Multi-line conversational input must not be silently accepted as multiple learned facts.

Learning must not be used as a mechanism for hard-coding prompt-specific answers into Response or Dispatcher.

Retained knowledge remains evidence. It does not automatically become policy, authorization, or executable authority merely because it was learned.

---

## 11. Response Behavior

Response exists to produce the answer or requested generated result after upstream routing has established the task and supplied appropriate context.

Response must not become a second Dispatcher, Validator, authentication system, or authorization system.

A successful answer should:

- answer the requested subject directly;
- preserve the requested action;
- use grounded retained/source information when required;
- avoid unsupported technical claims;
- avoid irrelevant evidence narration;
- avoid echoing the request as though it were an answer;
- avoid replacing an action with generic security boilerplate; and
- state genuine capability limitations accurately.

Formatting will be improved so structured answers can use readable numbered or otherwise appropriate formatting instead of unbroken generated prose.

---

## 12. Validator Responsibility

Validator is the validation authority for the defined input/output validation boundaries.

Other modules must not accumulate their own overlapping interpretations of Validator policy. This prevents contradictory rejection paths and repeated whack-a-mole fixes across modules.

The desired relationship is:

```text
candidate
   |
Validator
   +--> ACCEPT
   |
   +--> REJECT: reason
            |
         correction / regeneration
            |
         Validator again
```

Validation must test the candidate against the applicable evidence and rules without manufacturing unrelated requirements.

---

## 13. GUI Direction

Digit GUI is the operator-facing control surface.

The current GUI already provides conversation channels, history, alerts, and communication with Digit. The next major GUI development is authenticated operator access.

Planned GUI responsibilities include:

1. operator login;
2. authenticated session handling;
3. display of the authenticated operator identity;
4. access to applicable operator context;
5. command submission under that authenticated session;
6. clear reporting of authorization or capability failures; and
7. logout/session termination.

The GUI will not decide authorization by itself. It collects credentials, establishes a server-authenticated session, and carries that trusted session identity with subsequent requests.

---

## 14. Server Authentication Direction

Digit requires a server-side authentication facility corresponding to GUI login.

The server side will be responsible for establishing that supplied credentials correspond to a known operator and for issuing or maintaining the authenticated session used by the GUI.

Operator identity data, qualification/certification data, and mission state must have authoritative sources. Their exact persistence format and service boundaries will be implemented deliberately rather than inferred or fabricated by the language model.

Authentication failures will fail closed.

---

## 15. Mission Awareness

Digit is an operational agent, so authorization may depend on current mission context rather than identity alone.

A person may be authenticated and qualified while not presently assigned to an operation requiring a particular authority. Mission assignment and mission status therefore form part of operator context where applicable.

This allows Digit to distinguish:

```text
known operator
qualified operator
authorized operator for this mission
```

without asking Llama to estimate the relationship.

---

## 16. Module Architecture

Digit remains module-driven.

Modules expose bounded services and are hot-loaded through the established module architecture. New capabilities should be implemented in the module that owns that responsibility instead of spreading duplicate logic across unrelated modules.

Examples of responsibility boundaries include:

```text
Validator   -> input/output validation
Dispatcher  -> intent preservation and service routing
Response    -> grounded response/result generation
Source      -> source-project operations
Lesson      -> lesson ingestion
GUI         -> operator interface and login/session presentation
Core        -> common agent/runtime responsibilities
```

Authentication and authorization services will receive explicit ownership as they are implemented. Their logic will not be hidden inside Response prompts.

---

## 17. Development Discipline

Digit development follows these rules:

- small, deterministic, and easy to use;
- no prompt-specific hard-coded factual answers;
- no duplicated validation systems;
- no inferred authorization when authoritative state can decide it;
- no claims that an action occurred unless the responsible service actually performed it;
- no silent conversion of operator intent into another task;
- one defect should be fixed at the layer that owns the defect;
- module responsibility remains bounded;
- platform-specific behavior remains behind appropriate interfaces where applicable; and
- human-directed development remains authoritative within established policy and law.

During active development, prefer one-file-in / one-patched-file-out work when practical so changes remain reviewable and failures can be isolated without chasing unrelated modules.

---

## 18. Near-Term Development Sequence

The human-directed priority is now identity-aware conversational communications and context. Source mutation, coding/engineering, executive assistance, and network/physical-security integrations remain later capabilities.

1. Preserve established Intent, Corpus, Reasoning, Response, and Validator boundaries; do not hardcode greetings, conversation, or personality in C.
2. Implement server-side identity authentication and GUI login with active-account checks and session revocation.
3. Introduce organization-scoped Projects -> Channels workspaces, with authenticated channel listing, selection, creation and administration.
4. Enforce the mandatory three-part channel eligibility rule: valid Qualification AND active Job Assignment AND valid Mission Qualification; missing any means DENY.
5. Add project security channels, private Digit-to-user delivery, structured security audit, and protected administration.
6. Implement identity-bound active conversational context, followed by controlled persistent context.
7. Verify unscripted natural conversation and full authenticated multi-user communications integration.
8. Expand Source-backed engineering, executive assistance, and operational awareness only after their authorized development phases.

Implementation will be incremental, with negative tests and full module requalification after changes. A written plan or qualification counter is not evidence of a working security boundary.

---

## 19. Target End State

Digit should eventually receive an operator command with enough authoritative context to know:

```text
WHO issued the command
WHAT they requested
WHAT they are qualified to do
WHAT mission they are currently assigned to
WHAT authority applies
WHAT policy constrains the operation
WHAT Digit can actually execute
WHAT evidence is relevant
```

From there, the system can make deterministic decisions wherever deterministic information exists and reserve generative reasoning for the work that actually requires generation.

The intended result is not an agent that says whatever sounds safe.

It is an agent that knows its mission, knows its operator, knows its authority, knows its available capabilities, follows established policy, and produces auditable results.

---

## 20. Identity-Aware Organizational Communications — Operator Requirements (2026-10-08)

### Scope and identity

Digit is STN-LABZ's everyday AI. An employee must authenticate through the GUI before accessing organizational information or invoking authenticated services. No guest/anonymous mode. Identity comes from verified server-side session state, never a chat message or user-supplied display name. Disabled or former-employee accounts are denied, and established sessions are revoked when account eligibility changes. Successful authentication is distinct from authorization.

An identity includes an immutable user identifier, organization memberships, display name and preferred form of address. Identity and session context are separate from Corpus language lessons. Language, grammar, natural/kitty speech and personality remain Corpus-driven; no hardcoded greetings or personality text is introduced to satisfy examples.

### Organizations, projects, channels and roles

The namespace is Organization -> Project -> Channel. Organizations include STN-LABZ and ChAoS Foundation/Team ChAoS as authorized by the operator. Administrative roles are scoped to the authorized organization and its resources; role names by themselves do not confer privilege. Protected operator-private workspaces require explicit authority even from another organization administrator.

Organization administrators may establish and manage their organization's projects, channels and applicable membership/ACL assignments. A project's protected #security channel is provisioned with the project, is persistent and cannot be deleted through ordinary channel administration. Digit and the authorized founding administrator are its initial members. Digit does not automatically join ordinary project #general channels. Digit is a permanent participant in STN-LABZ/#general, her home channel, and in each project #security channel. Other participation requires appropriate authorization/assignment.

### Access decisions

Channel access is denied by default and requires every mandatory factor for the requested scope:

    Qualification VALID
    AND Job Assignment ACTIVE
    AND Mission Qualification VALID
    AND authenticated active organizational identity
    AND explicit resource scope/ACL permitting access

Any missing, revoked, conflicting or unverifiable factor results in NO ACCESS. A successful login does not imply project or channel access. Organizational role and position are distinct from qualification, assignment, mission qualification and actual access. Authorization is deterministic, server-side, and checked on every protected operation, including direct API requests, history reads, project enumeration, channel joins and channel administration—not solely in GUI navigation or language generation. On revocation, affected active access terminates. A user cannot obtain authority by asserting an identity, title, or permission in conversation.

An unauthorized /join request results in a respectful private denial to the requesting session; it does not post to a public channel. Access-denied is not presumed misconduct. Private/undiscoverable resource identifiers are not disclosed to unauthorized users. The denial and access decision are recorded in audit and reported to the appropriate project's restricted #security channel as informational unless established policy determines a higher severity.

### Messages and context

Digit may send private operational messages to an individual authenticated user, and a user may privately converse with Digit. Employee-to-employee instant messaging is prohibited, and Digit must not relay user-authored personal messages between employees as an indirect workaround. Public/project channels are authorized organizational workspaces for interacting with Digit, not a general-purpose IM system.

Private security decisions and notices must never fall back to broadcast if delivery fails. The protected security channel presents security reports and supports authorized administrators reviewing and updating ACL-related records. Changes to qualification/assignment/mission qualification come only from their respective authorized record authorities; editing a channel membership never manufactures an absent qualification.

Digit's context tracks per-identity and per-project conversations, active references, pending access requests and administrative decisions. When a change in authoritative eligibility makes a previously denied request eligible, Digit may prompt an authorized administrator to approve access. Eligibility alone never auto-grants access. After approval the ACL is revalidated, the result audited, and any user notification delivered privately. Cross-user, cross-project and cross-organization context is isolated unless separately authorized.

### GUI entry and operator experience

After successful login, the GUI shows the recognized person's preferred name, organization, current authorized channel and accessible channel list. It displays the controlled notice that employee-to-employee instant messaging is prohibited while private communications with Digit remain allowed. The welcome notice is not an authorization source. Unauthorized projects/channels are not exposed merely because the user guessed their names.

### Release and regression gates

No release may claim these controls exist until server-side enforcement is implemented and verified. Required negative/integration tests include: unauthenticated API denial; invalid/disabled user login denial; session revocation; organization isolation; each missing qualification factor; denied /join private-only delivery; no public fallback; protected #security membership and permanence; project creation with required #security provisioning; admin constrained to organizational scope; inability to send employee-to-employee or proxy IM; per-user context isolation; eligibility-change notification without automatic grant; revalidation at approval time; and retention of the existing natural conversational response regressions.

Existing Core/router behavior and published ABIs remain stable unless an approved design explicitly authorizes a change. Authoritative services, not Response or Corpus, own authentication, authorization, and membership enforcement.

Status: REQUIREMENTS CAPTURED; IMPLEMENTATION AND RUNTIME QUALIFICATION PENDING.
