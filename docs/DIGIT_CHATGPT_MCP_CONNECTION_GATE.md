# Digit Dev — live MCP connection gate (2026-10-11)

## Established evidence

- Installed Digit Interface 1.8.6: QUALIFICATION_GREEN and HOTLOAD_ACTIVE per operator log 2026-10-11T18:03:52Z.
- Installed Digit ChatGPT 1.0.2: QUALIFICATION_GREEN and HOTLOAD_ACTIVE per operator log 2026-10-11T17:57:46Z.
- Plugin Digit Dev 0.1.2 is installed but does not yet include a connected callable MCP runtime tool.
- This document does not authorize additional privileges or deployment.

## Blocking authentication incompatibility

Current Interface `POST /mcp` requires an **existing session bearer** created through `POST /ai/session/login` using a tab-delimited AI platform username and password. ChatGPT's authenticated custom MCP connection instead needs a supported, interactive authorization scheme (typically OAuth 2.1), including published protected-resource metadata and an authorization server. The `/ai/session/login` endpoint is not an OAuth token endpoint or authorization flow.

Therefore the source-defined `/mcp` route **cannot be attached as an authenticated ChatGPT plugin simply by pointing to its URL**. Enabling anonymous `/mcp` would violate the required explicit identity / SA capability boundary and is prohibited.

## Minimum compatible implementation

1. Preserve human account authorization and existing private `/ask` behavior.
2. Add a separately SA-controlled **AI platform OAuth authorization** path with consent, registration/metadata and PKCE appropriate for ChatGPT; bind issued access tokens to exact `ai.<provider>` identity. No credentials, authorization codes, or access tokens in URLs, logs or GitHub.
3. The public HTTPS resource `/mcp` must advertise protected-resource metadata via `.well-known/oauth-protected-resource` and challenge unauthorized clients appropriately, while verifying audience, expiry, identity, revocation and granted capabilities on every tool request.
4. Implement complete MCP Streamable HTTP semantics (supported protocol version negotiation; initialize; notifications/initialized; tools/list; tools/call; JSON-RPC error handling; Accept and MCP session behavior). Do not infer a functional MCP endpoint from the current string-scanning JSON candidate.
5. Expose a single bounded `digit_ask` tool initially; keep token-to-private-session continuity across requests so DSD's previous-turn evidence is preserved.
6. Ensure an actual registered/connected ChatGPT MCP server appears as callable tool in Digit Dev, while GitHub remains a separately connected provider. Do not infer that a plugin skill can create or call a runtime tool.

## Acceptance gate

- Anonymous requests cannot invoke `digit_ask`.
- A valid SA-approved `ai.chatgpt` authorization completes plugin connection with no secret shared in conversation.
- `initialize`, `tools/list`, and two sequential `tools/call` requests succeed through the ChatGPT tool runtime; second asks `Digit, diagnose my last request.` and demonstrates same-session DSD evidence.
- Revoked token, wrong audience, expired token, different AI identity and missing diagnostics permission are all denied.
- Module descriptor version increments on **every module code change** under STN-LABZ revision rules, and the matching qualification expectation updates in the same change. Release acceptance follows actual compilation, tests, qualification and hotload logs.

**Status:** design/connection gap identified. No claim of OAuth implementation, ChatGPT runtime integration, or acceptance.
