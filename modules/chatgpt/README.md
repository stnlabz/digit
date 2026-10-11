# Digit ChatGPT Integration — 1.0.6 source candidate

ISO C module hosted in Digit's existing process. **Not yet an operational
ChatGPT OAuth connector.** No public HTTP OAuth endpoint or MCP client connection is
claimed by this source candidate.

## ChatGPT-local authentication primitives

- `chatgpt_auth.c` recognizes the existing **Bearer + 64 lowercase hex**
  token header syntax. It does **not** authenticate the token, establish an
  identity, authorize a request, or replace server-side credential checks.
- `digit_chatgpt_pkce_s256` implements RFC 7636 S256 challenge verification.
  It validates input character sets/length and compares the challenge in
  constant time. The module does not yet issue authorization codes or tokens.
- These primitives are built into `chatgpt.so` and covered by
  `tests/test_chatgpt_auth.c`. No additional listeners or processes are added.
- `chatgpt.capabilities` continues to report that Interface binding and live
  conversations are disabled. `chatgpt.test_case` still denies execution.
  The existing `chatgpt.mcp` service only supports limited protocol probes.

## PKCE authorization-code state (ChatGPT module only)

The isolated `chatgpt_oauth_codes.c` component issues cryptographically random,
short-lived (five-minute), single-use authorization codes and redeems them only
with exact client ID, redirect URI, and RFC 7636 S256 verifier matches. A
matching code is consumed even when redemption is rejected. It is in-memory,
not persisted, and is not exposed as a remote service or public route. The
caller must independently verify SA-approved principal authorization, client
registration, redirect URI ownership, consent, and permissions **before**
issuing a code. The authorization-code component does not mint bearer tokens; the separate token lifecycle library does, but neither is connected to a public OAuth exchange.

## OAuth bearer lifecycle (ChatGPT module only)

`chatgpt_oauth_tokens.c` now implements a bounded, in-memory bearer-token
store: CSPRNG generation, SHA-256 digests instead of plaintext stored tokens,
explicit identity and audience binding, 3600-second expiry, and revocation.
The matching test checks invalid principals, wrong audiences, invalid bearer
values, expiration, and revocation. These primitives are not an OAuth token
endpoint; the caller must check live account permissions and consent. They
have not yet been bound to `/oauth/token` or the public `/mcp` route.

## /oauth route namespace (ChatGPT module only)

The in-process `chatgpt.oauth` service now dispatches exact paths:

- `GET /oauth/authorize` — currently 503, until SA-approved authorization and consent are bound.
- `POST /oauth/token` — currently 503, until authenticated authorization-code exchange exists.
- `POST /oauth/register` — currently 503, until approved client registration exists.
- Unsupported methods return 405; unknown paths return 404.
- These paths are **not public HTTP URLs** until a qualified transport binds
  requests to the `chatgpt.oauth` service. No Interface/Core changes were made.
  Denial is intentional; the module does not mint unauthenticated tokens.

## Remaining integration boundary

For an actual ChatGPT MCP OAuth connection, a qualified transport must expose
OAuth authorization-server and protected-resource metadata, an SA-controlled
consent/authorization-code endpoint, PKCE-protected code redemption and
appropriate access-token validation, while retaining the existing identity
and capability controls and private-session DSD continuity.

**These routes are service-level placeholders, not operational OAuth endpoints.** Simply validating a PKCE
challenge or bearer string does not authenticate a ChatGPT user. In particular,
the existing `POST /mcp` HTTP transport currently belongs to Interface and
does not invoke these helpers. No Core or Interface files were modified in
this change, and no authorization boundary has been bypassed.

Build and unit-test (Codex must run and report actual results):

```sh
make -C modules/chatgpt all
make -C modules/chatgpt test
```

Unit tests are not runtime qualification. Prior to claiming connectivity,
Codex must verify negative authorization tests and a real ChatGPT-to-Digit
initialize, tools/list, and two-turn digit_ask round trip. Learning,
Memory/Corpus mutation, Developer and SA authority are never conferred by
authentication alone.
