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

The current planned sequence is:

```text
1. Stabilize Validator input/output behavior.
2. Stabilize Dispatcher intent routing.
3. Prevent Response from duplicating validation/authorization behavior.
4. Add Digit GUI login.
5. Add server-side operator authentication/session handling.
6. Establish authoritative operator context.
7. Add deterministic authorization using identity, qualification, mission, and policy.
8. Carry authenticated/authorized context through command dispatch.
9. Add bounded source-write/development capability where authorized.
10. Improve generated response formatting and structured output.
11. Continue expanding capabilities through bounded modules rather than Core growth.
```

This sequence may be revised by human-directed development as Digit evolves.

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