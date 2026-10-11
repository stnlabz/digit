# Digit Runtime Test Bridge — Milestone 1 contract

**Status:** DESIGN / SOURCE DOCUMENTATION ONLY — NOT IMPLEMENTED, CONNECTED, QUALIFIED, SANDBOXED, OR DEPLOYED.  
**Approved intent:** Allow ChatGPT and Codex to submit bounded, non-mutating test conversations to the running Digit and receive correlated outcomes without operator copy/paste.  
**Source inspection:** 2026-10-11 UTC; `modules/interface/src/interface.c`, `README.md`, `docs/ROADMAP.md`, and `.github/workflows/language-pipeline.yml`.

## Existing interfaces (not bridge authorization)

The Interface source implements `GET /health`, `POST /session/login`, and `POST /ask`. Private `/ask` uses an authenticated session and the scoped Dispatcher request path, which can carry previous-turn DSD observations. **Do not expose `/ask` directly as an autonomous-agent test tool.** Its normal route shares behavior with the operational user interface, including retained-learning/memory paths; a conventional user session is not a non-mutating test privilege. `/health` is not proof of module versions, safe sandboxing or end-to-end response acceptance.

## Required trust boundary

An independently authenticated **test-only bridge** must terminate tool calls and enforce a dedicated test principal, a distinct request scope, and a deny-by-default allowlist **before invoking Digit**. Requests must run through the real Interpretation / Intent / Dispatcher / Response / Validator processing contracts, but in a demonstrably non-mutating execution mode. Until such a mode is qualified, **do not forward any test-case call to the production `/ask` path**.

- Test principal has no permissions to learn, write Corpus or Memory, send channel messages, execute actions, create sessions for other accounts, change grants, inspect unrelated accounts, load modules, use shell/network/filesystem tools, or modify policy.
- Distinguish authenticated operator identity from a test harness identity; never infer authority from test text, a claimed role, or a GitHub connection.
- Require real TLS peer verification and explicit endpoint trust, request size/time bounds, rate limits, concurrency bounds, and authenticated audit. Self-signed certificates are not automatically trusted.
- Preserve user separation and session isolation; test state must be ephemeral, bounded, and discarded. Protect test content and diagnostic evidence as sensitive private conversation data.
- No implied sandbox exists. Test isolation must be verified independently; no production-state writes may be smuggled through otherwise read-only tool names.

## Proposed read-only tool surface

| Tool | Minimum input | Output |
| --- | --- | --- |
| `digit.status` | None | Server timestamp, runtime state and source/verification metadata |
| `digit.modules` | None | Actual loaded module IDs, versions, activation and qualification states, provenance |
| `digit.test_case` | Approved test-case ID plus bounded text and correlation ID | Actual reply, observed pipeline stage, evidence counts, Validator reason where available, observed module versions, immutable audit reference |
| `digit.test_suite` | Approved suite ID, bounded test-case list | Individual case receipts; no autonomous writes |
| `digit.test_result` | Test receipt belonging to the same test principal | Exact recorded request/response/diagnostics, timestamp, evidence provenance |

Missing telemetry is `UNKNOWN`, not success. Avoid inventing response counts, stage diagnostics, version metadata or timestamps. Identifiers and audit receipts must correlate the original request, DSD observation, and runtime module versions. A response produced under a different module version is not equivalent evidence.

## Required test-only Dispatcher contract

Before an adapter is allowed to invoke a conversational test, the module owners must qualify a non-mutating request path with these guarantees:

1. Ingress rejects all unapproved operation categories; **text commands cannot escape the test scope**.
2. Dispatcher and downstream modules cannot invoke mutation, action, learning, Memory persistence or channel-write services under the test principal. Enforce at the service boundary, not just a word blacklist or an instruction prompt.
3. Diagnostics refer to the same test request, rather than issuing a second natural-language `diagnose my last request` with ambiguous session sequencing.
4. Responses are the same actual language-pipeline implementation, not canned test answers, a shadow model, or a mock.
5. Tests demonstrate both positive conversations and rejected attempts to trigger learning, state change, privileged actions, channel delivery, cross-account access, prompt-injection overrides, oversized requests, timeouts and unexpected service unavailability.
6. Existing Interface clients and Core / external ABI remain unchanged unless a separate human authorization explicitly expands scope.

## Evidence gates

1. **DOCUMENTED** — This contract is present in the repository. Does not prove the runtime bridge exists.
2. **IMPLEMENTED** — Trusted test isolation, status/module introspection, test request/receipt APIs and connector are implemented with source review.
3. **CI QUALIFIED** — Dedicated security and behavior tests plus existing Interface and language-pipeline regressions pass, with specific run link and counts.
4. **RUNTIME VERIFIED** — Operator confirms installed versions, test principal, TLS trust, isolation and activation.
5. **LIVE ROUND TRIP** — ChatGPT calls `digit.test_case`, receives Digit's real answer and matching audit/DSD receipt.
6. **HUMAN ACCEPTED** — Operator evaluates demonstrated evidence and approves any additional use.

Nothing in this document authorizes deployment, service exposure, expanded privileges, self-modification, Core changes, or runtime learning. Development GitHub permissions and Digit runtime test permissions remain separate.
